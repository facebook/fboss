/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/cli/fboss2/CmdUtils.h"

#include "fboss/cli/fboss2/CmdGlobalOptions.h"

#include <folly/logging/LogConfig.h>
#include <folly/logging/LoggerDB.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <chrono>
#include <fstream>

using namespace std::chrono;

using folly::ByteRange;
using folly::IPAddress;
using folly::IPAddressV6;

namespace facebook::fboss::utils {

const folly::IPAddress getIPFromHost(const std::string& hostname) {
  if (IPAddress::validate(hostname)) {
    return IPAddress(hostname);
  }
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* result = nullptr;
  auto rv = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);

  SCOPE_EXIT {
    freeaddrinfo(result);
  };
  if (rv < 0) {
    XLOG(ERR) << "Could not get an IP for: " << hostname;
    return IPAddress();
  }
  struct addrinfo* res;
  for (res = result; res != nullptr; res = res->ai_next) {
    if (res->ai_addr->sa_family == AF_INET) { // IPV4
      struct sockaddr_in* sp = (struct sockaddr_in*)res->ai_addr;
      return IPAddress::fromLong(sp->sin_addr.s_addr);
    } else if (res->ai_addr->sa_family == AF_INET6) { // IPV6
      struct sockaddr_in6* sp = (struct sockaddr_in6*)res->ai_addr;
      return IPAddress::fromBinary(ByteRange(
          static_cast<unsigned char*>(&(sp->sin6_addr.s6_addr[0])),
          IPAddressV6::byteCount()));
    }
  }
  XLOG(ERR) << "Could not get an IP for: " << hostname;
  return IPAddress();
}

std::vector<std::string> getHostsFromFile(const std::string& filename) {
  std::vector<std::string> hosts;
  std::ifstream in(filename);
  std::string str;

  while (std::getline(in, str)) {
    hosts.push_back(str);
  }

  return hosts;
}

void setLogLevel(std::string logLevelStr) {
  if (logLevelStr == "DBG0") {
    return;
  }

  auto logLevel = folly::stringToLogLevel(logLevelStr);

  auto logConfig = folly::LoggerDB::get().getConfig();
  auto& categoryMap = logConfig.getCategoryConfigs();

  for (auto& p : categoryMap) {
    auto category = folly::LoggerDB::get().getCategory(p.first);
    if (category != nullptr) {
      folly::LoggerDB::get().setLevel(category, logLevel);
    }
  }

  XLOG(DBG1) << "Setting loglevel to " << logLevelStr;
}

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient>
createPlaintextAgentClient(const std::string& ip) {
  auto eb = folly::EventBaseManager::get()->getEventBase();
  auto agentPort = CmdGlobalOptions::getInstance()->getAgentThriftPort();
  auto addr = folly::SocketAddress(ip, agentPort);
  auto sock = folly::AsyncSocket::newSocket(eb, addr, kConnTimeout);
  sock->setSendTimeout(kSendTimeout);
  auto channel =
      apache::thrift::HeaderClientChannel::newChannel(std::move(sock));
  channel->setTimeout(kRecvTimeout);
  return std::make_unique<facebook::fboss::FbossCtrlAsyncClient>(
      std::move(channel));
}

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient>
createPlaintextQsfpClient(const std::string& ip) {
  auto eb = folly::EventBaseManager::get()->getEventBase();
  auto qsfpServicePort = CmdGlobalOptions::getInstance()->getQsfpThriftPort();
  auto addr = folly::SocketAddress(ip, qsfpServicePort);
  auto sock = folly::AsyncSocket::newSocket(eb, addr, kConnTimeout);
  sock->setSendTimeout(kSendTimeout);
  auto channel =
      apache::thrift::HeaderClientChannel::newChannel(std::move(sock));
  channel->setTimeout(kRecvTimeout);
  return std::make_unique<facebook::fboss::QsfpServiceAsyncClient>(
      std::move(channel));
}

} // namespace facebook::fboss::utils
