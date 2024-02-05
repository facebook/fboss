// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

class AgentInNullRouteDiscardsCounterTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::NULL_ROUTE_IN_DISCARDS_COUNTER};
  }

 protected:
  void pumpTraffic(bool isV6) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcIp = folly::IPAddress(isV6 ? "1001::1" : "10.0.0.1");
    auto dstIp = folly::IPAddress(isV6 ? "100:100:100::1" : "100.100.100.1");
    auto pkt = utility::makeUDPTxPacket(
        getSw(), vlanId, intfMac, intfMac, srcIp, dstIp, 10000, 10001);
    getSw()->sendPacketOutOfPortAsync(
        std::move(pkt), PortID(masterLogicalInterfacePortIds()[0]));
  }
};

TEST_F(AgentInNullRouteDiscardsCounterTest, nullRouteHit) {
  auto setup = [=]() {};
  PortID portId = masterLogicalInterfacePortIds()[0];
  auto verify = [=, this]() {
    auto portStatsBefore = getLatestPortStats(portId);
    pumpTraffic(true);
    pumpTraffic(false);
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(portId);
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inDstNullDiscards_() -
              *portStatsBefore.inDstNullDiscards_());
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.inDiscardsRaw_(),
          *portStatsAfter.inDstNullDiscards_());
    });
    // Collect once more and assert that counter remains same.
    // We expect this to be a cumulative counter and not a read
    // on clear counter. Assert that.
    auto portStatsAfter = getLatestPortStats(portId);
    EXPECT_EQ(
        2,
        *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
    EXPECT_EQ(
        2,
        *portStatsAfter.inDstNullDiscards_() -
            *portStatsBefore.inDstNullDiscards_());
    // Assert that other ports did not see any in discard
    // counter increment
    auto allPortStats = getLatestPortStats(masterLogicalInterfacePortIds());
    for (const auto& [port, otherPortStats] : allPortStats) {
      if (port == portId) {
        continue;
      }
      EXPECT_EQ(otherPortStats.inDiscardsRaw_(), 0);
      EXPECT_EQ(otherPortStats.inDiscards_(), 0);
      EXPECT_EQ(otherPortStats.inDstNullDiscards_(), 0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
