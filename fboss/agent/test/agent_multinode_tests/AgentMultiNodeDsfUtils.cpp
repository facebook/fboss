// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
using namespace facebook::fboss::utility;

bool verifyFabricConnectivityHelper(
    const std::string& switchToVerify,
    const std::set<std::string>& expectedConnectedSwitches) {
  std::set<std::string> gotConnectedSwitches;
  for (const auto& [portName, fabricEndpoint] :
       getFabricPortToFabricEndpoint(switchToVerify)) {
    if (fabricEndpoint.isAttached().value()) {
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
  return expectedConnectedSwitches == gotConnectedSwitches;
}

} // namespace

namespace facebook::fboss::utility {

bool verifyFabricConnectivityForRdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    int clusterId,
    const std::string& rdswToVerify) {
  // Every RDSW is connected to all FDSWs in its cluster
  std::set<std::string> expectedConnectedSwitches(
      std::begin(topologyInfo->getClusterIdToFdsws().at(clusterId)),
      std::end(topologyInfo->getClusterIdToFdsws().at(clusterId)));

  return verifyFabricConnectivityHelper(
      rdswToVerify, expectedConnectedSwitches);
}

bool verifyFabricConnectivityForRdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& [clusterId, rdsws] :
       std::as_const(topologyInfo->getClusterIdToRdsws())) {
    for (const auto& rdsw : rdsws) {
      if (!verifyFabricConnectivityForRdsw(topologyInfo, clusterId, rdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool verifyFabricConnectivityForFdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    int clusterId,
    const std::string& fdswToVerify) {
  // Every FDSW is connected to all RDSWs in its cluster and all SDSWs
  std::set<std::string> expectedConnectedSwitches(
      std::begin(topologyInfo->getClusterIdToRdsws().at(clusterId)),
      std::end(topologyInfo->getClusterIdToRdsws().at(clusterId)));
  expectedConnectedSwitches.insert(
      topologyInfo->getSdsws().begin(), topologyInfo->getSdsws().end());

  return verifyFabricConnectivityHelper(
      fdswToVerify, expectedConnectedSwitches);
}

bool verifyFabricConnectivityForFdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& [clusterId, fdsws] :
       std::as_const(topologyInfo->getClusterIdToFdsws())) {
    for (const auto& fdsw : fdsws) {
      if (!verifyFabricConnectivityForFdsw(topologyInfo, clusterId, fdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool verifyFabricConnectivityForSdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    const std::string& sdswToVerify) {
  // Every SDSW is connected to all FDSWs in all clusters
  return verifyFabricConnectivityHelper(sdswToVerify, topologyInfo->getFdsws());
}

bool verifyFabricConnectivityForSdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& sdsw : std::as_const(topologyInfo->getSdsws())) {
    if (!verifyFabricConnectivityForSdsw(topologyInfo, sdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyFabricConnectivity(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  return verifyFabricConnectivityForRdsws(topologyInfo) &&
      verifyFabricConnectivityForFdsws(topologyInfo) &&
      verifyFabricConnectivityForSdsws(topologyInfo);
}

void verifyDsfCluster(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(5000), {
    verifyFabricConnectivity(topologyInfo);
  });
}

void verifyDsfAgentDownUp() {}

} // namespace facebook::fboss::utility
