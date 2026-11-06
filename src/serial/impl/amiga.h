/*
 * serial::Serial on AmigaOS 3: serial.device (or any device that speaks
 * its commands, such as a USB serial adapter's), one IOExtSer per port.
 * The port name is "<device>/<unit>", e.g. "serial.device/0".
 * Copyright (c) 2026 AmiCradle Team
 * MIT licence, as the serial library it plugs into.
 */
#ifndef SERIAL_IMPL_AMIGA_H
#define SERIAL_IMPL_AMIGA_H

#include "serial/serial.h"
#include <mutex>

struct IOExtSer;

namespace serial {

using std::size_t;
using std::string;

class MillisecondTimer {
public:
  MillisecondTimer(const uint32_t millis);
  int64_t remaining();
private:
  uint64_t expiry_ms;
};

class serial::Serial::SerialImpl {
public:
  SerialImpl(const string &port, unsigned long baudrate, bytesize_t bytesize,
             parity_t parity, stopbits_t stopbits, flowcontrol_t flowcontrol);
  virtual ~SerialImpl();

  void open();
  void close();
  bool isOpen() const;
  size_t available();
  bool waitReadable(uint32_t timeout);
  void waitByteTimes(size_t count);
  size_t read(uint8_t *buf, size_t size = 1);
  size_t write(const uint8_t *data, size_t length);
  void flush();
  void flushInput();
  void flushOutput();
  void sendBreak(int duration);
  void setBreak(bool level);
  void setRTS(bool level);
  void setDTR(bool level);
  bool waitForChange();
  bool getCTS();
  bool getDSR();
  bool getRI();
  bool getCD();
  void setPort(const string &port);
  string getPort() const;
  void setTimeout(Timeout &timeout);
  Timeout getTimeout() const;
  void setBaudrate(unsigned long baudrate);
  unsigned long getBaudrate() const;
  void setBytesize(bytesize_t bytesize);
  bytesize_t getBytesize() const;
  void setParity(parity_t parity);
  parity_t getParity() const;
  void setStopbits(stopbits_t stopbits);
  stopbits_t getStopbits() const;
  void setFlowcontrol(flowcontrol_t flowcontrol);
  flowcontrol_t getFlowcontrol() const;
  void readLock();
  void readUnlock();
  void writeLock();
  void writeUnlock();

protected:
  void reconfigurePort();

private:
  // one command on the port, from whichever task calls (OpenCPN reads in
  // its own thread): the reply port is made for the call and freed after
  long doIO(unsigned short command, void *data = nullptr, unsigned long length = 0,
            unsigned long *actual = nullptr);
  unsigned short status();

  string port_;
  IOExtSer *io_;
  bool is_open_;
  Timeout timeout_;
  unsigned long baudrate_;
  uint32_t byte_time_ms_;
  parity_t parity_;
  bytesize_t bytesize_;
  stopbits_t stopbits_;
  flowcontrol_t flowcontrol_;
  std::mutex read_mutex_;
  std::mutex write_mutex_;
};

}  // namespace serial

#endif
