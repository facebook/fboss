// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchNeighborTests.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchTrafficTest
    : public AgentMultiNodeVoqSwitchNeighborTest {
 protected:
  std::pair<folly::IPAddressV6, int16_t> kGetRoutePrefixAndPrefixLength()
      const {
    return std::make_pair(folly::IPAddressV6("2001:0db8:85a3::"), 64);
  }

  std::map<std::string, utility::Neighbor>
  configureNeighborsAndRoutesForTrafficLoop(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
    std::map<std::string, utility::Neighbor> rdswToNeighbor;
    auto [prefix, prefixLength] = kGetRoutePrefixAndPrefixLength();
    std::optional<std::string> prevRdsw{std::nullopt};
    utility::Neighbor firstRdswNeighbor{};

    // Add a neighbor for every RDSW in the cluster
    // Also, add routes for every RDSW to create a loop i.e.:
    //    - RDSW A has route with nexthop as RDSW B's neighbor
    //    - RDSW B has route with nexthop as RDSW C's neighbor
    //    ...
    //    - RDSW Z has route with nexthop as RDSW A's neighbor
    //      so the packet loops around
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
        return {};
      }

      if (!prevRdsw.has_value()) { // first RDSW
        firstRdswNeighbor = neighbor;
      } else {
        utility::addRoute(
            prevRdsw.value(), prefix, prefixLength, {neighbor.ip});
        if (!utility::verifyRoutePresent(
                prevRdsw.value(), prefix, prefixLength)) {
          XLOG(DBG2) << "Route add verification failed: " << prevRdsw.value()
                     << " route: " << prefix.str()
                     << " prefixLength: " << prefixLength;
          return {};
        }
      }
      prevRdsw = rdsw;
    }

    // Add route for first RDSW to complete the loop
    CHECK(!topologyInfo->getRdsws().empty());
    auto lastRdsw = std::prev(topologyInfo->getRdsws().end());
    utility::addRoute(*lastRdsw, prefix, prefixLength, {firstRdswNeighbor.ip});
    if (!utility::verifyRoutePresent(*lastRdsw, prefix, prefixLength)) {
      XLOG(DBG2) << "Route add verification failed: " << *lastRdsw
                 << " route: " << prefix.str()
                 << " prefixLength: " << prefixLength;
      return {};
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
