/*
 * The parts of OpenCPN's network side this build leaves out, answered so
 * that the rest links (see NONET_* in scripts/sources.mk): downloads (curl),
 * sending routes to another OpenCPN (curl), the REST server's certificate
 * (OpenSSL) and mDNS. Each says it isn't here and does nothing.
 * Copyright (c) 2026 AmiCradle Team (GPL v2 or later, as OpenCPN)
 */
#include <ostream>
#include <string>
#include <vector>

#include "model/certificates.h"
#include "model/downloader.h"
#include "model/logger.h"
#include "model/mdns_query.h"
#include "model/mdns_service.h"
#include "model/peer_client.h"

// Signal K servers found by mDNS (none: mDNS is parked).
std::vector<ocpn_DNS_record_t> g_sk_servers;

static const char* const kNoNet = "not in this build of OpenCPN";

// ---- downloads (model/src/downloader.cpp uses curl)

Downloader::Downloader(std::string url_)
    : url(std::move(url_)), stream(nullptr), errorcode(0) {}

int Downloader::last_errorcode() { return errorcode; }

std::string Downloader::last_error() { return error_msg; }

void Downloader::on_chunk(const char* buff, unsigned bytes) {
  if (stream) stream->write(buff, bytes);
}

bool Downloader::download(std::ostream* s) {
  stream = s;
  errorcode = -1;
  error_msg = std::string("Downloads are ") + kNoNet + ": " + url;
  MESSAGE_LOG << error_msg;
  return false;
}

bool Downloader::download(std::string& path) {
  (void)path;
  return download(static_cast<std::ostream*>(nullptr));
}

long Downloader::get_filesize() { return 0; }

// ---- sending routes, tracks and marks to another OpenCPN (peer_client.cpp)

using PeerDlgPair = std::pair<PeerDlgResult, std::string>;

PeerData::PeerData(EventVar& p)
    : overwrite(false),
      activate(false),
      progress(p),
      run_status_dlg([](PeerDlg, int) { return PeerDlgResult::Cancel; }),
      run_pincode_dlg([] { return PeerDlgPair(PeerDlgResult::Cancel, ""); }) {}

void GetApiVersion(PeerData& peer_data) { (void)peer_data; }

bool SendNavobjects(PeerData& peer_data) {
  MESSAGE_LOG << "Sending to " << peer_data.dest_ip_address << " is " << kNoNet;
  peer_data.run_status_dlg(PeerDlg::InvalidHttpResponse, -1);
  return false;
}

bool CheckNavObjects(PeerData& peer_data) {
  (void)peer_data;
  return false;
}

// ---- the REST server's certificate (certificates.cpp uses OpenSSL)

int make_certificate(std::string ip_v4, std::string destination_dir) {
  (void)ip_v4;
  (void)destination_dir;
  return -1;
}

// ---- mDNS (mdns_query.cpp, mdns_service.cpp)

int StartMDNSService(std::string hostname, std::string service_name,
                     int service_port) {
  (void)hostname;
  (void)service_name;
  (void)service_port;
  return -1;
}

bool StopMDNSService() { return true; }

int send_dns_sd(void) { return -1; }

void FindAllOCPNServers(size_t timeout_secs) { (void)timeout_secs; }

void FindAllSignalKServers(size_t timeout_secs) { (void)timeout_secs; }

std::vector<std::string> get_local_ipv4_addresses() { return {}; }
