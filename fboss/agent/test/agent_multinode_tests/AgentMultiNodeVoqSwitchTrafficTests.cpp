// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchNeighborTests.h"

#include "fboss/agent/packet/PktFactory.h"
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
    auto logAddRoute =
        [](const auto& rdsw, const auto& prefix, const auto& neighbor) {
          XLOG(DBG2) << "Adding route:: " << " prefix: " << prefix.str()
                     << " nexthop: " << neighbor.str() << " to " << rdsw;
        };

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
        logAddRoute(prevRdsw.value(), kPrefix, neighbor);
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
    logAddRoute(*lastRdsw, prefix, firstRdswNeighbor);
    utility::addRoute(*lastRdsw, prefix, prefixLength, {firstRdswNeighbor.ip});
    if (!utility::verifyRoutePresent(*lastRdsw, prefix, prefixLength)) {
      XLOG(DBG2) << "Route add verification failed: " << *lastRdsw
                 << " route: " << prefix.str()
                 << " prefixLength: " << prefixLength;
      return {};
    }

    return rdswToNeighbor;
  }

  void injectTraffic(const utility::Neighbor& neighbor) const {
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
    auto static kSrcIP = folly::IPAddressV6("2001:0db8:85a0::");
    auto [prefix, _] = kGetRoutePrefixAndPrefixLength();
    for (int i = 0; i < 1000; i++) {
      auto txPacket = utility::makeUDPTxPacket(
          getSw(),
          std::nullopt, // vlanIDForTx
          folly::MacAddress("00:02:00:00:01:01"), // srcMac
          utility::getMacForFirstInterfaceWithPorts(
              getSw()->getState()), // dstMac
          kSrcIP,
          prefix, // dstIP
          8000,
          8001,
          0, // ECN
          255, // TTL
          // Payload
          std::vector<uint8_t>(1200, 0xff));

      getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), PortID(neighbor.portID));
    }
  }

  bool setupTrafficLoop(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
    // Configure for Traffic loop
    auto rdswToNeighbor =
        configureNeighborsAndRoutesForTrafficLoop(topologyInfo);
    if (rdswToNeighbor.empty()) {
      return false;
    }

    auto myHostname = topologyInfo->getMyHostname();
    CHECK(rdswToNeighbor.find(myHostname) != rdswToNeighbor.end());
    // Create Traffic loop
    injectTraffic(rdswToNeighbor[myHostname]);

    return true;
  }

 public:
  bool verifyTrafficSpray(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
    XLOG(DBG2) << "Verifying Traffic Spray";

    if (!setupTrafficLoop(topologyInfo)) {
      XLOG(ERR) << "Traffic loop setup failed";
      return false;
    }

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
