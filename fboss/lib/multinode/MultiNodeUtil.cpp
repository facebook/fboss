/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/multinode/MultiNodeUtil.h"

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "common/network/NetworkUtil.h"
#include "common/thrift/thrift/gen-cpp2/MonitorAsyncClient.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/if/gen-cpp2/TestCtrlAsyncClient.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_clients.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
using facebook::fboss::FabricEndpoint;
using facebook::fboss::FbossHwCtrl;
using facebook::fboss::MultiSwitchRunState;
using facebook::fboss::QsfpService;
using facebook::fboss::QsfpServiceRunState;
using facebook::fboss::TestCtrl;
using facebook::fboss::fsdb::FsdbService;
using facebook::fboss::fsdb::SubscriberIdToOperSubscriberInfos;
using RunForHwAgentFn = std::function<void(
    apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client)>;
using facebook::fboss::ClientID;
using facebook::fboss::IpPrefix;
using facebook::fboss::PortInfoThrift;
using facebook::fboss::RouteDetails;
using facebook::fboss::UnicastRoute;
using facebook::fboss::cfg::DsfNode;

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

std::unique_ptr<apache::thrift::Client<QsfpService>> getQsfpThriftClient(
    const std::string& switchName) {
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

std::unique_ptr<apache::thrift::Client<FsdbService>> getFsdbThriftClient(
    const std::string& switchName) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress fsdb(remoteSwitchIp, 5908);
  auto socket = folly::AsyncSocket::newSocket(eb, fsdb);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<FsdbService>>(
      std::move(channel));
}

MultiSwitchRunState getMultiSwitchRunState(const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  MultiSwitchRunState runState;
  swAgentClient->sync_getMultiSwitchRunState(runState);
  return runState;
}

QsfpServiceRunState getQsfpServiceRunState(const std::string& switchName) {
  auto qsfpClient = getQsfpThriftClient(switchName);
  return qsfpClient->sync_getQsfpServiceRunState();
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

void adminDisablePort(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setPortState(portID, false /* disable port */);
}

void adminEnablePort(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setPortState(portID, true /* enable port */);
}

void drainPort(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setPortDrainState(portID, true /* drain port */);
}

void undrainPort(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setPortDrainState(portID, false /* undrain port */);
}

void setSelfHealingLagEnable(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setSelfHealingLagState(portID, true /* enable SHEL */);
}

void setSelfHealingLagDisable(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setSelfHealingLagState(portID, false /* disable SHEL */);
}

void enableConditionalEntropyRehash(
    const std::string& switchName,
    int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setConditionalEntropyRehash(
      portID, true /* enable conditional entropy rehash */);
}

void disableConditionalEntropyRehash(
    const std::string& switchName,
    int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setConditionalEntropyRehash(
      portID, false /* disable conditional entropy rehash */);
}

std::map<int64_t, DsfNode> getSwitchIdToDsfNode(const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::map<int64_t, DsfNode> switchIdToDsfNode;
  swAgentClient->sync_getDsfNodes(switchIdToDsfNode);
  return switchIdToDsfNode;
}

std::vector<facebook::fboss::NdpEntryThrift> getNdpEntries(
    const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::vector<facebook::fboss::NdpEntryThrift> ndpEntries;
  swAgentClient->sync_getNdpTable(ndpEntries);
  return ndpEntries;
}

void addNeighbor(
    const std::string& switchName,
    const int32_t& interfaceID,
    const folly::IPAddress& neighborIP,
    const folly::MacAddress& macAddress,
    int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_addNeighbor(
      interfaceID,
      facebook::network::toBinaryAddress(neighborIP),
      macAddress.toString(),
      portID);
}

void removeNeighbor(
    const std::string& switchName,
    const int32_t& interfaceID,
    const folly::IPAddress& neighborIP) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_flushNeighborEntry(
      facebook::network::toBinaryAddress(neighborIP), interfaceID);
}

std::vector<RouteDetails> getAllRoutes(const std::string& switchName) {
  std::vector<RouteDetails> routes;
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_getRouteTableDetails(routes);
  return routes;
}

void addRoute(
    const std::string& switchName,
    const folly::IPAddress& destPrefix,
    const int16_t prefixLength,
    const std::vector<folly::IPAddress>& nexthops) {
  UnicastRoute route;
  route.dest()->ip() = facebook::network::toBinaryAddress(destPrefix);
  route.dest()->prefixLength() = prefixLength;
  for (const auto& nexthop : nexthops) {
    route.nextHopAddrs()->push_back(
        facebook::network::toBinaryAddress(nexthop));
  }

  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_addUnicastRoutes(
      static_cast<int16_t>(ClientID::STATIC_ROUTE), {std::move(route)});
}

std::map<int32_t, facebook::fboss::InterfaceDetail> getIntfIdToIntf(
    const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  std::map<int32_t, facebook::fboss::InterfaceDetail> intfIdToIntf;
  swAgentClient->sync_getAllInterfaces(intfIdToIntf);
  return intfIdToIntf;
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

std::map<std::string, int64_t> getCounterNameToCount(
    const std::string& switchName) {
  std::map<std::string, int64_t> counterNameToCount;

  auto multiSwitchRunState = getMultiSwitchRunState(switchName);
  // For split binary (multiSwitchEnabled), the counters are available in the
  // HwAgent. For monolithic, the counters are available in the SwAgent.
  if (*multiSwitchRunState.multiSwitchEnabled()) {
    auto hwAgentQueryFn =
        [&counterNameToCount](apache::thrift::Client<FbossHwCtrl>& client) {
          std::map<std::string, int64_t> hwAgentCounters;
          apache::thrift::Client<facebook::thrift::Monitor> monitoringClient{
              client.getChannelShared()};
          monitoringClient.sync_getCounters(hwAgentCounters);
          counterNameToCount.merge(hwAgentCounters);
        };
    runOnAllHwAgents(switchName, hwAgentQueryFn);
  } else {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_getCounters(counterNameToCount);
  }

  return counterNameToCount;
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

void restartAgentWithDelay(
    const std::string& switchName,
    int32_t delayInSeconds) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_gracefullyRestartServiceWithDelay(
        "wedge_agent_test", delayInSeconds);
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

SubscriberIdToOperSubscriberInfos getSubscriberIdToOperSusbscriberInfos(
    const std::string& switchName) {
  auto fsdbClient = getFsdbThriftClient(switchName);
  SubscriberIdToOperSubscriberInfos subInfos;
  fsdbClient->sync_getAllOperSubscriberInfos(subInfos);
  return subInfos;
}

// Invoke the provided func on every element of given set.
// except on exclusions.
template <typename Callable, typename... Args>
void forEachExcluding(
    const std::set<std::string>& inputSet,
    const std::set<std::string>& exclusions,
    Callable&& func,
    Args&&... args) {
  // Store arguments in a tuple for safe repeated use
  auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
  for (const auto& elem : inputSet) {
    if (exclusions.find(elem) == exclusions.end()) {
      std::apply(
          [&](auto&&... unpackedArgs) {
            func(elem, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
          },
          argsTuple);
    }
  }
}

// Invoke the provided func on every element of given set.
// except on exclusions.
// Return true if and only if every func returns true.
// false otherwise. Stops on first failure.
template <typename Callable, typename... Args>
bool checkForEachExcluding(
    const std::set<std::string>& inputSet,
    const std::set<std::string>& exclusions,
    Callable&& func,
    Args&&... args) {
  // Store arguments in a tuple for safe repeated use
  auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
  for (const auto& elem : inputSet) {
    if (exclusions.find(elem) == exclusions.end()) {
      bool result = std::apply(
          [&](auto&&... unpackedArgs) {
            return func(
                elem, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
          },
          argsTuple);
      if (!result) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

namespace facebook::fboss::utility {

MultiNodeUtil::MultiNodeUtil(
    SwSwitch* sw,
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap)
    : sw_(sw) {
  populateDsfNodes(dsfNodeMap);
  populateAllRdsws();
  populateAllFdsws();
  populateAllSwitches();
}

void MultiNodeUtil::populateDsfNodes(
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap) {
  for (const auto& [_, dsfNodes] : std::as_const(*dsfNodeMap)) {
    for (const auto& [_, node] : std::as_const(*dsfNodes)) {
      switchIdToSwitchName_[node->getSwitchId()] = node->getName();
      switchNameToSwitchIds_[node->getName()].insert(node->getSwitchId());
      switchNameToAsicType_[node->getName()] = node->getAsicType();

      if (node->getType() == cfg::DsfNodeType::INTERFACE_NODE) {
        CHECK(node->getClusterId().has_value());
        auto clusterId = node->getClusterId().value();
        clusterIdToRdsws_[clusterId].push_back(node->getName());
      } else if (node->getType() == cfg::DsfNodeType::FABRIC_NODE) {
        CHECK(node->getFabricLevel().has_value());
        if (node->getFabricLevel().value() == 1) {
          CHECK(node->getClusterId().has_value());
          auto clusterId = node->getClusterId().value();
          clusterIdToFdsws_[clusterId].push_back(node->getName());
        } else if (node->getFabricLevel().value() == 2) {
          sdsws_.insert(node->getName());
        } else {
          XLOG(FATAL) << "Invalid fabric level"
                      << node->getFabricLevel().value();
        }
      } else {
        XLOG(FATAL) << "Invalid DSF Node type"
                    << apache::thrift::util::enumNameSafe(node->getType());
      }
    }
  }
}

void MultiNodeUtil::populateAllRdsws() {
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : rdsws) {
      allRdsws_.insert(rdsw);
    }
  }
}

void MultiNodeUtil::populateAllFdsws() {
  for (const auto& [clusterId, fdsws] : std::as_const(clusterIdToFdsws_)) {
    for (const auto& fdsw : fdsws) {
      allFdsws_.insert(fdsw);
    }
  }
}

void MultiNodeUtil::populateAllSwitches() {
  allSwitches_.insert(allRdsws_.begin(), allRdsws_.end());
  allSwitches_.insert(allFdsws_.begin(), allFdsws_.end());
  allSwitches_.insert(sdsws_.begin(), sdsws_.end());
}

std::map<std::string, FabricEndpoint>
MultiNodeUtil::getConnectedFabricPortToFabricEndpoint(
    const std::string& switchName) const {
  std::map<std::string, FabricEndpoint> connectedFabricPortToFabricEndpoint;
  auto fabricPortToFabricEndpoint = getFabricPortToFabricEndpoint(switchName);

  std::copy_if(
      fabricPortToFabricEndpoint.begin(),
      fabricPortToFabricEndpoint.end(),
      std::inserter(
          connectedFabricPortToFabricEndpoint,
          connectedFabricPortToFabricEndpoint.begin()),
      [](const auto& pair) {
        const auto& fabricEndpoint = pair.second;
        return fabricEndpoint.isAttached().value();
      });

  return connectedFabricPortToFabricEndpoint;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesHelper(
    SwitchType switchType,
    const std::string& switchToVerify,
    const std::set<std::string>& expectedConnectedSwitches) const {
  auto logFabricEndpoint = [switchToVerify](
                               const FabricEndpoint& fabricEndpoint) {
    XLOG(DBG2) << "From " << " switchName: " << switchToVerify
               << " actualPeerSwitchId: " << fabricEndpoint.switchId().value()
               << " actualPeerSwitchName: "
               << fabricEndpoint.switchName().value_or("none")
               << " actualPeerPortId: " << fabricEndpoint.portId().value()
               << " actualPortName: "
               << fabricEndpoint.portName().value_or("none")
               << " expectedPeerSwitchId: "
               << fabricEndpoint.expectedSwitchId().value_or(-1)
               << " expectedPeerSwitchName: "
               << fabricEndpoint.expectedSwitchName().value_or("none")
               << " expectedPeerPortId: "
               << fabricEndpoint.expectedPortId().value_or(-1)
               << " expectedPeerPortName: "
               << fabricEndpoint.expectedPortName().value_or("none");
  };

  std::set<std::string> gotConnectedSwitches;
  for (const auto& [portName, fabricEndpoint] :
       getFabricPortToFabricEndpoint(switchToVerify)) {
    if (fabricEndpoint.isAttached().value()) {
      logFabricEndpoint(fabricEndpoint);

      auto actualRemoteSwitchId = fabricEndpoint.switchId().value();
      auto expectedRemoteSwitchId =
          fabricEndpoint.expectedSwitchId().value_or(-1);
      auto actualRemoteSwitchName = fabricEndpoint.switchName();
      auto expectedRemoteSwitchName = fabricEndpoint.expectedSwitchName();

      auto actualRemotePortId = fabricEndpoint.portId().value();
      auto expectedRemotePortId = fabricEndpoint.expectedPortId().value_or(-1);
      auto actualRemotePortName = fabricEndpoint.portName();
      auto expectedRemotePortName = fabricEndpoint.portName();

      // Expected switch/port ID/name must match for every entry
      if (!(expectedRemoteSwitchId == actualRemoteSwitchId &&
            expectedRemoteSwitchName == actualRemoteSwitchName &&
            expectedRemotePortId == actualRemotePortId &&
            expectedRemotePortName == actualRemotePortName)) {
        return false;
      }
      if (fabricEndpoint.switchName().has_value()) {
        gotConnectedSwitches.insert(fabricEndpoint.switchName().value());
      }
    }
  }

  XLOG(DBG2) << "From " << switchTypeToString(switchType)
             << ":: " << switchToVerify << " Expected Connected Switches: "
             << folly::join(",", expectedConnectedSwitches)
             << " Got Connected Switches: "
             << folly::join(",", gotConnectedSwitches);

  return expectedConnectedSwitches == gotConnectedSwitches;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForRdsw(
    int clusterId,
    const std::string& rdswToVerify) const {
  // Every RDSW is connected to all FDSWs in its cluster
  std::set<std::string> expectedConnectedSwitches(
      std::begin(clusterIdToFdsws_.at(clusterId)),
      std::end(clusterIdToFdsws_.at(clusterId)));

  return verifyFabricConnectedSwitchesHelper(
      SwitchType::RDSW, rdswToVerify, expectedConnectedSwitches);
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForAllRdsws() const {
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : rdsws) {
      if (!verifyFabricConnectedSwitchesForRdsw(clusterId, rdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForFdsw(
    int clusterId,
    const std::string& fdswToVerify) const {
  // Every FDSW is connected to all RDSWs in its cluster and all SDSWs
  std::set<std::string> expectedConnectedSwitches(
      std::begin(clusterIdToRdsws_.at(clusterId)),
      std::end(clusterIdToRdsws_.at(clusterId)));
  expectedConnectedSwitches.insert(sdsws_.begin(), sdsws_.end());

  return verifyFabricConnectedSwitchesHelper(
      SwitchType::FDSW, fdswToVerify, expectedConnectedSwitches);
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForAllFdsws() const {
  for (const auto& [clusterId, fdsws] : std::as_const(clusterIdToFdsws_)) {
    for (const auto& fdsw : fdsws) {
      if (!verifyFabricConnectedSwitchesForFdsw(clusterId, fdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForSdsw(
    const std::string& sdswToVerify) const {
  // Every SDSW is connected to all FDSWs in all clusters

  return verifyFabricConnectedSwitchesHelper(
      SwitchType::SDSW,
      sdswToVerify,
      allFdsws_ /* expectedConnectedSwitches */);
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForAllSdsws() const {
  for (const auto& sdsw : sdsws_) {
    if (!verifyFabricConnectedSwitchesForSdsw(sdsw)) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricConnectivity() const {
  return verifyFabricConnectedSwitchesForAllRdsws() &&
      verifyFabricConnectedSwitchesForAllFdsws() &&
      verifyFabricConnectedSwitchesForAllSdsws();
}

bool MultiNodeUtil::verifyFabricReachablityForRdsw(
    const std::string& rdswToVerify) const {
  auto logReachability = [rdswToVerify](
                             const std::string& remoteSwitchName,
                             const std::vector<std::string>& reachablePorts) {
    XLOG(DBG2) << "From " << rdswToVerify
               << " remoteSwitchName: " << remoteSwitchName
               << " reachablePorts: " << folly::join(",", reachablePorts);
  };

  // Every remote RDSW across all clusters is reachable from the local RDSW
  std::vector<std::string> remoteSwitchNames;
  std::copy_if(
      allRdsws_.begin(),
      allRdsws_.end(),
      std::back_inserter(remoteSwitchNames),
      [rdswToVerify](const std::string& switchName) {
        return switchName != rdswToVerify; // exclude self
      });

  auto swAgentClient = getSwAgentThriftClient(rdswToVerify);
  std::map<std::string, std::vector<std::string>> reachability;
  swAgentClient->sync_getSwitchReachability(reachability, remoteSwitchNames);

  // Every remote RDSW must be reachable via every local active port.
  // TODO: This assertion is not true when Input Balanced Mode is enabled.
  // We will enhance this check for DSF Dual Stage where Input Balanced Mode
  // is enabled.
  auto activePorts = getActiveFabricPorts(rdswToVerify);

  for (auto& [remoteSwitchName, reachablePorts] : reachability) {
    logReachability(remoteSwitchName, reachablePorts);

    std::set<std::string> reachablePortsSet(
        reachablePorts.begin(), reachablePorts.end());
    XLOG(DBG2) << "From RDSW:: " << rdswToVerify
               << " Expected reachable ports (Active Ports): "
               << folly::join(",", activePorts) << " Got reachable ports: "
               << folly::join(",", reachablePortsSet);

    if (activePorts != reachablePortsSet) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricReachability() const {
  for (const auto& rdsw : allRdsws_) {
    if (!verifyFabricReachablityForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::set<std::string> MultiNodeUtil::getActiveFabricPorts(
    const std::string& switchName) const {
  auto activeFabricPortNameToPortInfo =
      getActiveFabricPortNameToPortInfo(switchName);
  std::set<std::string> activePorts;

  std::transform(
      activeFabricPortNameToPortInfo.begin(),
      activeFabricPortNameToPortInfo.end(),
      std::inserter(activePorts, activePorts.begin()),
      [](const auto& pair) {
        const auto& portInfo = pair.second;
        return portInfo.name().value();
      });

  return activePorts;
}

std::map<std::string, PortInfoThrift>
MultiNodeUtil::getActiveFabricPortNameToPortInfo(
    const std::string& switchName) const {
  std::map<std::string, PortInfoThrift> activeFabricPortNameToPortInfo;
  for (const auto& [_, portInfo] : getFabricPortNameToPortInfo(switchName)) {
    if (portInfo.activeState().has_value() &&
        portInfo.activeState().value() == PortActiveState::ACTIVE) {
      activeFabricPortNameToPortInfo.emplace(portInfo.name().value(), portInfo);
    }
  }

  return activeFabricPortNameToPortInfo;
}

std::map<std::string, PortInfoThrift>
MultiNodeUtil::getFabricPortNameToPortInfo(
    const std::string& switchName) const {
  std::map<std::string, PortInfoThrift> fabricPortNameToPortInfo;
  for (const auto& [_, portInfo] : getPortIdToPortInfo(switchName)) {
    if (portInfo.portType().value() == cfg::PortType::FABRIC_PORT) {
      fabricPortNameToPortInfo.emplace(portInfo.name().value(), portInfo);
    }
  }

  return fabricPortNameToPortInfo;
}

std::map<std::string, PortInfoThrift>
MultiNodeUtil::getUpEthernetPortNameToPortInfo(
    const std::string& switchName) const {
  std::map<std::string, PortInfoThrift> upEthernetPortNameToPortInfo;
  for (const auto& [_, portInfo] : getPortIdToPortInfo(switchName)) {
    if (portInfo.portType().value() == cfg::PortType::INTERFACE_PORT &&
        portInfo.operState().value() == PortOperState::UP) {
      upEthernetPortNameToPortInfo.emplace(portInfo.name().value(), portInfo);
    }
  }

  return upEthernetPortNameToPortInfo;
}

bool MultiNodeUtil::verifyPortActiveStateForSwitch(
    SwitchType switchType,
    const std::string& switchName) const {
  // Every Connected Fabric Port must be Active
  auto connectedFabricPortToFabricEndpoint =
      getConnectedFabricPortToFabricEndpoint(switchName);
  std::set<std::string> expectedActivePorts;
  std::transform(
      connectedFabricPortToFabricEndpoint.begin(),
      connectedFabricPortToFabricEndpoint.end(),
      std::inserter(expectedActivePorts, expectedActivePorts.begin()),
      [](const auto& pair) { return pair.first; });

  auto gotActivePorts = getActiveFabricPorts(switchName);

  XLOG(DBG2) << "From " << switchTypeToString(switchType) << ":: " << switchName
             << " Expected Active Ports (connected fabric ports): "
             << folly::join(",", expectedActivePorts)
             << " Got Active Ports: " << folly::join(",", gotActivePorts);

  return expectedActivePorts == gotActivePorts;
}

bool MultiNodeUtil::verifyNoPortErrorsForSwitch(
    SwitchType switchType,
    const std::string& switchName) const {
  // No ports should have errors
  for (const auto& [_, portInfo] : getPortIdToPortInfo(switchName)) {
    if (portInfo.activeErrors()->size() != 0) {
      XLOG(DBG2) << "From " << switchTypeToString(switchType)
                 << ":: " << switchName << " Port: " << portInfo.name().value()
                 << " has errors: "
                 << folly::join(",", *portInfo.activeErrors());
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyPortCableLength(
    SwitchType switchType,
    const std::string& switchName) const {
  // Cable length query (i.e. cable propagation delay attribute) works only on
  // below fabric ports::
  // DSF Single Stage system:
  //   o RDSW Fabric ports towards FDSW
  // DSF Dual Stage system:
  //   o RDSW Fabric ports towards FDSW
  //   o FDSW Fabric ports towards SDSW
  //
  // TODO Enhance this check for DSF Dual stage: FDSW Fabric ports towards SDSW
  if (switchType != SwitchType::RDSW) {
    return true;
  }

  // Verify if all connected fabric ports have valid cable length
  for (const auto& [_, portInfo] :
       getActiveFabricPortNameToPortInfo(switchName)) {
    if (portInfo.cableLengthMeters().has_value() &&
        portInfo.cableLengthMeters().value() >= 0) {
      // Cable length may vary on different test setups. Thus, verify
      // if the Cable length query returns a valid non-0 cable length.
      continue;
    }

    XLOG(DBG2) << "From " << switchTypeToString(switchType)
               << ":: " << switchName << " Port: " << portInfo.name().value()
               << " has invalid cable length: "
               << portInfo.cableLengthMeters().value_or(-1);
    return false;
  }

  return true;
}

bool MultiNodeUtil::verifyPortsForSwitch(
    SwitchType switchType,
    const std::string& switchName) const {
  return verifyPortActiveStateForSwitch(switchType, switchName) &&
      verifyNoPortErrorsForSwitch(switchType, switchName) &&
      verifyPortCableLength(switchType, switchName);
}

bool MultiNodeUtil::verifyPorts() const {
  // The checks are identical for all Switch types at the moment
  // as we only verify Fabric ports. We may add Ethernet port checks
  // specific to RDSWs in the future.
  for (const auto& rdsw : allRdsws_) {
    if (!verifyPortsForSwitch(SwitchType::RDSW, rdsw)) {
      return false;
    }
  }

  for (const auto& fdsw : allFdsws_) {
    if (!verifyPortsForSwitch(SwitchType::FDSW, fdsw)) {
      return false;
    }
  }

  for (const auto& sdsw : sdsws_) {
    if (!verifyPortsForSwitch(SwitchType::SDSW, sdsw)) {
      return false;
    }
  }

  return true;
}

std::map<std::string, std::vector<SystemPortThrift>>
MultiNodeUtil::getRdswToSystemPorts() const {
  std::map<std::string, std::vector<SystemPortThrift>> rdswToSystemPorts;

  for (const auto& rdsw : allRdsws_) {
    auto systemPortIdToSystemPort = getSystemPortdIdToSystemPort(rdsw);

    std::transform(
        systemPortIdToSystemPort.begin(),
        systemPortIdToSystemPort.end(),
        std::back_inserter(rdswToSystemPorts[rdsw]),
        [](const auto& pair) { return pair.second; });
  }

  return rdswToSystemPorts;
}

std::set<std::string> MultiNodeUtil::getGlobalSystemPortsOfType(
    const std::string& rdsw,
    const std::set<RemoteSystemPortType>& types) const {
  auto matchesSystemPortType =
      [&types](const facebook::fboss::SystemPortThrift& systemPort) {
        if (systemPort.remoteSystemPortType().has_value()) {
          return types.find(systemPort.remoteSystemPortType().value()) !=
              types.end();
        } else {
          return types.empty();
        }
      };

  std::set<std::string> systemPortsOfType;
  for (const auto& [_, systemPort] : getSystemPortdIdToSystemPort(rdsw)) {
    if (*systemPort.scope() == cfg::Scope::GLOBAL &&
        matchesSystemPortType(systemPort)) {
      systemPortsOfType.insert(systemPort.portName().value());
    }
  }
  return systemPortsOfType;
}

bool MultiNodeUtil::verifySystemPortsForRdsw(
    const std::string& rdswToVerify) const {
  // Every GLOBAL system port of every remote RDSW is either a STATIC_ENTRY
  // or DYNAMIC_ENTRY of the local RDSW
  std::set<std::string> gotSystemPorts;
  std::set<std::string> expectedSystemPorts;
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == rdswToVerify) {
        gotSystemPorts = getGlobalSystemPortsOfType(
            rdswToVerify,
            {RemoteSystemPortType::STATIC_ENTRY,
             RemoteSystemPortType::
                 DYNAMIC_ENTRY} /* Get Global Remote ports of self */);
      } else {
        auto systemPortsForRdsw = getGlobalSystemPortsOfType(
            rdsw, {} /* Get Global ports of Remote RDSW */);
        expectedSystemPorts.insert(
            systemPortsForRdsw.begin(), systemPortsForRdsw.end());
      }
    }
  }

  XLOG(DBG2) << "From " << rdswToVerify << " Expected System Ports: "
             << folly::join(",", expectedSystemPorts)
             << " Got System Ports: " << folly::join(",", gotSystemPorts);

  return expectedSystemPorts == gotSystemPorts;
}

bool MultiNodeUtil::verifySystemPorts() const {
  for (const auto& rdsw : allRdsws_) {
    if (!verifySystemPortsForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::map<std::string, std::vector<InterfaceDetail>>
MultiNodeUtil::getRdswToRifs() const {
  std::map<std::string, std::vector<InterfaceDetail>> rdswToRifs;
  for (const auto& rdsw : allRdsws_) {
    auto intfIdToIntf = getIntfIdToIntf(rdsw);

    std::transform(
        intfIdToIntf.begin(),
        intfIdToIntf.end(),
        std::back_inserter(rdswToRifs[rdsw]),
        [](const auto& pair) { return pair.second; });
  }

  return rdswToRifs;
}

std::set<int> MultiNodeUtil::getGlobalRifsOfType(
    const std::string& rdsw,
    const std::set<RemoteInterfaceType>& types) const {
  auto matchesRifType = [&types](const facebook::fboss::InterfaceDetail& rif) {
    if (rif.remoteIntfType().has_value()) {
      return types.find(rif.remoteIntfType().value()) != types.end();
    } else {
      return types.empty();
    }
  };

  std::set<int> rifsOfType;
  for (const auto& [_, rif] : getIntfIdToIntf(rdsw)) {
    if (*rif.scope() == cfg::Scope::GLOBAL && matchesRifType(rif)) {
      rifsOfType.insert(rif.interfaceId().value());
    }
  }
  return rifsOfType;
}

bool MultiNodeUtil::verifyRifsForRdsw(const std::string& rdswToVerify) const {
  // Every GLOBAL rif of every remote RDSW is either a STATIC_ENTRY or
  // DYNAMIC_ENTRY of the local RDSW
  std::set<int> gotRifs;
  std::set<int> expectedRifs;
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == rdswToVerify) {
        gotRifs = getGlobalRifsOfType(
            rdswToVerify,
            {RemoteInterfaceType::STATIC_ENTRY,
             RemoteInterfaceType::
                 DYNAMIC_ENTRY} /* Get Global Remote rifs of self */);
      } else {
        auto rifsForRdsw =
            getGlobalRifsOfType(rdsw, {} /* Get Global rifs of Remote RDSW */);
        expectedRifs.insert(rifsForRdsw.begin(), rifsForRdsw.end());
      }
    }
  }

  XLOG(DBG2) << "From " << rdswToVerify
             << " Expected Rifs: " << folly::join(",", expectedRifs)
             << " Got Rifs: " << folly::join(",", gotRifs);

  return expectedRifs == gotRifs;
}

bool MultiNodeUtil::verifyRifs() const {
  for (const auto& rdsw : allRdsws_) {
    if (!verifyRifsForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::vector<NdpEntryThrift> MultiNodeUtil::getNdpEntriesOfType(
    const std::string& rdsw,
    const std::set<std::string>& types) const {
  auto ndpEntries = getNdpEntries(rdsw);

  std::vector<NdpEntryThrift> filteredNdpEntries;
  std::copy_if(
      ndpEntries.begin(),
      ndpEntries.end(),
      std::back_inserter(filteredNdpEntries),
      [this, rdsw, &types](const facebook::fboss::NdpEntryThrift& ndpEntry) {
        logNdpEntry(rdsw, ndpEntry);
        return types.find(ndpEntry.state().value()) != types.end();
      });

  return filteredNdpEntries;
}

bool MultiNodeUtil::verifyStaticNdpEntries() const {
  // Every remote RDSW must have a STATIC NDP entry in the local RDSW
  auto expectedRdsws = allRdsws_;

  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      auto staticNdpEntries = getNdpEntriesOfType(rdsw, {"STATIC"});

      std::set<std::string> gotRdsws;
      std::transform(
          staticNdpEntries.begin(),
          staticNdpEntries.end(),
          std::inserter(gotRdsws, gotRdsws.begin()),
          [this](const auto& ndpEntry) {
            CHECK(ndpEntry.switchId().has_value());
            CHECK(
                switchIdToSwitchName_.find(
                    SwitchID(ndpEntry.switchId().value())) !=
                std::end(switchIdToSwitchName_));

            return switchIdToSwitchName_.at(
                SwitchID(ndpEntry.switchId().value()));
          });

      if (expectedRdsws != gotRdsws) {
        XLOG(DBG2) << "STATIC NDP Entries from " << rdsw
                   << " Expected RDSWs: " << folly::join(",", expectedRdsws)
                   << " Got RDSWs: " << folly::join(",", gotRdsws);
        return false;
      }
    }
  }

  return true;
}

std::map<std::string, DsfSessionThrift> MultiNodeUtil::getPeerToDsfSession(
    const std::string& rdsw) const {
  auto logDsfSession =
      [rdsw](const facebook::fboss::DsfSessionThrift& session) {
        XLOG(DBG2) << "From " << rdsw << " session: " << *session.remoteName()
                   << " state: "
                   << apache::thrift::util::enumNameSafe(*session.state())
                   << " lastEstablishedAt: "
                   << session.lastEstablishedAt().value_or(0)
                   << " lastDisconnectedAt: "
                   << session.lastDisconnectedAt().value_or(0);
      };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::vector<facebook::fboss::DsfSessionThrift> sessions;
  swAgentClient->sync_getDsfSessions(sessions);

  std::map<std::string, DsfSessionThrift> peerToDsfSession;
  for (const auto& session : sessions) {
    logDsfSession(session);
    // remoteName format: peerName::peerIP, extract peerName.
    size_t pos = (*session.remoteName()).find("::");
    if (pos != std::string::npos) {
      auto peer = (*session.remoteName()).substr(0, pos);
      peerToDsfSession.emplace(peer, session);
    }
  }

  return peerToDsfSession;
}

std::set<std::string> MultiNodeUtil::getRdswsWithEstablishedDsfSessions(
    const std::string& rdsw) const {
  std::set<std::string> gotRdsws;
  for (const auto& [peer, session] : getPeerToDsfSession(rdsw)) {
    if (session.state() == facebook::fboss::DsfSessionState::ESTABLISHED) {
      gotRdsws.insert(peer);
    }
  }

  return gotRdsws;
}

bool MultiNodeUtil::verifyDsfSessions() const {
  // Every RDSW must have an ESTABLISHED DSF Session with every other RDSW

  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      auto expectedRdsws = allRdsws_;
      expectedRdsws.erase(rdsw); // exclude self
      auto gotRdsws = getRdswsWithEstablishedDsfSessions(rdsw);
      if (expectedRdsws != gotRdsws) {
        XLOG(DBG2) << "DSF ESTABLISHED sessions from " << rdsw
                   << " Expected RDSWs: " << folly::join(",", expectedRdsws)
                   << " Got RDSWs: " << folly::join(",", gotRdsws);
        return false;
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyNoSessionsFlap(
    const std::string& rdswToVerify,
    const std::map<std::string, DsfSessionThrift>& baselinePeerToDsfSession,
    const std::optional<std::string>& rdswToExclude) const {
  auto noSessionFlap =
      [this, rdswToVerify, baselinePeerToDsfSession, rdswToExclude]() -> bool {
    auto currentPeerToDsfSession = getPeerToDsfSession(rdswToVerify);
    // All entries must be identical i.e.
    // DSF Session state (ESTABLISHED or not) is the same.
    // For any session the establishedAt and connnectedAt is the same.
    if (rdswToExclude.has_value()) {
      currentPeerToDsfSession.erase(rdswToExclude.value());
    }

    return baselinePeerToDsfSession == currentPeerToDsfSession;
  };

  // It may take several (> 15) seconds for ESTABLISHED => CONNECT.
  // Thus, keep retrying for several seconds to ensure that the session
  // stays ESTABLISHED.
  return checkAlwaysTrueWithRetryErrorReturn(
      noSessionFlap, 30 /* num retries */);
}

bool MultiNodeUtil::verifyNoSessionsEstablished(
    const std::string& rdswToVerify) const {
  auto noSessionsEstablished = [this, rdswToVerify]() -> bool {
    for (const auto& [peer, session] : getPeerToDsfSession(rdswToVerify)) {
      if (session.state() == facebook::fboss::DsfSessionState::ESTABLISHED) {
        return false;
      }
    }

    return true;
  };

  // It may take several (> 15) seconds for ESTABLISHED => CONNECT. Thus,
  // try for several seconds and check if the session transitions to
  // CONNECT.
  return checkWithRetryErrorReturn(noSessionsEstablished, 30 /* num retries */);
}

bool MultiNodeUtil::verifyAllSessionsEstablished(
    const std::string& rdswToVerify) const {
  auto allSessionsEstablished = [this, rdswToVerify]() -> bool {
    for (const auto& [peer, session] : getPeerToDsfSession(rdswToVerify)) {
      if (session.state() != facebook::fboss::DsfSessionState::ESTABLISHED) {
        return false;
      }
    }

    return true;
  };

  // It may take several seconds(>15) for ESTABLISHED => CONNECT. Thus,
  // try for several seconds and check if the session transitions to
  // ESTABLISHED.
  return checkWithRetryErrorReturn(
      allSessionsEstablished, 30 /* num retries */);
}

bool MultiNodeUtil::verifyGracefulFabricLinkDown(
    const std::string& rdswToVerify,
    const std::map<std::string, PortInfoThrift>& activeFabricPortNameToPortInfo)
    const {
  CHECK(activeFabricPortNameToPortInfo.size() > 2);
  auto rIter = activeFabricPortNameToPortInfo.rbegin();
  auto lastActivePort = rIter->first;
  auto secondLastActivePort = std::next(rIter)->first;
  auto baselinePeerToDsfSession = getPeerToDsfSession(rdswToVerify);

  // Admin disable all Active fabric ports
  // Verify that all sessions stay established till last port is disabled.
  // TODO: modify the config to set minLink threshold, and then extend the
  // logic to verify that the sessions stay established while the minLink
  // requirement is met.
  for (const auto& [portName, portInfo] : activeFabricPortNameToPortInfo) {
    XLOG(DBG2) << __func__
               << " Admin disabling port:: " << portInfo.name().value()
               << " portID: " << portInfo.portId().value();
    adminDisablePort(rdswToVerify, portInfo.portId().value());

    bool checkPassed = true;
    if (portName == lastActivePort) {
      checkPassed = verifyNoSessionsEstablished(rdswToVerify);
    } else if (portName == secondLastActivePort) {
      // verify no flaps is expensive.
      // Thus, only verify just before disabling the last port.
      // There is no loss of signal due to this approach as if the sessions
      // flap due to an intermediate port admin disable, it will be detected
      // by this check failure anyway.
      checkPassed =
          verifyNoSessionsFlap(rdswToVerify, baselinePeerToDsfSession);
    }
    if (!checkPassed) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulFabricLinkUp(
    const std::string& rdswToVerify,
    const std::map<std::string, PortInfoThrift>& activeFabricPortNameToPortInfo)
    const {
  CHECK(activeFabricPortNameToPortInfo.size() > 2);
  auto firstActivePort = activeFabricPortNameToPortInfo.begin()->first;
  auto rIter = activeFabricPortNameToPortInfo.rbegin();
  auto lastActivePort = rIter->first;
  // Admin Re-enable these fabric ports
  for (const auto& [portName, portInfo] : activeFabricPortNameToPortInfo) {
    XLOG(DBG2) << __func__
               << " Admin enabling port:: " << portInfo.name().value()
               << " portID: " << portInfo.portId().value();
    adminEnablePort(rdswToVerify, portInfo.portId().value());

    bool checkPassed = true;
    if (portName == firstActivePort || portName == lastActivePort) {
      checkPassed = verifyAllSessionsEstablished(rdswToVerify);
    }

    if (!checkPassed) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulFabricLinkDownUp() const {
  auto myHostname = getLocalHostname();
  auto activeFabricPortNameToPortInfo =
      getActiveFabricPortNameToPortInfo(myHostname);

  if (!verifyGracefulFabricLinkDown(
          myHostname, activeFabricPortNameToPortInfo)) {
    return false;
  }

  if (!verifyGracefulFabricLinkUp(myHostname, activeFabricPortNameToPortInfo)) {
    return false;
  }

  return true;
}

bool MultiNodeUtil::verifySwSwitchRunState(
    const std::string& rdswToVerify,
    const SwitchRunState& expectedSwitchRunState) const {
  auto switchRunStateMatches =
      [this, rdswToVerify, expectedSwitchRunState]() -> bool {
    auto multiSwitchRunState = getMultiSwitchRunState(rdswToVerify);
    auto gotSwitchRunState = multiSwitchRunState.swSwitchRunState();
    return gotSwitchRunState == expectedSwitchRunState;
  };

  // Thrift client queries will throw exception while the Agent is
  // initializing. Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      switchRunStateMatches,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyQsfpServiceRunState(
    const std::string& rdswToVerify,
    const QsfpServiceRunState& expectedQsfpRunState) const {
  auto qsfpServiceRunStateMatches = [rdswToVerify,
                                     expectedQsfpRunState]() -> bool {
    auto gotQsfpServiceRunState = getQsfpServiceRunState(rdswToVerify);
    return gotQsfpServiceRunState == expectedQsfpRunState;
  };

  // Thrift client queries will throw exception while QSFP is initializing.
  // Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      qsfpServiceRunStateMatches,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyFsdbIsUp(const std::string& rdswToVerify) const {
  auto fsdbIsUp = [this, rdswToVerify]() -> bool {
    // Unlike Agent and QSFP, FSDB lacks notion of "RunState" that can be
    // queried to confirm whether FSDB has completed initialization.
    // In the meanwhile, query some FSDB thrift method.
    // If FSDB has not up yet, this will throw an error and
    // checkwithRetryErrorReturn will retry the specified number of times.
    // TODO: T238268316 will add "RunState" to FSDB. Leverage it then.
    getSubscriberIdToOperSusbscriberInfos(rdswToVerify);
    return true;
  };

  // Thrift client queries will throw exception while FSDB is initializing.
  // Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      fsdbIsUp,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyDeviceDownUpForRemoteRdswsHelper(
    bool triggerGraceFulRestart) const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  auto baselinePeerToDsfSession = getPeerToDsfSession(myHostname);

  // For any one RDSW in every remote cluster issue Agent restart
  for (const auto& [_, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == myHostname) { // exclude self
        continue;
      }

      // Trigger graceful or ungraceful Agent restart
      triggerGraceFulRestart ? triggerGracefulAgentRestart(rdsw)
                             : triggerUngracefulAgentRestart(rdsw);

      // Wait for the switch to come up
      if (!verifySwSwitchRunState(rdsw, SwitchRunState::CONFIGURED)) {
        XLOG(DBG2) << "Agent failed to come up post warmboot: " << rdsw;
        return false;
      }

      // Sessions to RDSW that was just restarted are expected to flap.
      // This is regardless of graceful or ungraceful Agent restart.
      // Verify no other sessions flap.
      auto expectedPeerToDsfSession = baselinePeerToDsfSession;
      expectedPeerToDsfSession.erase(rdsw);
      if (!verifyNoSessionsFlap(
              myHostname /* rdswToVerify */,
              expectedPeerToDsfSession,
              rdsw /* rdswToExclude */)) {
        return false;
      }

      // Verify all DSF sessions are established for the RDSW that was
      // restarted
      verifyAllSessionsEstablished(rdsw);

      // Restart only one remote RDSW per cluster
      break;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulDeviceDownUpForRemoteRdsws() const {
  return verifyDeviceDownUpForRemoteRdswsHelper(
      true /* triggerGracefulRestart*/);
}

bool MultiNodeUtil::verifyGracefulDeviceDownUpForRemoteFdsws() const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  auto baselinePeerToDsfSession = getPeerToDsfSession(myHostname);

  // For any one FDSW in every remote cluster issue graceful restart
  for (const auto& [_, fdsws] : std::as_const(clusterIdToFdsws_)) {
    // Gracefully restart only one remote FDSW per cluster
    if (!fdsws.empty()) {
      auto fdsw = fdsws.front();
      triggerGracefulAgentRestart(fdsw);
      // Wait for the switch to come up
      if (!verifySwSwitchRunState(fdsw, SwitchRunState::CONFIGURED)) {
        XLOG(DBG2) << "Agent failed to come up post warmboot: " << fdsw;
        return false;
      }
    }
  }

  // verify no flaps is expensive.
  // Thus, only verify after warmboot restarting one FDSW from each cluster.
  // There is no loss of signal due to this approach as if the sessions flap
  // due to an intermediate warmboot, it will be detected by this check
  // anyway.
  if (!verifyNoSessionsFlap(myHostname, baselinePeerToDsfSession)) {
    return false;
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulDeviceDownUpForRemoteSdsws() const {
  return true;
}

bool MultiNodeUtil::verifyGracefulDeviceDownUp() const {
  return verifyGracefulDeviceDownUpForRemoteRdsws() &&
      verifyGracefulDeviceDownUpForRemoteFdsws() &&
      verifyGracefulDeviceDownUpForRemoteSdsws();
}

bool MultiNodeUtil::verifyUngracefulDeviceDownUpForRemoteRdsws() const {
  return verifyDeviceDownUpForRemoteRdswsHelper(
      false /* triggerGracefulRestart*/);
}

bool MultiNodeUtil::verifyUngracefulDeviceDownUpForRemoteFdsws() const {
  // TODO verify
  return true;
}

bool MultiNodeUtil::verifyUngracefulDeviceDownUpForRemoteSdsws() const {
  // TODO verify
  return true;
}

bool MultiNodeUtil::verifyUngracefulDeviceDownUp() const {
  return verifyUngracefulDeviceDownUpForRemoteRdsws() &&
      verifyUngracefulDeviceDownUpForRemoteFdsws() &&
      verifyUngracefulDeviceDownUpForRemoteSdsws();
}

std::set<std::string>
MultiNodeUtil::triggerGraceFulRestartTimeoutForRemoteRdsws() const {
  auto static constexpr kGracefulRestartTimeout = 120;
  auto static constexpr kDelayBetweenRestarts =
      kGracefulRestartTimeout + 60 /* time to verify STALE post GR timeout */;

  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);

  // For any one RDSW in every remote cluster issue delayed Agent restart
  std::set<std::string> restartedRdsws;
  for (const auto& [_, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == myHostname) { // exclude self
        continue;
      }
      restartAgentWithDelay(rdsw, kDelayBetweenRestarts);
      restartedRdsws.insert(rdsw);

      // Gracefully restart only one remote RDSW per cluster
      break;
    }
  }

  return restartedRdsws;
}

bool MultiNodeUtil::verifyStaleSystemPorts(
    const std::set<std::string>& restartedRdsws) const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);

  auto staleSystemPorts = [this, myHostname, restartedRdsws] {
    // Verify system ports for restarted RDSWs are STALE
    // Verify system ports for non-restarted RDSWs are LIVE
    for (const auto& [rdsw, systemPorts] : getRdswToSystemPorts()) {
      bool isRestarted = restartedRdsws.find(rdsw) != restartedRdsws.end();

      for (const auto& systemPort : systemPorts) {
        auto livenessStatus = systemPort.remoteSystemPortLivenessStatus();
        if (!livenessStatus.has_value()) {
          continue;
        }

        if (isRestarted) {
          // Restarted RDSW should have STALE system ports
          if (livenessStatus.value() != LivenessStatus::STALE) {
            return false;
          }
        } else {
          // Non-Restarted RDSW should have LIVE system ports
          if (livenessStatus.value() != LivenessStatus::LIVE) {
            return false;
          }
        }
      }
    }

    return true;
  };

  return checkWithRetryErrorReturn(
      staleSystemPorts,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyStaleRifs(
    const std::set<std::string>& restartedRdsws) const {
  auto staleRifs = [this, restartedRdsws] {
    // Verify rifs for restarted RDSWs are STALE
    // Verify rifs for non-restarted RDSWs are LIVE
    for (const auto& [rdsw, rifs] : getRdswToRifs()) {
      bool isRestarted = restartedRdsws.find(rdsw) != restartedRdsws.end();

      for (const auto& rif : rifs) {
        auto livenessStatus = rif.remoteIntfLivenessStatus();
        if (!livenessStatus.has_value()) {
          continue;
        }

        if (isRestarted) {
          // Restarted RDSW should have STALE rifs
          if (livenessStatus.value() != LivenessStatus::STALE) {
            return false;
          }
        } else {
          // Non-Restarted RDSW should have LIVE rifs
          if (livenessStatus.value() != LivenessStatus::LIVE) {
            return false;
          }
        }
      }
    }

    return true;
  };

  return checkWithRetryErrorReturn(
      staleRifs,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyLiveSystemPorts() const {
  auto liveSystemPorts = [this] {
    for (const auto& [rdsw, systemPorts] : getRdswToSystemPorts()) {
      for (const auto& systemPort : systemPorts) {
        auto livenessStatus = systemPort.remoteSystemPortLivenessStatus();
        if (!livenessStatus.has_value()) {
          continue;
        }

        if (livenessStatus.value() != LivenessStatus::LIVE) {
          return false;
        }
      }
    }

    return true;
  };

  return checkWithRetryErrorReturn(
      liveSystemPorts,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);

  return true;
}

bool MultiNodeUtil::verifyLiveRifs() const {
  auto liveRifs = [this] {
    for (const auto& [_, rifs] : getRdswToRifs()) {
      for (const auto& rif : rifs) {
        auto livenessStatus = rif.remoteIntfLivenessStatus();
        if (!livenessStatus.has_value()) {
          continue;
        }

        if (livenessStatus.value() != LivenessStatus::LIVE) {
          return false;
        }
      }
    }

    return true;
  };

  return checkWithRetryErrorReturn(
      liveRifs,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyGracefulRestartTimeoutRecovery() const {
  // This test can be written as:
  //  - Stop Agent
  //  - Wait for 120s i.e. GR timeout
  //  - Verify entries are STALE
  //  - Start Agent
  //  - Verify entries are LIVE
  // However, in order to implement above sequence, we need a mechanism for
  // the test to invoke "Start Agent" when Agent thrift server is not
  // running.
  //
  // This can be accomplished by test client (this code) logging into the
  // remote device running Agent. But then the test needs to worry about
  // login credentials. Alternatively, the remote device can run a Thrift
  // server for the test purpose along, but that is an overkill for this use
  // case.
  //
  // This test logic solves it with the following approach:
  //  - Restart Agent API with delay: Test API supported by remote device
  //    - Stop Agent,
  //    - Sleep for 120s (GR Timeout) + 60s (time to verify STALE state)
  //    - Start Agent
  //  - Validate STALE state... while Agent is stopped.
  //  - Validate LIVE state.... after Agent has restarted.
  //

  auto restartedRdsws = triggerGraceFulRestartTimeoutForRemoteRdsws();

  // First: Agent stops, GR time out, Sys ports and Rifs turn STALE.
  // Later: Agent restarts, Sys ports and Rifs resync and turn LIVE.
  return verifyStaleSystemPorts(restartedRdsws) &&
      verifyStaleRifs(restartedRdsws) && verifyLiveSystemPorts() &&
      verifyLiveRifs();
}

bool MultiNodeUtil::verifyGracefulQsfpDownUp() const {
  XLOG(DBG2) << __func__;

  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  auto baselinePeerToDsfSession = getPeerToDsfSession(myHostname);

  // For every device (RDSW, FDSW, SDSW) issue QSFP graceful restart
  for (const auto& [_, switchName] : switchIdToSwitchName_) {
    triggerGracefulQsfpRestart(switchName);
  }

  // Wait for QSFP service to come up
  for (const auto& [_, switchName] : switchIdToSwitchName_) {
    if (!verifyQsfpServiceRunState(switchName, QsfpServiceRunState::ACTIVE)) {
      XLOG(DBG2) << "QSFP failed to come up post warmboot: " << switchName;
      return false;
    }
  }

  // No session flaps are expected for QSFP graceful restart.
  if (!verifyNoSessionsFlap(myHostname, baselinePeerToDsfSession)) {
    return false;
  }

  return true;
}

bool MultiNodeUtil::verifyUngracefulQsfpDownUpForRemoteRdsws() const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  auto baselinePeerToDsfSession = getPeerToDsfSession(myHostname);

  // For any one RDSW in every remote cluster issue ungraceful QSFP restart
  for (const auto& [_, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == myHostname) { // exclude self
        continue;
      }
      triggerUngracefulQsfpRestart(rdsw);

      // Wait for QSFP service to come up
      if (!verifyQsfpServiceRunState(rdsw, QsfpServiceRunState::ACTIVE)) {
        XLOG(DBG2) << "QSFP failed to come up post warmboot: " << rdsw;
        return false;
      }

      // Sessions to RDSW whose QSFP just restarted are expected to flap.
      // Verify no other sessions flap.
      auto expectedPeerToDsfSession = baselinePeerToDsfSession;
      expectedPeerToDsfSession.erase(rdsw);
      if (!verifyNoSessionsFlap(
              myHostname /* rdswToVerify */,
              expectedPeerToDsfSession,
              rdsw /* rdswToExclude */)) {
        return false;
      }

      // Verify all DSF sessions are established for the RDSW that was
      // restarted
      verifyAllSessionsEstablished(rdsw);

      // Ungracefully restart only one remote RDSW QSFP per cluster
      break;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyUngracefulQsfpDownUpForRemoteFdsws() const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  auto baselinePeerToDsfSession = getPeerToDsfSession(myHostname);

  // For any one FDSW in every remote cluster issue ungraceful QSFP restart
  std::set<std::string> restartedFdsws;
  for (const auto& [_, fdsws] : std::as_const(clusterIdToFdsws_)) {
    // Ungracefully restart only one remote FDSW QSFP per cluster
    if (!fdsws.empty()) {
      auto fdsw = fdsws.front();
      triggerUngracefulQsfpRestart(fdsw);
    }
  }

  for (const auto& fdsw : restartedFdsws) {
    // Wait for QSFP service to come up
    if (!verifyQsfpServiceRunState(fdsw, QsfpServiceRunState::ACTIVE)) {
      XLOG(DBG2) << "QSFP failed to come up post warmboot: " << fdsw;
      return false;
    }
  }

  // No session flaps are expected for QSFP restart.
  if (!verifyNoSessionsFlap(myHostname, baselinePeerToDsfSession)) {
    return false;
  }

  return true;
}

bool MultiNodeUtil::verifyUngracefulQsfpDownUpForRemoteSdsws() const {
  // TODO
  return true;
}

bool MultiNodeUtil::verifyUngracefulQsfpDownUp() const {
  return verifyUngracefulQsfpDownUpForRemoteRdsws() &&
      verifyUngracefulQsfpDownUpForRemoteFdsws() &&
      verifyUngracefulQsfpDownUpForRemoteSdsws();
}

bool MultiNodeUtil::verifyFsdbDownUpForRemoteRdswsHelper(
    bool triggerGraceFulRestart) const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  auto baselinePeerToDsfSession = getPeerToDsfSession(myHostname);

  // For any one RDSW in every remote cluster issue graceful FSDB restart
  for (const auto& [_, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == myHostname) { // exclude self
        continue;
      }

      // Trigger graceful or ungraceful FSDB restart
      triggerGraceFulRestart ? triggerGracefulFsdbRestart(rdsw)
                             : triggerUngracefulFsdbRestart(rdsw);

      // Wait for FSDB to come up
      if (!verifyFsdbIsUp(rdsw)) {
        XLOG(DBG2) << "FSDB failed to come up post restart: " << rdsw;
        return false;
      }

      // Sessions to RDSW whose FSDB just restarted are expected to flap.
      // Verify no other sessions flap.
      auto expectedPeerToDsfSession = baselinePeerToDsfSession;
      expectedPeerToDsfSession.erase(rdsw);
      if (!verifyNoSessionsFlap(
              myHostname /* rdswToVerify */,
              expectedPeerToDsfSession,
              rdsw /* rdswToExclude */)) {
        return false;
      }

      // Verify all DSF sessions are established for the RDSW that was
      // restarted
      verifyAllSessionsEstablished(rdsw);

      // Restart only one remote RDSW FSDB per cluster
      break;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulFsdbDownUp() const {
  XLOG(DBG2) << __func__;
  return verifyFsdbDownUpForRemoteRdswsHelper(
      true /* triggerGracefulFsdbRestart */);
}

bool MultiNodeUtil::verifyUngracefulFsdbDownUp() const {
  XLOG(DBG2) << __func__;
  return verifyFsdbDownUpForRemoteRdswsHelper(
      false /* triggerGracefulFsdbRestart */);
}

// For given RDSWs:
//    - Find Up Ethernet ports
//    - Compute InterfaceID corresponding to each of those ports
//    - Get IP address for that InterfaceID
//    - Add some offset to this IP to derive neighbor IP
//    - Use lower bits of the neighbor IP to derive some neighbor MAC
//
// Generate specified number of neighbors and return.
std::vector<MultiNodeUtil::NeighborInfo> MultiNodeUtil::computeNeighborsForRdsw(
    const std::string& rdsw,
    const int& numNeighbors) const {
  auto populateUpEthernetPorts = [this, rdsw, numNeighbors](auto& neighbors) {
    auto upEthernetPortNameToPortInfo = getUpEthernetPortNameToPortInfo(rdsw);
    CHECK(upEthernetPortNameToPortInfo.size() >= numNeighbors);

    for (const auto& [portName, portInfo] : upEthernetPortNameToPortInfo) {
      if (neighbors.size() >= static_cast<size_t>(numNeighbors)) {
        break;
      }

      NeighborInfo neighborInfo;
      neighborInfo.portID = *portInfo.portId();
      neighbors.push_back(neighborInfo);
    }

    return neighbors;
  };

  auto getSystemPortMin = [this, rdsw]() {
    // Get system portID range for the RDSW
    CHECK(switchNameToSwitchIds_.find(rdsw) != switchNameToSwitchIds_.end());
    auto switchId = *switchNameToSwitchIds_.at(rdsw).begin();
    auto switchIdToDsfNode = getSwitchIdToDsfNode(rdsw);
    CHECK(switchIdToDsfNode.find(switchId) != switchIdToDsfNode.end());
    auto ranges = switchIdToDsfNode.at(switchId).systemPortRanges();

    // TODO: Extend to work with multiple system port ranges
    CHECK(ranges->systemPortRanges()->size() >= 1);
    auto systemPortMin = *ranges->systemPortRanges()->front().minimum();

    return systemPortMin;
  };

  auto populateIntfIDs = [rdsw, getSystemPortMin](auto& neighbors) {
    auto systemPortMin = getSystemPortMin();
    for (auto& neighbor : neighbors) {
      neighbor.intfID = int32_t(systemPortMin) + neighbor.portID;
    }
  };

  auto getIntfIDToIp = [this, rdsw]() {
    std::map<int32_t, folly::IPAddress> intfIDToIp;
    for (const auto& [_, rif] : getIntfIdToIntf(rdsw)) {
      for (const auto& ipPrefix : *rif.address()) {
        auto ip = folly::IPAddress::fromBinary(
            folly::ByteRange(
                reinterpret_cast<const unsigned char*>(
                    ipPrefix.ip()->addr()->data()),
                ipPrefix.ip()->addr()->size()));

        if (!folly::IPAddress(ip).isLinkLocal()) {
          // Pick any one non-local IP per interface
          intfIDToIp[*rif.interfaceId()] = ip;
          break;
        }
      }
    }

    return intfIDToIp;
  };

  auto computeNeighborIpAndMac = [this](const std::string& ipAddress) {
    auto constexpr kOffset = 0x10;
    auto ipv6Address = folly::IPAddressV6::tryFromString(ipAddress);
    std::array<uint8_t, 16> bytes = ipv6Address->toByteArray();
    bytes[15] += kOffset; // add some offset to derive neighbor IP
    auto neighborIp = folly::IPAddressV6::fromBinary(bytes);

    // Resolve Neighbor to Router MAC.
    //
    // Neighbor is resolved on a loopback port.
    // Thus, packets out of this port get looped back. Since those packets
    // carry router MAC as dstMac the packets get routed and help us create
    // traffic loop.
    auto macStr = utility::getMacForFirstInterfaceWithPorts(sw_->getState());
    auto neighborMac = folly::MacAddress(macStr);

    return std::make_pair(neighborIp, neighborMac);
  };

  auto populateNeighborIpAndMac =
      [rdsw, getIntfIDToIp, computeNeighborIpAndMac](auto& neighbors) {
        auto intfIDToIp = getIntfIDToIp();

        for (auto& neighbor : neighbors) {
          CHECK(intfIDToIp.find(neighbor.intfID) != intfIDToIp.end())
              << "rdsw: " << rdsw << " neighbor.intfID: " << neighbor.intfID;
          auto ip = intfIDToIp.at(neighbor.intfID);
          auto [neighborIp, neighborMac] = computeNeighborIpAndMac(ip.str());

          neighbor.ip = neighborIp;
          neighbor.mac = neighborMac;
        }
      };

  std::vector<NeighborInfo> neighbors;
  populateUpEthernetPorts(neighbors);
  populateIntfIDs(neighbors);
  populateNeighborIpAndMac(neighbors);

  return neighbors;
}

// if allNeighborsMustBePresent is true, then all neighbors must be present
// for every rdsw in rdswToNdpEntries.
// if allNeighborsMustBePresent is false, then all neighbors must be absent
// for every rdsw in rdswToNdpEntries.
bool MultiNodeUtil::verifyNeighborHelper(
    const std::vector<MultiNodeUtil::NeighborInfo>& neighbors,
    const std::map<std::string, std::vector<NdpEntryThrift>>& rdswToNdpEntries,
    bool allNeighborsMustBePresent) const {
  auto isNeighborPresentHelper = [](const auto& ndpEntries,
                                    const auto& neighbor) {
    for (const auto& ndpEntry : ndpEntries) {
      auto ndpEntryIp = folly::IPAddress::fromBinary(
          folly::ByteRange(
              folly::StringPiece(ndpEntry.ip().value().addr().value())));

      if (ndpEntry.interfaceID().value() == neighbor.intfID &&
          ndpEntryIp == neighbor.ip) {
        return true;
      }
    }

    return false;
  };

  logRdswToNdpEntries(rdswToNdpEntries);
  for (const auto& neighbor : neighbors) {
    for (const auto& [rdsw, ndpEntries] : rdswToNdpEntries) {
      auto isNeighborPresent = isNeighborPresentHelper(ndpEntries, neighbor);
      if (allNeighborsMustBePresent) {
        if (!isNeighborPresent) {
          XLOG(DBG2) << "RDSW: " << rdsw
                     << " neighbor missing: " << neighbor.str();
          return false;
        }
      } else { // allNeighborsMust NOT be present
        if (isNeighborPresent) {
          XLOG(DBG2) << "RDSW: " << rdsw
                     << " excess neighbor: " << neighbor.str();
          return false;
        }
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyNeighborsPresent(
    const std::string& rdswToVerify,
    const std::vector<MultiNodeUtil::NeighborInfo>& neighbors) const {
  auto verifyNeighborPresentHelper = [this, rdswToVerify, neighbors] {
    auto getRdswToNdpEntries = [this, rdswToVerify]() {
      std::map<std::string, std::vector<NdpEntryThrift>> rdswToNdpEntries;
      for (const auto& rdsw : allRdsws_) {
        if (rdsw == rdswToVerify) { // PROBE/REACHABLE for rdswToVerify
          rdswToNdpEntries[rdsw] =
              getNdpEntriesOfType(rdswToVerify, {"PROBE", "REACHABLE"});
        } else { // DYNAMIC for every remote RDSW
          rdswToNdpEntries[rdsw] = getNdpEntriesOfType(rdsw, {"DYNAMIC"});
        }
      }

      return rdswToNdpEntries;
    };

    // Every neighbor added to rdswToVerify, the neighbor must be:
    //    - PROBE/REACHABLE for rdswToVerify
    //    - DYNAMIC for every other rdsw.
    auto rdswToNdpEntries = getRdswToNdpEntries();
    logRdswToNdpEntries(rdswToNdpEntries);
    return verifyNeighborHelper(
        neighbors, rdswToNdpEntries, true /* allNeighborsMustBePresent */);
  };

  return checkWithRetryErrorReturn(
      verifyNeighborPresentHelper,
      10 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyNeighborLocalPresent(
    const std::string& rdsw,
    const std::vector<MultiNodeUtil::NeighborInfo>& neighbors) const {
  auto verifyNeighborLocalPresentHelper = [this, rdsw, neighbors]() {
    auto isLocalNeighborPresent = [this, rdsw](const auto& neighbor) {
      auto ndpEntries = getNdpEntries(rdsw);

      for (const auto& ndpEntry : ndpEntries) {
        auto ndpEntryIp = folly::IPAddress::fromBinary(
            folly::ByteRange(
                folly::StringPiece(ndpEntry.ip().value().addr().value())));

        if (ndpEntry.interfaceID().value() == neighbor.intfID &&
            ndpEntryIp == neighbor.ip) {
          return true;
        }
      }

      return false;
    };

    for (const auto& neighbor : neighbors) {
      if (isLocalNeighborPresent(neighbor)) {
        return true;
      }
    }

    return false;
  };

  return checkWithRetryErrorReturn(
      verifyNeighborLocalPresentHelper,
      10 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyNeighborsAbsent(
    const std::vector<MultiNodeUtil::NeighborInfo>& neighbors,
    const std::optional<std::string>& rdswToExclude) const {
  auto verifyNeighborAbsentHelper = [this, neighbors, rdswToExclude] {
    auto getRdswToAllNdpEntries = [this, rdswToExclude]() {
      std::map<std::string, std::vector<NdpEntryThrift>> rdswToAllNdpEntries;
      for (const auto& rdsw : allRdsws_) {
        if (rdswToExclude.has_value() && rdswToExclude.value() == rdsw) {
          continue;
        }

        rdswToAllNdpEntries[rdsw] = getNdpEntries(rdsw);
      }

      return rdswToAllNdpEntries;
    };

    auto rdswToAllNdpEntries = getRdswToAllNdpEntries();
    logRdswToNdpEntries(rdswToAllNdpEntries);
    return verifyNeighborHelper(
        neighbors,
        rdswToAllNdpEntries,
        false /* allNeighborsMust NOT be present */);
  };

  return checkWithRetryErrorReturn(
      verifyNeighborAbsentHelper,
      10 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyNeighborLocalPresentRemoteAbsent(
    const std::vector<MultiNodeUtil::NeighborInfo>& neighbors,
    const std::string& rdsw) const {
  // verify neighbor entry is present on local RDSW, but absent
  // from all remote RDSWs
  if (verifyNeighborLocalPresent(rdsw, neighbors)) {
    return verifyNeighborsAbsent(neighbors, rdsw /* rdsw to exclude */);
  }

  return false;
}

bool MultiNodeUtil::verifyNeighborAddRemove() const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);

  // For any one RDSW in every cluster
  for (const auto& [_, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == myHostname) { // exclude self
        continue;
      }

      auto neighbors =
          computeNeighborsForRdsw(rdsw, 2 /* number of neighbors */);
      CHECK_EQ(neighbors.size(), 2);
      auto firstNeighbor = neighbors[0];
      auto secondNeighbor = neighbors[1];

      for (const auto& neighbor : neighbors) {
        addNeighbor(
            rdsw, neighbor.intfID, neighbor.ip, neighbor.mac, neighbor.portID);
      }
      // Verify every neighbor is added/sync'ed to every RDSW
      if (!verifyNeighborsPresent(rdsw, neighbors)) {
        XLOG(DBG2) << "Neighbor add verification failed: " << rdsw;
        return false;
      }

      // Remove first neighbor and verify it is removed from every RDSW
      removeNeighbor(rdsw, firstNeighbor.intfID, firstNeighbor.ip);
      if (!verifyNeighborsAbsent({firstNeighbor})) {
        XLOG(DBG2) << "Neighbor remove verification failed: " << rdsw;
        return false;
      }

      // Disable second neighbor port and verify it is removed from every
      // RDSW
      adminDisablePort(rdsw, secondNeighbor.portID);
      if (!verifyNeighborLocalPresentRemoteAbsent({secondNeighbor}, rdsw)) {
        XLOG(DBG2) << "Neighbor remove verification failed: " << rdsw;
        return false;
      }

      // Add neighbor to one remote RDSW per cluster
      break;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyRoutePresent(
    const std::string& rdsw,
    const folly::IPAddress& destPrefix,
    const int16_t prefixLength) const {
  auto verifyRoutePresentHelper = [rdsw, destPrefix, prefixLength]() {
    for (const auto& route : getAllRoutes(rdsw)) {
      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(
              reinterpret_cast<const unsigned char*>(
                  route.dest()->ip()->addr()->data()),
              route.dest()->ip()->addr()->size()));
      if (ip == destPrefix && *route.dest()->prefixLength() == prefixLength) {
        XLOG(DBG2) << "rdsw: " << rdsw << " Found route:: prefix: " << ip.str()
                   << " prefixLength: " << *route.dest()->prefixLength();
        return true;
      }
    }

    return false;
  };

  return checkWithRetryErrorReturn(
      verifyRoutePresentHelper,
      30 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

std::map<std::string, MultiNodeUtil::NeighborInfo>
MultiNodeUtil::configureNeighborsAndRoutesForTrafficLoop() const {
  auto logAddRoute =
      [](const auto& rdsw, const auto& prefix, const auto& neighbor) {
        XLOG(DBG2) << "Adding route:: " << " prefix: " << prefix.str()
                   << " nexthop: " << neighbor.str() << " to " << rdsw;
      };

  std::map<std::string, NeighborInfo> rdswToNeighbor;
  auto [prefix, prefixLength] = kGetRoutePrefixAndPrefixLength();
  std::optional<std::string> prevRdsw{std::nullopt};
  MultiNodeUtil::NeighborInfo firstRdswNeighbor{};

  // Add a neighbor for every RDSW in the cluster
  // Also, add routes for every RDSW to create a loop i.e.:
  //    - RDSW A has route with nexthop as RDSW B's neighbor
  //    - RDSW B has route with nexthop as RDSW C's neighbor
  //    ...
  //    - RDSW Z has route with nexthop as RDSW A's neighbor
  //      so the packet loops around
  for (const auto& rdsw : allRdsws_) {
    auto neighbors = computeNeighborsForRdsw(rdsw, 1 /* number of neighbors */);
    CHECK_EQ(neighbors.size(), 1);
    auto neighbor = neighbors[0];
    rdswToNeighbor[rdsw] = neighbor;

    XLOG(DBG2) << "Adding neighbor: " << neighbor.str() << " to " << rdsw;
    addNeighbor(
        rdsw, neighbor.intfID, neighbor.ip, neighbor.mac, neighbor.portID);
    if (!verifyNeighborsPresent(rdsw, {neighbor})) {
      XLOG(DBG2) << "Neighbor add verification failed: " << rdsw
                 << " neighbor: " << neighbor.str();
      return rdswToNeighbor;
    }

    if (!prevRdsw.has_value()) { // first RDSW
      firstRdswNeighbor = neighbor;
    } else {
      logAddRoute(prevRdsw.value(), kPrefix, neighbor);
      addRoute(prevRdsw.value(), prefix, prefixLength, {neighbor.ip});
      if (!verifyRoutePresent(prevRdsw.value(), prefix, prefixLength)) {
        XLOG(DBG2) << "Route add verification failed: " << prevRdsw.value()
                   << " route: " << prefix.str()
                   << " prefixLength: " << prefixLength;
        return rdswToNeighbor;
      }
    }
    prevRdsw = rdsw;
  }

  // Add route for first RDSW to complete the loop
  CHECK(!allRdsws_.empty());
  auto lastRdsw = std::prev(allRdsws_.end());
  logAddRoute(*lastRdsw, prefix, firstRdswNeighbor);
  addRoute(*lastRdsw, prefix, prefixLength, {firstRdswNeighbor.ip});
  if (!verifyRoutePresent(*lastRdsw, prefix, prefixLength)) {
    XLOG(DBG2) << "Route add verification failed: " << *lastRdsw
               << " route: " << prefix.str()
               << " prefixLength: " << prefixLength;
    return rdswToNeighbor;
  }

  return rdswToNeighbor;
}

void MultiNodeUtil::createTrafficLoop(const NeighborInfo& neighborInfo) const {
  // configureNeighborsAndRoutesForTrafficLoop configures
  //  o RDSW A route to point to RDSW B's neighbor,
  //  o RDSW B route to point to RDSW C's neighbor,
  //  o ...
  //  o last RDSW's route to point to RDSW A's neighbor.
  //
  // Send packet froms self (say RDSW A) with dstMAC as RouterMAC:
  //  o The packet is routed to RDSW B's neighbor.
  //  o RDSW B neighbor is resolved on a port in loopback mode.
  //  o The packet thus gets looped back.
  //  o Since the packet carries dstMac = Router MAC, it gets routed.
  //  o This packet routes to RDSW C's neighbor
  //  ....
  //  o The last RDSW routes the packet to RDSW A and loop continues.
  //
  // Forwarding is enabled for TTL0 packets so the packet continues to be
  // forwarded even after TTL is 0. Thus, we get traffic flood.
  //
  // Injecting 1000 packets on one 400G NIF port is sufficient to create a
  // loop that saturates the 400G RDSW links on every RDSW.
  //
  // TODO: Before injecting packets, verify that
  //  o neighbor is resolved and programmed on every RDSW.
  //  o the route is programmed on every RDSW with that neighbor as nexthop.
  // Otherwise, the packets will be blackholed and we will not get traffic
  // loop.
  auto static kSrcIP = folly::IPAddressV6("2001:0db8:85a0::");
  auto [prefix, _] = kGetRoutePrefixAndPrefixLength();
  for (int i = 0; i < 1000; i++) {
    auto txPacket = utility::makeUDPTxPacket(
        sw_,
        std::nullopt, // vlanIDForTx
        folly::MacAddress("00:02:00:00:01:01"), // srcMac
        utility::getMacForFirstInterfaceWithPorts(sw_->getState()), // dstMac
        kSrcIP,
        prefix, // dstIP
        8000,
        8001,
        0, // ECN
        255, // TTL
        // Payload
        std::vector<uint8_t>(1200, 0xff));

    sw_->sendPacketOutOfPortAsync(
        std::move(txPacket), PortID(neighborInfo.portID));
  }
}

bool MultiNodeUtil::verifyLineRate(
    const std::string& rdsw,
    const MultiNodeUtil::NeighborInfo& neighborInfo) const {
  auto portIdToPortInfo = getPortIdToPortInfo(rdsw);
  CHECK(portIdToPortInfo.find(neighborInfo.portID) != portIdToPortInfo.end());
  auto portName = portIdToPortInfo[neighborInfo.portID].name().value();
  auto portSpeedMbps =
      portIdToPortInfo[neighborInfo.portID].speedMbps().value();

  // Verify is line rate is achieved i.e. in bytes within 5% of line rate.
  constexpr float kVariance = 0.05; // i.e. + or - 5%
  auto lowPortSpeedMbps = portSpeedMbps * (1 - kVariance);

  auto verifyLineRateHelper =
      [rdsw, portName, portSpeedMbps, lowPortSpeedMbps]() {
        auto counterNameToCount = getCounterNameToCount(rdsw);
        auto outSpeedMbps =
            counterNameToCount[portName + ".out_bytes.rate.60"] * 8 / 1000000;
        XLOG(DBG2) << "portSpeedMbps: " << portSpeedMbps
                   << " lowPortSpeedMbps: " << lowPortSpeedMbps
                   << " outSpeedMbps: " << outSpeedMbps;

        return lowPortSpeedMbps < outSpeedMbps;
      };

  return checkWithRetryErrorReturn(
      verifyLineRateHelper,
      60 /* num retries, flood takes about ~30s to reach line rate */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyFabricSpray(const std::string& rdsw) const {
  auto verifyFabricSprayHelper = [this, rdsw]() {
    int64_t lowestMbps = std::numeric_limits<int64_t>::max();
    int64_t highestMbps = std::numeric_limits<int64_t>::min();

    auto counterNameToCount = getCounterNameToCount(rdsw);
    for (const auto& [_, portInfo] : getActiveFabricPortNameToPortInfo(rdsw)) {
      auto portName = portInfo.name().value();
      auto counterName = portName + ".out_bytes.rate.60";
      auto outSpeedMbps = counterNameToCount[counterName] * 8 / 1000000;

      lowestMbps = std::min(lowestMbps, outSpeedMbps);
      highestMbps = std::max(highestMbps, outSpeedMbps);

      XLOG(DBG2) << "Active Fabric port: " << portInfo.name().value()
                 << " outSpeedMbps: " << outSpeedMbps
                 << " lowestMbps: " << lowestMbps
                 << " highestMbps: " << highestMbps;
    }

    return isDeviationWithinThreshold(
        lowestMbps, highestMbps, 5 /* maxDeviationPct */);
  };

  return checkWithRetryErrorReturn(
      verifyFabricSprayHelper,
      30 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::setupTrafficLoop() const {
  // Configure for Traffic loop
  auto rdswToNeighbor = configureNeighborsAndRoutesForTrafficLoop();
  if (rdswToNeighbor.empty()) {
    return false;
  }

  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  CHECK(rdswToNeighbor.find(myHostname) != rdswToNeighbor.end());
  // Create Traffic loop
  createTrafficLoop(rdswToNeighbor[myHostname]);

  for (const auto& [rdsw, neighbor] : rdswToNeighbor) {
    if (!verifyLineRate(rdsw, neighbor)) {
      XLOG(DBG2) << "Verify line rate failed for rdsw: " << rdsw;
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyTrafficSpray() const {
  XLOG(DBG2) << __func__;

  auto getPeerToDsfSessionForAllRdsws = [this] {
    std::map<std::string, std::map<std::string, DsfSessionThrift>>
        rdswToPeerAndDsfSession;
    for (const auto& rdsw : allRdsws_) {
      rdswToPeerAndDsfSession[rdsw] = getPeerToDsfSession(rdsw);
    }

    return rdswToPeerAndDsfSession;
  };

  auto verifyNoSessionsFlapForAllRdsws =
      [this](const auto& baselineRdswToPeerAndDsfSession) {
        for (const auto& [rdsw, baselinePeerToDsfSession] :
             baselineRdswToPeerAndDsfSession) {
          if (!verifyNoSessionsFlap(rdsw, baselinePeerToDsfSession)) {
            XLOG(DBG2) << "DSF Sessions flapped from rdsw: " << rdsw;
            return false;
          }
        }

        return true;
      };

  // Store all DSF sessions for every RDSW before creating Traffic loop
  auto baselineRdswToPeerAndDsfSession = getPeerToDsfSessionForAllRdsws();

  if (!setupTrafficLoop()) {
    XLOG(DBG2) << "Traffic loop setup failed";
    return false;
  }

  for (const auto& [switchName, _] : switchNameToSwitchIds_) {
    if (!verifyFabricSpray(switchName)) {
      XLOG(DBG2) << "Verify line rate failed for switch: " << switchName;
      return false;
    }
  }

  // Verify no DSF Sessions flapped due to traffic loop
  if (!verifyNoSessionsFlapForAllRdsws(baselineRdswToPeerAndDsfSession)) {
    XLOG(DBG2) << "Traffic flood flapped some DSF sessions";
    return false;
  }

  return true;
}

bool MultiNodeUtil::runScenariosAndVerifyNoDrops(
    const std::vector<Scenario>& scenarios) const {
  for (const auto& scenario : scenarios) {
    XLOG(DBG2) << "Running scenario: " << scenario.name;
    if (!scenario.setup()) {
      XLOG(DBG2) << "Scenario: " << scenario.name << " failed";
      return false;
    }

    if (!verifyNoReassemblyErrorsForAllSwitches()) {
      // TODO query drop counters to root cause reason for reassembly errors
      XLOG(DBG2) << "Scenario: " << scenario.name
                 << " unexpected reassembly errors";
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyNoReassemblyErrorsForAllSwitches() const {
  auto verifyNoReassemblyErrorsForAllSwitchesHelper = [this]() {
    auto getCounterName = [this](const auto& switchName) {
      auto iter = switchNameToAsicType_.find(switchName);
      CHECK(iter != switchNameToAsicType_.end());
      auto asicType = iter->second;
      auto vendorName = getHwAsicForAsicType(asicType).getVendor();

      return vendorName + ".reassembly.errors.sum";
    };

    for (const auto& [switchName, _] : switchNameToSwitchIds_) {
      auto counterNameToCount = getCounterNameToCount(switchName);
      auto counterName = getCounterName(switchName);
      auto reassemblyErrors = counterNameToCount[counterName];
      if (reassemblyErrors > 0) {
        XLOG(DBG2) << "Switch: " << switchName
                   << " counterName: " << counterName
                   << " reassemblyErrors: " << reassemblyErrors;
        return false;
      }
    }

    return true;
  };

  // The reassembly errors may happen after a few seeconds.
  // Thus, keep retrying for several seconds to ensure no reassembly errors.
  return checkAlwaysTrueWithRetryErrorReturn(
      verifyNoReassemblyErrorsForAllSwitchesHelper, 10 /* num retries */);
}

std::set<std::string> MultiNodeUtil::getOneFabricSwitchForEachCluster() const {
  // Get one FDSW from each cluster + one SDSW
  std::set<std::string> fabricSwitchesToTest;
  for (const auto& [_, fdsws] : std::as_const(clusterIdToFdsws_)) {
    if (!fdsws.empty()) {
      fabricSwitchesToTest.insert(fdsws.front());
    }
  }

  if (!sdsws_.empty()) {
    fabricSwitchesToTest.insert(*sdsws_.begin());
  }

  return fabricSwitchesToTest;
}

bool MultiNodeUtil::verifyNoTrafficDropOnProcessRestarts() const {
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);

  if (!setupTrafficLoop()) {
    XLOG(DBG2) << "Traffic loop setup failed";
    return false;
  }

  if (!verifyNoReassemblyErrorsForAllSwitches()) {
    XLOG(DBG2) << "Unexpected reassembly errors";
    // TODO query drop counters to root cause reason for reassembly errors
    return false;
  }

  // With traffic loop running, execute a variety of scenarios.
  // For each scenario, expect no drops on Fabric ports.

  auto allQsfpRestartHelper = [this](bool gracefulRestart) {
    {
      // Gracefully restart QSFP on all switches
      forEachExcluding(
          allSwitches_,
          {}, // exclude none
          gracefulRestart ? triggerGracefulQsfpRestart
                          : triggerUngracefulQsfpRestart);

      // Wait for QSFP to come up on all switches
      auto restart = checkForEachExcluding(
          allSwitches_,
          {}, // exclude none
          [this](
              const std::string& switchName, const QsfpServiceRunState& state) {
            return this->verifyQsfpServiceRunState(switchName, state);
          },
          QsfpServiceRunState::ACTIVE);

      return restart;
    }
  };

  Scenario gracefullyRestartQsfpAllSwitches = {
      "gracefullyRestartQsfpAllSwitches",
      [&] { return allQsfpRestartHelper(true /* gracefulRestart */); }};

  Scenario unGracefullyRestartQsfpAllSwitches = {
      "unGracefullyRestartQsfpAllSwitches",
      [&] { return allQsfpRestartHelper(false /* ungracefulRestart */); }};

  Scenario gracefullyRestartFSDBAllSwitches = {
      "gracefullyRestartFSDBAllSwitches", [this]() {
        // Gracefully restart FSDB on all switches
        forEachExcluding(
            allSwitches_,
            {}, // exclude none
            triggerGracefulFsdbRestart);

        // Wait for FSDB to come up on all switches
        auto gracefulRestart = checkForEachExcluding(
            allSwitches_,
            {}, // exclude none
            [this](const std::string& switchName) {
              return this->verifyFsdbIsUp(switchName);
            });

        return gracefulRestart;
      }};

  Scenario ungracefullyRestartFSDBAllSwitches = {
      "ungracefullyRestartFSDBAllSwitches", [this]() {
        // Ungracefully restart FSDB on all switches
        forEachExcluding(
            allSwitches_,
            {}, // exclude none
            triggerUngracefulFsdbRestart);

        // Wait for FSDB to come up on all switches
        auto ungracefulRestart = checkForEachExcluding(
            allSwitches_,
            {}, // exclude none
            [this](const std::string& switchName) {
              return this->verifyFsdbIsUp(switchName);
            });

        return ungracefulRestart;
      }};

  Scenario gracefullyRestartAgentAllSwitches = {
      "gracefullyRestartAgentAllSwitches", [this, myHostname]() {
        // Gracefully restart Agent on all switches
        forEachExcluding(
            allSwitches_,
            {myHostname}, // exclude self
            triggerGracefulAgentRestart);

        // Wait for Agent to come up on all switches
        auto gracefulRestart = checkForEachExcluding(
            allSwitches_,
            {myHostname}, // exclude self
            [this](const std::string& switchName, const SwitchRunState& state) {
              return this->verifySwSwitchRunState(switchName, state);
            },
            SwitchRunState::CONFIGURED);

        return gracefulRestart;
      }};

  std::vector<Scenario> scenarios = {
      std::move(gracefullyRestartQsfpAllSwitches),
      std::move(unGracefullyRestartQsfpAllSwitches),
      std::move(gracefullyRestartFSDBAllSwitches),
      std::move(ungracefullyRestartFSDBAllSwitches),
      std::move(gracefullyRestartAgentAllSwitches),
  };

  return runScenariosAndVerifyNoDrops(scenarios);
}

bool MultiNodeUtil::drainUndrainActiveFabricLinkForSwitch(
    const std::string& switchName) const {
  auto isPortDrainedHelper = [switchName](int32_t portId, bool drained) {
    auto portIdToPortInfo = getPortIdToPortInfo(switchName);
    auto iter = portIdToPortInfo.find(portId);
    CHECK(iter != portIdToPortInfo.end());
    return iter->second.isDrained() == drained;
  };

  auto activeFabricPortNameToPortInfo =
      getActiveFabricPortNameToPortInfo(switchName);
  CHECK(!activeFabricPortNameToPortInfo.empty());

  auto [_, portInfo] = *activeFabricPortNameToPortInfo.begin();
  XLOG(DBG2) << "Draining port: " << portInfo.name().value();
  drainPort(switchName, portInfo.portId().value());

  if (!checkWithRetryErrorReturn(
          [&] {
            return isPortDrainedHelper(
                portInfo.portId().value(), true /* drained */);
          },
          30 /* num retries */)) {
    XLOG(DBG2) << "Port not drained: " << portInfo.name().value();
    return false;
  }

  XLOG(DBG2) << "Undraining port: " << portInfo.name().value();
  undrainPort(switchName, portInfo.portId().value());

  if (!checkWithRetryErrorReturn(
          [&] {
            return isPortDrainedHelper(
                portInfo.portId().value(), false /* undrained */);
          },
          30 /* num retries */)) {
    XLOG(DBG2) << "Port not undrained: " << portInfo.name().value();
    return false;
  }

  return true;
}

bool MultiNodeUtil::verifyNoTrafficDropOnDrainUndrain() const {
  if (!setupTrafficLoop()) {
    XLOG(DBG2) << "Traffic loop setup failed";
    return false;
  }

  if (!verifyNoReassemblyErrorsForAllSwitches()) {
    XLOG(DBG2) << "Unexpected reassembly errors";
    // TODO query drop counters to root cause reason for reassembly errors
    return false;
  }

  Scenario drainUndrainFabricLinkOnePerFabric = {
      "drainUndrainFabricLinkOnePerFabric", [this]() {
        auto fabricSwitchesToDrainUndrainLinks =
            getOneFabricSwitchForEachCluster();

        auto drainUndrainFabricLink = checkForEachExcluding(
            fabricSwitchesToDrainUndrainLinks,
            {}, // exclude none
            [this](const std::string& switchName) {
              return drainUndrainActiveFabricLinkForSwitch(switchName);
            });

        return drainUndrainFabricLink;
      }};

  const std::vector<Scenario> scenarios = {
      std::move(drainUndrainFabricLinkOnePerFabric),
  };

  return runScenariosAndVerifyNoDrops(scenarios);
}

bool MultiNodeUtil::verifySelfHealingECMPLag() const {
  return true;
}

} // namespace facebook::fboss::utility
