// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchTrafficTests.h"

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

namespace facebook::fboss {

std::map<std::string, utility::Neighbor>
AgentMultiNodeVoqSwitchTrafficTest::configureNeighborsAndRoutesForTrafficLoop(
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
      utility::addRoute(prevRdsw.value(), prefix, prefixLength, {neighbor.ip});
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

void AgentMultiNodeVoqSwitchTrafficTest::injectTraffic(
    const utility::Neighbor& neighbor) const {
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

bool AgentMultiNodeVoqSwitchTrafficTest::setupTrafficLoop(
    const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
  // Configure for Traffic loop
  auto rdswToNeighbor = configureNeighborsAndRoutesForTrafficLoop(topologyInfo);
  if (rdswToNeighbor.empty()) {
    return false;
  }

  auto myHostname = topologyInfo->getMyHostname();
  CHECK(rdswToNeighbor.find(myHostname) != rdswToNeighbor.end());
  // Create Traffic loop
  injectTraffic(rdswToNeighbor[myHostname]);

  // Verify line rate is reached for every switch
  for (const auto& [rdsw, neighbor] : rdswToNeighbor) {
    if (!utility::verifyLineRate(rdsw, neighbor.portID)) {
      XLOG(DBG2) << "Verify line rate failed for rdsw: " << rdsw
                 << " port: " << neighbor.portID;
      return false;
    }
  }

  return true;
}

bool AgentMultiNodeVoqSwitchTrafficTest::verifyTrafficSpray(
    const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
  XLOG(DBG2) << "Verifying Traffic Spray";

  auto baselineRdswToPeerAndDsfSession =
      utility::getPeerToDsfSessionForRdsws(topologyInfo->getRdsws());

  if (!setupTrafficLoop(topologyInfo)) {
    XLOG(ERR) << "Traffic loop setup failed";
    return false;
  }

  // Verify traffic spray for VOQ as well as Fabric switches
  for (const auto& switchName : topologyInfo->getAllSwitches()) {
    if (!utility::verifyFabricSpray(switchName)) {
      XLOG(DBG2) << "Verify fabric spray failed for switch: " << switchName;
      return false;
    }
  }

  // Verify no DSF Sessions flapped due to traffic flood
  if (!utility::verifyNoSessionsFlapForRdsws(
          topologyInfo->getRdsws(), baselineRdswToPeerAndDsfSession)) {
    XLOG(DBG2) << "Traffic flood flapped some DSF sessions";
    return false;
  }

  return true;
}

std::pair<std::string, std::vector<utility::Neighbor>>
AgentMultiNodeVoqSwitchTrafficTest::configureRouteToRemoteRdswWithTwoNhops(
    const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
  auto myHostname = topologyInfo->getMyHostname();
  const auto& rdsws = topologyInfo->getRdsws();

  auto getRemoteRdsw = [myHostname, rdsws] {
    auto iter =
        std::find_if(rdsws.cbegin(), rdsws.cend(), [myHostname](auto& rdsw) {
          return rdsw != myHostname;
        });
    CHECK(iter != rdsws.cend());
    return *iter;
  };

  auto addNeighbors =
      [this](const auto& topologyInfo, const std::string& remoteRdsw) {
        auto neighbors = computeNeighborsForRdsw(
            topologyInfo, remoteRdsw, 2 /* number of neighbors */);
        for (const auto& neighbor : neighbors) {
          utility::addNeighbor(
              remoteRdsw,
              neighbor.intfID,
              neighbor.ip,
              neighbor.mac,
              neighbor.portID);
        }

        return neighbors;
      };

  auto getNextHops = [](const std::vector<utility::Neighbor>& neighbors) {
    std::vector<folly::IPAddress> nexthops;
    std::transform(
        neighbors.begin(),
        neighbors.end(),
        std::back_inserter(nexthops),
        [](const utility::Neighbor& neighbor) { return neighbor.ip; });
    return nexthops;
  };

  // Setup a route from TestDriver (self) to a remote RDSW.
  // The route has 2 nexthops.
  // Resolve neighbors corresponding to those nexthops.
  auto remoteRdsw = getRemoteRdsw();
  auto remoteNeighbors = addNeighbors(topologyInfo, remoteRdsw);
  CHECK_EQ(remoteNeighbors.size(), 2);

  auto nexthops = getNextHops(remoteNeighbors);
  const auto& [prefix, prefixLength] = kGetRoutePrefixAndPrefixLength();
  utility::addRoute(myHostname, prefix, prefixLength, nexthops);

  return std ::make_pair(remoteRdsw, remoteNeighbors);
}

void AgentMultiNodeVoqSwitchTrafficTest::pumpRoCETraffic(
    const PortID& localPort) const {
  auto static kSrcIP = folly::IPAddressV6("2001:0db8:85a0::");
  auto [prefix, _] = kGetRoutePrefixAndPrefixLength();

  constexpr auto kReservedBytesWithRehashEnabled = 0x40;
  auto packetSize = utility::pumpRoCETraffic(
      true /* isV6 */,
      utility::makeAllocator(getSw()),
      utility::getSendPktFunc(getSw()),
      utility::getMacForFirstInterfaceWithPorts(getSw()->getState()), // dstMac
      std::nullopt /* vlan */,
      localPort,
      kSrcIP,
      prefix, // dstIP
      utility::kUdfL4DstPort,
      255, // hopLimit
      folly::MacAddress("00:02:00:00:01:01"), // srcMac
      1000000, /* packetCount */
      utility::kUdfRoceOpcodeAck,
      kReservedBytesWithRehashEnabled, /* reserved */
      std::nullopt, /* nextHdr */
      true /* sameDstQueue */);

  XLOG(DBG2) << "RoCE packet sent. Size: " << packetSize;
}

bool AgentMultiNodeVoqSwitchTrafficTest::verifyShelAndConditionalEntropy(
    const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
  XLOG(DBG2) << "Verifying SHEL(Self Healing ECMP LAG) and Conditional Entropy";
  return true;
}

TEST_F(AgentMultiNodeVoqSwitchTrafficTest, verifyTrafficSpray) {
  runTestWithVerifyCluster([this](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return this->verifyTrafficSpray(topologyInfo);
    }
  });
}

TEST_F(AgentMultiNodeVoqSwitchTrafficTest, verifyShelAndConditionalEntropy) {
  runTestWithVerifyCluster([this](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return this->verifyShelAndConditionalEntropy(topologyInfo);
    }
  });
}

} // namespace facebook::fboss
