// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchNeighborTests.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchTrafficTest
    : public AgentMultiNodeVoqSwitchNeighborTest {
 protected:
  std::map<std::string, utility::Neighbor>
  configureNeighborsAndRoutesForTrafficLoop(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
    std::map<std::string, utility::Neighbor> rdswToNeighbor;

    // Add a neighbor for every RDSW in the cluster
    for (const auto& rdsw : topologyInfo->getRdsws()) {
      auto neighbors = computeNeighborsForRdsw(
          topologyInfo, rdsw, 1 /* number of neighbors */);
      CHECK_EQ(neighbors.size(), 1);
      auto neighbor = neighbors[0];
      rdswToNeighbor[rdsw] = neighbor;

      XLOG(DBG2) << "Adding neighbor: " << neighbor.str() << " to " << rdsw;
      utility::addNeighbor(
          rdsw, neighbor.intfID, neighbor.ip, neighbor.mac, neighbor.portID);
      if (!utility::verifyNeighborsPresent(
              topologyInfo->getRdsws(), rdsw, {neighbor})) {
        XLOG(DBG2) << "Neighbor add verification failed: " << rdsw
                   << " neighbor: " << neighbor.str();
        return rdswToNeighbor;
      }
    }
    return rdswToNeighbor;
  }

 public:
  bool verifyTrafficSpray(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
    return true;
  }
};

TEST_F(AgentMultiNodeVoqSwitchTrafficTest, verifyTrafficSpray) {
  runTestWithVerifyCluster([this](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return this->verifyTrafficSpray(topologyInfo);
    }
  });
}

} // namespace facebook::fboss
