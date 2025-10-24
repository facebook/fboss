/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "common/network/NetworkUtil.h"

namespace facebook::fboss::utility {

using facebook::fboss::TestCtrl;
using RunForHwAgentFn = std::function<void(
    apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client)>;

std::unique_ptr<apache::thrift::Client<TestCtrl>> getSwAgentThriftClient(
    const std::string& switchName) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress agent(remoteSwitchIp, 5909);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<TestCtrl>>(std::move(channel));
}

std::unique_ptr<apache::thrift::Client<FbossHwCtrl>> getHwAgentThriftClient(
    const std::string& switchName,
    int port) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress agent(remoteSwitchIp, port);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<FbossHwCtrl>>(
      std::move(channel));
}

MultiSwitchRunState getMultiSwitchRunState(const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  MultiSwitchRunState runState;
  swAgentClient->sync_getMultiSwitchRunState(runState);
  return runState;
}

int getNumHwSwitches(const std::string& switchName) {
  auto runState = getMultiSwitchRunState(switchName);
  return runState.hwIndexToRunState()->size();
}

void runOnAllHwAgents(
    const std::string& switchName,
    const RunForHwAgentFn& fn) {
  static const std::vector kHwAgentPorts = {5931, 5932};
  auto numHwSwitches = getNumHwSwitches(switchName);
  for (int i = 0; i < numHwSwitches; i++) {
    auto hwAgentClient = getHwAgentThriftClient(switchName, kHwAgentPorts[i]);
    fn(*hwAgentClient);
  }
}

} // namespace facebook::fboss::utility
