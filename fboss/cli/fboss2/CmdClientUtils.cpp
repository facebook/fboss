/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/cli/fboss2/CmdClientUtils.h"

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"

#include <string>

namespace facebook::fboss::utils {

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

template <>
std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createClient(
    const std::string& ip) {
  return utils::createAgentClient(ip);
}

template <>
std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createClient(
    const std::string& ip) {
  return utils::createQsfpClient(ip);
}

} // namespace facebook::fboss::utils
