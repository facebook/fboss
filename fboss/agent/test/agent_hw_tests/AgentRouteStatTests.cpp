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
    // verify unique counters
    auto count1Before = getRouteCounterValue(*kCounterID1);
    auto count2Before = getRouteCounterValue(*kCounterID2);
    sendL3Packet(kAddr1, masterLogicalInterfacePortIds()[1]);
    sendL3Packet(kAddr2, masterLogicalInterfacePortIds()[1]);
    WITH_RETRIES({
      auto count1After = getRouteCounterValue(*kCounterID1);
      auto count2After = getRouteCounterValue(*kCounterID2);
      EXPECT_EVENTUALLY_GT(count1After, count1Before);
      EXPECT_EVENTUALLY_GT(count2After, count2Before);
    });

    // verify shared route counters
    auto countBefore = getRouteCounterValue(*kCounterID2);
    sendL3Packet(kAddr2, masterLogicalInterfacePortIds()[1]);
    sendL3Packet(kAddr3, masterLogicalInterfacePortIds()[1]);
    WITH_RETRIES({
      auto countAfter = getRouteCounterValue(*kCounterID2);
      EXPECT_EVENTUALLY_GT(countAfter, countBefore);
    });

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
    // verify counter works
    {
      auto countBefore = getRouteCounterValue(*kCounterID1);
      sendL3Packet(kAddr1, masterLogicalInterfacePortIds()[1]);
      WITH_RETRIES({
        auto countAfter = getRouteCounterValue(*kCounterID1);
        EXPECT_EVENTUALLY_GT(countAfter, countBefore);
      });
    }

    // modify the route - no change to counter id
    addRoute(
        kAddr1,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[1]),
        kCounterID1);
    {
      auto countBefore = getRouteCounterValue(*kCounterID1);
      sendL3Packet(kAddr1, masterLogicalInterfacePortIds()[1]);
      WITH_RETRIES({
        auto countAfter = getRouteCounterValue(*kCounterID1);
        EXPECT_EVENTUALLY_GT(countAfter, countBefore);
      });
    }

    // modify the counter id
    addRoute(
        kAddr1,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID2);
    {
      auto countBefore = getRouteCounterValue(*kCounterID2);
      sendL3Packet(kAddr1, masterLogicalInterfacePortIds()[1]);
      WITH_RETRIES({
        auto countAfter = getRouteCounterValue(*kCounterID2);
        EXPECT_EVENTUALLY_GT(countAfter, countBefore);
      });
    }

    // counter id changing from valid to null
    auto countBefore = getRouteCounterValue(*kCounterID2);
    addRoute(
        kAddr1,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        std::nullopt);
    sendL3Packet(kAddr1, masterLogicalInterfacePortIds()[1]);
    auto countAfter = getRouteCounterValue(*kCounterID2);
    EXPECT_EQ(countAfter, countBefore);

    // counter id changing from null to valid
    addRoute(
        kAddr1,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID2);
    {
      auto cb = getRouteCounterValue(*kCounterID2);
      sendL3Packet(kAddr1, masterLogicalInterfacePortIds()[1]);
      WITH_RETRIES({
        auto ca = getRouteCounterValue(*kCounterID2);
        EXPECT_EVENTUALLY_GT(ca, cb);
      });
    }

    addRoute(
        kAddr1,
        120,
        PortDescriptor(masterLogicalInterfacePortIds()[0]),
        kCounterID1);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
