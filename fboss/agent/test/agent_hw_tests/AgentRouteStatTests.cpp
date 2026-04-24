// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/IPAddressV6.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
const folly::IPAddressV6 kAddr1{"2401::201:ab00"};
const folly::IPAddressV6 kAddr2{"2401::201:ac00"};
const folly::IPAddressV6 kAddr3{"2401::201:ad00"};
const std::optional<facebook::fboss::RouteCounterID> kCounterID1(
    "route.counter.0");
const std::optional<facebook::fboss::RouteCounterID> kCounterID2(
    "route.counter.1");
constexpr int kUdpSrcPort = 4049;
constexpr int kUdpDstPort = 4050;
} // namespace

namespace facebook::fboss {

class AgentRouteStatTest : public AgentHwTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::ROUTE_COUNTERS};
  }

  void SetUp() override {
    AgentHwTest::SetUp();
    if (!FLAGS_list_production_feature) {
      setupEcmpHelper();
    }
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    config.switchSettings()->maxRouteCounterIDs() = 3;
    return config;
  }

  void addRoute(
      folly::IPAddressV6 prefix,
      uint8_t mask,
      PortDescriptor port,
      std::optional<RouteCounterID> counterID) {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper_->resolveNextHops(in, {port});
    });
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper_->programRoutes(
        &wrapper,
        {port},
        {RoutePrefixV6{prefix, mask}},
        {},
        std::move(counterID));
  }

  size_t sendL3Packet(folly::IPAddressV6 dst, PortID from) {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);

    auto pkt = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        folly::IPAddressV6("1::10"),
        dst,
        kUdpSrcPort,
        kUdpDstPort);
    auto length = pkt->buf()->computeChainDataLength();
    getSw()->sendPacketOutOfPortAsync(std::move(pkt), from, std::nullopt);
    return length;
  }

  uint64_t getRouteCounterValue(const RouteCounterID& counterID) {
    auto hwSwitchStats = getHwSwitchStats();
    auto& hwCounters = *hwSwitchStats.counterStats()->hwCounters();
    auto it = hwCounters.find(counterID);
    if (it == hwCounters.end()) {
      return 0;
    }
    return it->second.bytes().value_or(0);
  }

  // Send packets and verify that the specified counters increment.
  // Each entry in packets is (dst address, counter ID to check).
  // Multiple packets can map to the same counter ID (shared counters).
  void sendAndVerifyCounterIncrement(
      const std::vector<std::pair<folly::IPAddressV6, RouteCounterID>>& packets,
      PortID srcPort) {
    // Snapshot all unique counters before sending
    std::map<RouteCounterID, uint64_t> countersBefore;
    for (const auto& [dst, counterID] : packets) {
      if (!countersBefore.count(counterID)) {
        countersBefore[counterID] = getRouteCounterValue(counterID);
      }
    }

    // Send all packets
    for (const auto& [dst, counterID] : packets) {
      sendL3Packet(dst, srcPort);
    }

    // Verify all counters incremented
    WITH_RETRIES({
      for (const auto& [counterID, before] : countersBefore) {
        auto after = getRouteCounterValue(counterID);
        EXPECT_EVENTUALLY_GT(after, before);
      }
    });
  }

  // Send packets and verify that the specified counters do NOT increment.
  void sendAndVerifyCounterUnchanged(
      const std::vector<std::pair<folly::IPAddressV6, RouteCounterID>>& packets,
      PortID srcPort) {
    std::map<RouteCounterID, uint64_t> countersBefore;
    for (const auto& [dst, counterID] : packets) {
      if (!countersBefore.count(counterID)) {
        countersBefore[counterID] = getRouteCounterValue(counterID);
      }
    }

    for (const auto& [dst, counterID] : packets) {
      sendL3Packet(dst, srcPort);
    }

    for (const auto& [counterID, before] : countersBefore) {
      auto after = getRouteCounterValue(counterID);
      EXPECT_EQ(after, before);
    }
  }

  void setupEcmpHelper() {
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
  }

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper_;
};

TEST_F(AgentRouteStatTest, RouteEntryTest) {
  auto setup = [this]() {
    setupEcmpHelper();
    addRoute(
        kAddr1,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID1);
    addRoute(
        kAddr2,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID2);
    addRoute(
        kAddr3,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID2);
  };
  auto verify = [this]() {
    auto srcPort = masterLogicalInterfacePortIds()[1];

    // verify unique counters
    sendAndVerifyCounterIncrement(
        {{kAddr1, *kCounterID1}, {kAddr2, *kCounterID2}}, srcPort);

    // verify shared route counters (two routes share kCounterID2)
    sendAndVerifyCounterIncrement(
        {{kAddr2, *kCounterID2}, {kAddr3, *kCounterID2}}, srcPort);

    // verify counters exist in stats
    auto hwSwitchStats = getHwSwitchStats();
    auto& hwCounters = *hwSwitchStats.counterStats()->hwCounters();
    EXPECT_TRUE(hwCounters.count(*kCounterID1));
    EXPECT_TRUE(hwCounters.count(*kCounterID2));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentRouteStatTest, CounterModify) {
  auto setup = [this]() {
    setupEcmpHelper();
    addRoute(
        kAddr1,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID1);
    addRoute(
        kAddr2,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID2);
  };
  auto verify = [this]() {
    auto origRoutePort = masterLogicalInterfacePortIds()[0];
    auto newRoutePort = masterLogicalInterfacePortIds()[1];

    // verify counter works
    sendAndVerifyCounterIncrement({{kAddr1, *kCounterID1}}, newRoutePort);

    // modify the route - no change to counter id
    addRoute(kAddr1, 120, PortDescriptor(newRoutePort), kCounterID1);
    sendAndVerifyCounterIncrement({{kAddr1, *kCounterID1}}, origRoutePort);

    // modify the counter id
    addRoute(kAddr1, 120, PortDescriptor(newRoutePort), kCounterID2);
    sendAndVerifyCounterIncrement({{kAddr1, *kCounterID2}}, origRoutePort);

    // counter id changing from valid to null
    addRoute(kAddr1, 120, PortDescriptor(newRoutePort), std::nullopt);
    sendAndVerifyCounterUnchanged({{kAddr1, *kCounterID2}}, origRoutePort);

    // counter id changing from null to valid
    addRoute(kAddr1, 120, PortDescriptor(newRoutePort), kCounterID2);
    sendAndVerifyCounterIncrement({{kAddr1, *kCounterID2}}, origRoutePort);

    // Restore the route to original state, so verify
    // can be run multiple times
    addRoute(kAddr1, 120, PortDescriptor(origRoutePort), kCounterID1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
