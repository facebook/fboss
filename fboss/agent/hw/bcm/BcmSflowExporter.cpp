/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSflowExporter.h"

#include <fstream>
#include <iostream>
#include <vector>

#include <fcntl.h>
#include <ifaddrs.h>

#include <folly/Range.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>
#include <optional>

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"

using namespace std;

namespace {
std::optional<folly::IPAddress> getLocalIPv6FromWhoAmI() {
  const std::string whoAmIFn = "/etc/fbwhoami";
  const std::string key = "DEVICE_PRIMARY_IPV6";

  std::ifstream infile(whoAmIFn);
  std::string line;

  while (std::getline(infile, line)) {
    std::vector<std::string> kv;
    folly::split('=', line, kv);
    if (kv.size() != 2) {
      continue;
    }
    if (kv[0] == key) {
      try {
        return folly::IPAddress(kv[1]);
      } catch (std::exception const& e) {
        XLOG(DBG2) << folly::exceptionStr(e);
        return std::nullopt;
      }
    }
  }
  return std::nullopt;
}

folly::IPAddress getLocalIPv6() {
  // We first try to get the local IPv6 in fbwhoami
  auto ret = getLocalIPv6FromWhoAmI();
  if (ret.has_value()) {
    XLOG(DBG2) << "Got local IPv6 address from fbwhoami";
    return ret.value();
  }

  struct ifaddrs* ifaddr{nullptr};
  std::vector<char> host;
  host.reserve(NI_MAXHOST);

  if (getifaddrs(&ifaddr) == -1) {
    XLOG(DBG2) << "getifaddrs failed. Returned default address ::";
    return folly::IPAddress("::");
  }
  SCOPE_EXIT {
    freeifaddrs(ifaddr);
  };

  for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr) {
      continue;
    }
    std::string ifname{ifa->ifa_name};
    if (ifname != "eth0" or ifa->ifa_addr->sa_family != AF_INET6) {
      continue;
    }
    int retno = getnameinfo(
        ifa->ifa_addr,
        sizeof(struct sockaddr_in6),
        host.data(),
        NI_MAXHOST,
        nullptr,
        0,
        NI_NUMERICHOST);
    if (retno != 0) {
      XLOG(DBG2) << "getnameinfo() failed: " << gai_strerror(retno);
      continue;
    }
    try {
      return folly::IPAddress(host.data());
    } catch (std::exception const& e) {
      XLOG(DBG2) << folly::exceptionStr(e);
      continue;
    }
  }
  XLOG(DBG2) << "Failed to get loopback ipv6 address, returned default one ::";
  return folly::IPAddress("::");
}
} // namespace

namespace facebook::fboss {

BcmSflowExporter::BcmSflowExporter(const folly::SocketAddress& address)
    : address_(address) {
  SCOPE_FAIL {
    close(socket_);
  };

  socket_ = ::socket(address_.getFamily(), SOCK_DGRAM, IPPROTO_UDP);

  if (socket_ == -1) {
    throw FbossError("Error creating UDP socket: ", folly::errnoStr(errno));
  }

  // put the socket in non-blocking mode
  if (fcntl(socket_, F_SETFL, O_NONBLOCK) != 0) {
    throw FbossError(
        "Failed to put socket in non-blocking mode: ", folly::errnoStr(errno));
  }

  // put the socket in reuse mode
  int val = 1;
  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
    throw FbossError(
        "Failed to put socket in reuse_addr mode: ", folly::errnoStr(errno));
  }

  // put the socket in port reuse mode
  val = 1;
  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != 0) {
    throw FbossError(
        "Failed to put socket in reuse_port mode: ", folly::errnoStr(errno));
  }

  // bind the socket.
  folly::SocketAddress localAddr;

  switch (address.getFamily()) {
    case AF_INET6:
      localAddr = folly::SocketAddress("::", 0);
      break;
    case AF_INET:
      localAddr = folly::SocketAddress("0.0.0.0", 0);
      break;
    default:
      throw FbossError("Unsupported address family for exporter target");
  }

  sockaddr_storage addrStorage;
  localAddr.getAddress(&addrStorage);
  sockaddr* saddr = reinterpret_cast<sockaddr*>(&addrStorage);
  if (bind(socket_, saddr, localAddr.getActualSize()) != 0) {
    throw FbossError(
        "Failed to bind the async udp socket for ",
        address_.describe(),
        ": ",
        folly::errnoStr(errno));
  }
}

ssize_t BcmSflowExporter::sendUDPDatagram(iovec* vec, const size_t iovec_len) {
  XLOG(DBG4) << "Sending an sFlow packet to " << address_.describe();

  sockaddr_storage addrStorage;
  address_.getAddress(&addrStorage);

  struct msghdr msg = {};
  msg.msg_name = reinterpret_cast<void*>(&addrStorage);
  msg.msg_namelen = address_.getActualSize();
  msg.msg_iov = vec;
  msg.msg_iovlen = iovec_len;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;
  auto ret = ::sendmsg(socket_, &msg, 0);
  if (ret < 0) {
    XLOG(DBG1) << "Failed sending sFlow packet to " << address_.describe()
               << " reason: " << folly::errnoStr(errno);
  }
  XLOG(DBG4) << "Sent " << ret << " bytes of sFlow packet to "
             << address_.describe();
  return ret;
}

BcmSflowExporter::~BcmSflowExporter() {
  if (socket_ != -1) {
    close(socket_);
  }
}

bool BcmSflowExporterTable::contains(
    const shared_ptr<SflowCollector>& c) const {
  auto iter = map_.find(c->getID());
  return iter != map_.end();
}

size_t BcmSflowExporterTable::size() const {
  return map_.size();
}

void BcmSflowExporterTable::addExporter(const shared_ptr<SflowCollector>& c) {
  try {
    auto exporter = make_unique<BcmSflowExporter>(c->getAddress());
    map_.emplace(c->getID(), std::move(exporter));
  } catch (const fboss::thrift::FbossBaseError& ex) {
    XLOG(ERR) << "Could not add exporter: "
              << c->getAddress().getFullyQualified()
              << " reason: " << folly::exceptionStr(ex);
    return;
  }

  XLOG(DBG2) << "Successfully added exporter for "
             << c->getAddress().getFullyQualified();
}

void BcmSflowExporterTable::removeExporter(const std::string& id) {
  XLOG(DBG2) << "Removed sFlow exporter " << id;
  map_.erase(id);
}

void BcmSflowExporterTable::updateSamplingRates(
    PortID id,
    int64_t inRate,
    int64_t outRate) {
  std::pair<int64_t, int64_t> rates(inRate, outRate);
  auto it = port2samplingRates_.find(id);
  if (it != port2samplingRates_.end()) {
    it->second = rates;
  } else {
    port2samplingRates_.insert(std::make_pair(id, rates));
  }

  // We piggyback the update of local IPv6
  localIP_ = getLocalIPv6();
}

void BcmSflowExporterTable::sendToAll(const SflowPacketInfo& info) {
  if (map_.empty()) {
    XLOG(DBG1)
        << "zero sFlow collectors with sflow enabled, skipping sample export";
    return;
  }
  // Serialize info to a string and wrap it in an IOBuf for sending
  string output;
  apache::thrift::BinarySerializer::serialize(info, &output);
  auto buf = folly::IOBuf::wrapBuffer(output.data(), output.length());

  // Map the IOBuf to a Linux iovec like a champ
  iovec vec[16] = {};
  size_t iovec_len = buf->fillIov(vec, sizeof(vec) / sizeof(vec[0])).numIovecs;
  if (UNLIKELY(iovec_len == 0)) {
    buf->coalesce();
    vec[0].iov_base = const_cast<uint8_t*>(buf->data());
    vec[0].iov_len = buf->length();
    iovec_len = 1;
  }

  for (const auto& c : map_) {
    // TODO: prob need to handle ret code?
    c.second->sendUDPDatagram(vec, iovec_len);
  }
}

} // namespace facebook::fboss
