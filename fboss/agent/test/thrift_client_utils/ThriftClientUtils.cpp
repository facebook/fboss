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

std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
getQsfpThriftClient(const std::string& switchName) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress qsfp(remoteSwitchIp, 5910);
  auto socket = folly::AsyncSocket::newSocket(eb, qsfp);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<QsfpService>>(
      std::move(channel));
}

std::unique_ptr<apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>
getFsdbThriftClient(const std::string& switchName) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress fsdb(remoteSwitchIp, 5908);
  auto socket = folly::AsyncSocket::newSocket(eb, fsdb);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<
      apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>(
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

QsfpServiceRunState getQsfpServiceRunState(const std::string& switchName) {
  auto qsfpClient = getQsfpThriftClient(switchName);
  return qsfpClient->sync_getQsfpServiceRunState();
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

std::vector<facebook::fboss::DsfSessionThrift> getDsfSessions(
    const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::vector<facebook::fboss::DsfSessionThrift> sessions;
  swAgentClient->sync_getDsfSessions(sessions);
  return sessions;
}

std::map<int64_t, cfg::DsfNode> getSwitchIdToDsfNode(
    const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::map<int64_t, cfg::DsfNode> switchIdToDsfNode;
  swAgentClient->sync_getDsfNodes(switchIdToDsfNode);
  return switchIdToDsfNode;
}

facebook::fboss::fsdb::SubscriberIdToOperSubscriberInfos
getSubscriberIdToOperSusbscriberInfos(const std::string& switchName) {
  auto fsdbClient = getFsdbThriftClient(switchName);
  facebook::fboss::fsdb::SubscriberIdToOperSubscriberInfos subInfos;
  fsdbClient->sync_getAllOperSubscriberInfos(subInfos);
  return subInfos;
}

void triggerGracefulAgentRestart(const std::string& switchName) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_gracefullyRestartService("wedge_agent_test");
  } catch (...) {
    // Thrift request may throw error as the Agent exits.
    // Ignore it, as we only wanted to trigger exit.
  }
}

void triggerUngracefulAgentRestart(const std::string& switchName) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_ungracefullyRestartService("wedge_agent_test");
  } catch (...) {
    // Thrift request may throw error as the Agent exits.
    // Ignore it, as we only wanted to trigger exit.
  }
}

void triggerGracefulQsfpRestart(const std::string& switchName) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_gracefullyRestartService("qsfp_service");
  } catch (...) {
    // Thrift request may throw error as the Agent exits.
    // Ignore it, as we only wanted to trigger exit.
  }
}

void triggerUngracefulQsfpRestart(const std::string& switchName) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_ungracefullyRestartService("qsfp_service");
  } catch (...) {
    // Thrift request may throw error as the Agent exits.
    // Ignore it, as we only wanted to trigger exit.
  }
}

void triggerGracefulFsdbRestart(const std::string& switchName) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_gracefullyRestartService("fsdb");
  } catch (...) {
    // Thrift request may throw error as the Agent exits.
    // Ignore it, as we only wanted to trigger exit.
  }
}

void triggerUngracefulFsdbRestart(const std::string& switchName) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_ungracefullyRestartService("fsdb");
  } catch (...) {
    // Thrift request may throw error as the Agent exits.
    // Ignore it, as we only wanted to trigger exit.
  }
}

} // namespace facebook::fboss::utility
