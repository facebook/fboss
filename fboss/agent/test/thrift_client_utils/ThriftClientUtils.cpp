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

std::map<std::string, FabricEndpoint> getFabricPortToFabricEndpoint(
    const std::string& switchName) {
  std::map<std::string, FabricEndpoint> fabricPortToFabricEndpoint;
  auto hwAgentQueryFn = [&fabricPortToFabricEndpoint](
                            apache::thrift::Client<FbossHwCtrl>& client) {
    std::map<std::string, FabricEndpoint> hwAgentEntries;
    client.sync_getHwFabricConnectivity(hwAgentEntries);
    fabricPortToFabricEndpoint.merge(hwAgentEntries);
  };
  runOnAllHwAgents(switchName, hwAgentQueryFn);

  return fabricPortToFabricEndpoint;
}

std::map<std::string, std::vector<std::string>> getRemoteSwitchToReachablePorts(
    const std::string& switchName,
    const std::vector<std::string>& remoteSwitches) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::map<std::string, std::vector<std::string>> remoteSwitchToReachablePorts;
  swAgentClient->sync_getSwitchReachability(
      remoteSwitchToReachablePorts, remoteSwitches);

  return remoteSwitchToReachablePorts;
}

std::map<int32_t, PortInfoThrift> getPortIdToPortInfo(
    const std::string& switchName) {
  std::map<int32_t, PortInfoThrift> portIdToPortInfo;
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_getAllPortInfo(portIdToPortInfo);
  return portIdToPortInfo;
}

std::map<int64_t, facebook::fboss::SystemPortThrift>
getSystemPortdIdToSystemPort(const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::map<int64_t, facebook::fboss::SystemPortThrift> systemPortIdToSystemPort;
  swAgentClient->sync_getSystemPorts(systemPortIdToSystemPort);
  return systemPortIdToSystemPort;
}

std::map<int32_t, facebook::fboss::InterfaceDetail> getIntfIdToIntf(
    const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::map<int32_t, facebook::fboss::InterfaceDetail> intfIdToIntf;
  swAgentClient->sync_getAllInterfaces(intfIdToIntf);
  return intfIdToIntf;
}

std::vector<facebook::fboss::NdpEntryThrift> getNdpEntries(
    const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::vector<facebook::fboss::NdpEntryThrift> ndpEntries;
  swAgentClient->sync_getNdpTable(ndpEntries);
  return ndpEntries;
}

} // namespace facebook::fboss::utility
