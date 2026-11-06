/*
 * serial::Serial on AmigaOS 3, through serial.device and the devices that
 * speak its commands (SDCMD_SETPARAMS, SDCMD_QUERY, CMD_READ, CMD_WRITE).
 * A port is named "<device>/<unit>", for example "serial.device/0".
 *
 * OpenCPN opens a port in one thread and reads it in another, so every
 * command makes its own reply port in the task that sends it.
 *
 * Copyright (c) 2026 AmiCradle Team
 * MIT licence, as the serial library it plugs into.
 */
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

#include <exec/types.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <devices/serial.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "serial/serial.h"
#include "serial/impl/amiga.h"

using std::string;
using serial::MillisecondTimer;
using serial::Serial;
using serial::SerialException;
using serial::PortNotOpenedException;
using serial::IOException;

// ---- time, from the DOS clock (ticks of 20 ms)

static uint64_t now_ms() {
  struct DateStamp ds;
  DateStamp(&ds);
  return (uint64_t)ds.ds_Days * 86400000ULL + (uint64_t)ds.ds_Minute * 60000ULL +
         (uint64_t)ds.ds_Tick * (1000 / TICKS_PER_SECOND);
}

static void sleep_ms(uint32_t ms) {
  LONG ticks = (LONG)((ms * TICKS_PER_SECOND + 999) / 1000);
  Delay(ticks > 0 ? ticks : 1);
}

MillisecondTimer::MillisecondTimer(const uint32_t millis)
    : expiry_ms(now_ms() + millis) {}

int64_t MillisecondTimer::remaining() {
  return (int64_t)expiry_ms - (int64_t)now_ms();
}

// ---- "serial.device/0" -> device name and unit

static void split_port(const string &port, string &device, ULONG &unit) {
  size_t slash = port.rfind('/');
  device = port;
  unit = 0;
  if (slash != string::npos && slash + 1 < port.size()) {
    char *end = nullptr;
    unsigned long u = strtoul(port.c_str() + slash + 1, &end, 10);
    if (end && *end == '\0') {
      device = port.substr(0, slash);
      unit = u;
    }
  }
  // OpenCPN shows ports as "name - description"
  size_t dash = device.find(" - ");
  if (dash != string::npos) device.erase(dash);
}

Serial::SerialImpl::SerialImpl(const string &port, unsigned long baudrate,
                               bytesize_t bytesize, parity_t parity,
                               stopbits_t stopbits, flowcontrol_t flowcontrol)
    : port_(port),
      io_(nullptr),
      is_open_(false),
      baudrate_(baudrate),
      byte_time_ms_(1),
      parity_(parity),
      bytesize_(bytesize),
      stopbits_(stopbits),
      flowcontrol_(flowcontrol) {
  if (!port_.empty()) open();
}

Serial::SerialImpl::~SerialImpl() { close(); }

long Serial::SerialImpl::doIO(unsigned short command, void *data,
                              unsigned long length, unsigned long *actual) {
  struct MsgPort *mp = CreateMsgPort();
  if (!mp) throw SerialException("serial: no memory for a reply port");
  io_->IOSer.io_Message.mn_ReplyPort = mp;
  io_->IOSer.io_Command = command;
  io_->IOSer.io_Data = data;
  io_->IOSer.io_Length = length;
  io_->IOSer.io_Flags = 0;
  LONG err = DoIO((struct IORequest *)io_);
  if (actual) *actual = io_->IOSer.io_Actual;
  io_->IOSer.io_Message.mn_ReplyPort = nullptr;
  DeleteMsgPort(mp);
  return err;
}

void Serial::SerialImpl::open() {
  if (port_.empty()) throw std::invalid_argument("Empty port is invalid.");
  if (is_open_) throw SerialException("Serial port already open.");

  string device;
  ULONG unit;
  split_port(port_, device, unit);

  struct MsgPort *mp = CreateMsgPort();
  if (!mp) throw SerialException("serial: no memory for a reply port");
  io_ = (IOExtSer *)CreateIORequest(mp, sizeof(struct IOExtSer));
  if (!io_) {
    DeleteMsgPort(mp);
    throw SerialException("serial: no memory for an I/O request");
  }
  io_->io_SerFlags = SERF_SHARED;
  if (flowcontrol_ != flowcontrol_software) io_->io_SerFlags |= SERF_XDISABLED;
  if (flowcontrol_ == flowcontrol_hardware) io_->io_SerFlags |= SERF_7WIRE;
  BYTE err = OpenDevice((CONST_STRPTR)device.c_str(), unit, (struct IORequest *)io_, 0);
  io_->IOSer.io_Message.mn_ReplyPort = nullptr;
  DeleteMsgPort(mp);
  if (err != 0) {
    DeleteIORequest((struct IORequest *)io_);
    io_ = nullptr;
    THROW(IOException, (port_ + ": the device would not open").c_str());
  }
  is_open_ = true;
  reconfigurePort();
}

void Serial::SerialImpl::reconfigurePort() {
  if (!is_open_) throw SerialException("serial: port not open");
  io_->io_Baud = baudrate_;
  io_->io_ReadLen = io_->io_WriteLen = (UBYTE)bytesize_;
  io_->io_StopBits = (stopbits_ == stopbits_two) ? 2 : 1;
  io_->io_RBufLen = 8192;
  io_->io_ExtFlags = 0;
  UBYTE f = io_->io_SerFlags & SERF_SHARED;
  if (flowcontrol_ != flowcontrol_software) f |= SERF_XDISABLED;
  if (flowcontrol_ == flowcontrol_hardware) f |= SERF_7WIRE;
  switch (parity_) {
    case parity_odd: f |= SERF_PARTY_ON | SERF_PARTY_ODD; break;
    case parity_even: f |= SERF_PARTY_ON; break;
    case parity_mark: f |= SERF_PARTY_ON; io_->io_ExtFlags = SEXTF_MSPON | SEXTF_MARK; break;
    case parity_space: f |= SERF_PARTY_ON; io_->io_ExtFlags = SEXTF_MSPON; break;
    default: break;
  }
  io_->io_SerFlags = f;
  if (doIO(SDCMD_SETPARAMS) != 0)
    THROW(IOException, (port_ + ": the device refused these settings").c_str());
  // one byte: start bit, data bits, parity, stop bits
  uint32_t bits = 1 + (uint32_t)bytesize_ + (parity_ != parity_none ? 1 : 0) +
                  (stopbits_ == stopbits_two ? 2 : 1);
  byte_time_ms_ = std::max<uint32_t>(1, (bits * 1000 + baudrate_ - 1) / std::max(1UL, baudrate_));
}

void Serial::SerialImpl::close() {
  if (!is_open_ || !io_) return;
  struct MsgPort *mp = CreateMsgPort();
  io_->IOSer.io_Message.mn_ReplyPort = mp;
  if (mp && !CheckIO((struct IORequest *)io_)) {
    AbortIO((struct IORequest *)io_);
    WaitIO((struct IORequest *)io_);
  }
  CloseDevice((struct IORequest *)io_);
  DeleteIORequest((struct IORequest *)io_);
  if (mp) DeleteMsgPort(mp);
  io_ = nullptr;
  is_open_ = false;
}

bool Serial::SerialImpl::isOpen() const { return is_open_; }

size_t Serial::SerialImpl::available() {
  if (!is_open_) return 0;
  unsigned long n = 0;
  if (doIO(SDCMD_QUERY, nullptr, 0, &n) != 0) THROW(IOException, "serial: query failed");
  return n;
}

bool Serial::SerialImpl::waitReadable(uint32_t timeout) {
  MillisecondTimer t(timeout);
  for (;;) {
    if (available() > 0) return true;
    int64_t left = t.remaining();
    if (left <= 0) return false;
    sleep_ms((uint32_t)std::min<int64_t>(left, 20));
  }
}

void Serial::SerialImpl::waitByteTimes(size_t count) {
  sleep_ms((uint32_t)(count * byte_time_ms_));
}

size_t Serial::SerialImpl::read(uint8_t *buf, size_t size) {
  if (!is_open_) throw PortNotOpenedException("Serial::read");
  size_t got = 0;
  long total = timeout_.read_timeout_constant +
               (long)timeout_.read_timeout_multiplier * (long)size;
  MillisecondTimer total_timeout(total);
  for (;;) {
    size_t avail = available();
    if (avail > 0) {
      unsigned long n = 0;
      if (doIO(CMD_READ, buf + got, std::min(avail, size - got), &n) != 0)
        THROW(IOException, "serial: read failed");
      got += n;
      if (got >= size) break;
      continue;
    }
    int64_t left = total_timeout.remaining();
    if (left <= 0) break;
    uint32_t wait = std::min<uint32_t>((uint32_t)left, timeout_.inter_byte_timeout);
    if (!waitReadable(wait) && got > 0) break;
  }
  return got;
}

size_t Serial::SerialImpl::write(const uint8_t *data, size_t length) {
  if (!is_open_) throw PortNotOpenedException("Serial::write");
  unsigned long n = 0;
  if (doIO(CMD_WRITE, (void *)data, length, &n) != 0) THROW(IOException, "serial: write failed");
  return n;
}

void Serial::SerialImpl::setPort(const string &port) { port_ = port; }
string Serial::SerialImpl::getPort() const { return port_; }
void Serial::SerialImpl::setTimeout(serial::Timeout &timeout) { timeout_ = timeout; }
serial::Timeout Serial::SerialImpl::getTimeout() const { return timeout_; }

void Serial::SerialImpl::setBaudrate(unsigned long baudrate) {
  baudrate_ = baudrate;
  if (is_open_) reconfigurePort();
}
unsigned long Serial::SerialImpl::getBaudrate() const { return baudrate_; }
void Serial::SerialImpl::setBytesize(serial::bytesize_t bytesize) {
  bytesize_ = bytesize;
  if (is_open_) reconfigurePort();
}
serial::bytesize_t Serial::SerialImpl::getBytesize() const { return bytesize_; }
void Serial::SerialImpl::setParity(serial::parity_t parity) {
  parity_ = parity;
  if (is_open_) reconfigurePort();
}
serial::parity_t Serial::SerialImpl::getParity() const { return parity_; }
void Serial::SerialImpl::setStopbits(serial::stopbits_t stopbits) {
  stopbits_ = stopbits;
  if (is_open_) reconfigurePort();
}
serial::stopbits_t Serial::SerialImpl::getStopbits() const { return stopbits_; }
void Serial::SerialImpl::setFlowcontrol(serial::flowcontrol_t flowcontrol) {
  flowcontrol_ = flowcontrol;
  if (is_open_) reconfigurePort();
}
serial::flowcontrol_t Serial::SerialImpl::getFlowcontrol() const { return flowcontrol_; }

void Serial::SerialImpl::flush() {
  // CMD_WRITE waits until the device has taken the data
}
void Serial::SerialImpl::flushInput() {
  if (!is_open_) throw PortNotOpenedException("Serial::flushInput");
  doIO(CMD_CLEAR);
}
void Serial::SerialImpl::flushOutput() {
  if (!is_open_) throw PortNotOpenedException("Serial::flushOutput");
  doIO(CMD_FLUSH);
}

void Serial::SerialImpl::sendBreak(int duration) {
  if (!is_open_) throw PortNotOpenedException("Serial::sendBreak");
  io_->io_BrkTime = (ULONG)std::max(1, duration) * 1000;  // microseconds
  doIO(SDCMD_BREAK);
}
void Serial::SerialImpl::setBreak(bool level) {
  if (level) sendBreak(250);
}
// serial.device drives RTS and DTR itself (RTS with the 7-wire handshake)
void Serial::SerialImpl::setRTS(bool) {}
void Serial::SerialImpl::setDTR(bool) {}

unsigned short Serial::SerialImpl::status() {
  if (!is_open_) throw PortNotOpenedException("Serial::status");
  doIO(SDCMD_QUERY);
  return io_->io_Status;
}
// io_Status: the modem lines are active low
bool Serial::SerialImpl::getDSR() { return !(status() & (1 << 3)); }
bool Serial::SerialImpl::getCTS() { return !(status() & (1 << 4)); }
bool Serial::SerialImpl::getCD() { return !(status() & (1 << 5)); }
bool Serial::SerialImpl::getRI() { return !(status() & (1 << 2)); }

bool Serial::SerialImpl::waitForChange() {
  unsigned short was = status() & 0x3c;
  for (int i = 0; i < 500; i++) {  // 10 s at most
    sleep_ms(20);
    if ((status() & 0x3c) != was) return true;
  }
  return false;
}

void Serial::SerialImpl::readLock() { read_mutex_.lock(); }
void Serial::SerialImpl::readUnlock() { read_mutex_.unlock(); }
void Serial::SerialImpl::writeLock() { write_mutex_.lock(); }
void Serial::SerialImpl::writeUnlock() { write_mutex_.unlock(); }

// ---- the ports there are: unit 0 of each serial device in DEVS:

static bool is_serial_device(const char *name) {
  string n(name);
  for (auto &c : n) c = (char)tolower((unsigned char)c);
  if (n.size() < 8 || n.compare(n.size() - 7, 7, ".device") != 0) return false;
  return n.find("serial") != string::npos || n.find("acm") != string::npos;
}

std::vector<serial::PortInfo> serial::list_ports() {
  std::vector<serial::PortInfo> ports;
  std::vector<string> names;
  BPTR lock = Lock((CONST_STRPTR) "DEVS:", ACCESS_READ);
  if (lock) {
    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, nullptr);
    if (fib) {
      if (Examine(lock, fib)) {
        while (ExNext(lock, fib)) {
          if (fib->fib_DirEntryType < 0 && is_serial_device((const char *)fib->fib_FileName))
            names.push_back((const char *)fib->fib_FileName);
        }
      }
      FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
  }
  // serial.device is in the ROM: it is there even when DEVS: has no file,
  // and it comes first
  names.erase(std::remove(names.begin(), names.end(), string("serial.device")), names.end());
  std::sort(names.begin(), names.end());
  names.insert(names.begin(), "serial.device");
  for (const auto &n : names) {
    serial::PortInfo p;
    p.port = n + "/0";
    p.description = (n == "serial.device") ? "Amiga serial port" : n;
    p.hardware_id = "n/a";
    ports.push_back(p);
  }
  return ports;
}
