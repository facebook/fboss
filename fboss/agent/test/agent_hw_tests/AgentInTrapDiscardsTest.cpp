// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

class AgentInTrapDiscardsCounterTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::TRAP_DISCARDS_COUNTER};
  }

 protected:
  void pumpTraffic(bool isV6) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcIp = folly::IPAddress(isV6 ? "1001::1" : "10.0.0.1");
    // Send packet to null IP destination - this should lead to a
    // trap drop increment.
    auto dstIp = folly::IPAddress(isV6 ? "::" : "0.0.0.0");
    auto pkt = utility::makeUDPTxPacket(
        getSw(), vlanId, intfMac, intfMac, srcIp, dstIp, 10000, 10001);
    getSw()->sendPacketOutOfPortAsync(
        std::move(pkt), PortID(masterLogicalInterfacePortIds()[0]));
  }
};

TEST_F(AgentInTrapDiscardsCounterTest, trapDrops) {
  auto setup = [=]() {};
  PortID portId = masterLogicalInterfacePortIds()[0];
  auto verify = [=, this]() {
    // Delete null route so that we hit the trap drop instead of null
    // route drop. We do this in verify since we unconditionally install
    // null routes on both Warm/Cold boots. And removing null routes in
    // setup, would just cause them to be restored in WB path. This
    // then would fail the test and also trigger unexpected WB writes.
    auto routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.delRoute(
        RouterID(0), folly::IPAddress("::"), 0, ClientID::STATIC_INTERNAL);
    routeUpdater.delRoute(
        RouterID(0), folly::IPAddress("0.0.0.0"), 0, ClientID::STATIC_INTERNAL);
    routeUpdater.program();
    auto portStatsBefore = getLatestPortStats(portId);
    pumpTraffic(true);
    pumpTraffic(false);
    auto portStatsAfter = portStatsBefore;
    WITH_RETRIES({
      portStatsAfter = getLatestPortStats(portId);
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
      EXPECT_EVENTUALLY_TRUE(portStatsAfter.inTrapDiscards_().has_value());
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inTrapDiscards_() -
              portStatsBefore.inTrapDiscards_().value_or(0));
    });
    checkStatsStabilize(
        10,
        [this, portId, portStatsAfter](
            const std::map<uint16_t, multiswitch::HwSwitchStats>& switchStats) {
          auto afterAfterPortStats = extractPortStats(portId, switchStats);
          EXPECT_EQ(
              afterAfterPortStats.inDiscardsRaw_(),
              portStatsAfter.inDiscardsRaw_());
          EXPECT_EQ(
              afterAfterPortStats.inTrapDiscards_(),
              portStatsAfter.inTrapDiscards_());
        });
    getAgentEnsemble()->applyNewState(
        [](const std::shared_ptr<SwitchState>& in) {
          return setupMinAlpmRouteState(in);
        });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
