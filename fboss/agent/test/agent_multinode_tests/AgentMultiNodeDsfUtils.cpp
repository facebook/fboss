// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"

#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::utility {

bool verifyFabricConnectivityForRdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    int clusterId,
    const std::string& rdswToVerify) {
  return true;
}

bool verifyFabricConnectivityForRdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  return true;

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

bool verifyFabricConnectivityForFdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  return true;
}

bool verifyFabricConnectivityForSdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
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
