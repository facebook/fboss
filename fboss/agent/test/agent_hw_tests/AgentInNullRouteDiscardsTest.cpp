// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

using facebook::fboss::utility::PfcBufferParams;

class AgentInNullRouteDiscardsCounterTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

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
  auto setup = [&]() {
    if (isSupportedOnAllAsics(HwAsic::Feature::PFC)) {
      // Setup buffer configurations only if PFC is supported
      cfg::SwitchConfig cfg = initialConfig(*getAgentEnsemble());
      std::vector<PortID> portIds = {masterLogicalInterfacePortIds()[0]};
      std::vector<int> losslessPgIds = {2};
      // Make sure default traffic goes to PG2, which is lossless
      const std::map<int, int> tcToPgOverride{{0, 2}};
      const auto bufferParams = PfcBufferParams{};
      utility::setupPfcBuffers(
          getAgentEnsemble(),
          cfg,
          portIds,
          losslessPgIds,
          tcToPgOverride,
          bufferParams);
      applyNewConfig(cfg);
    }
  };
  PortID portId = masterLogicalInterfacePortIds()[0];
  auto verify = [=, this]() {
    auto isVoqSwitch =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getSwitchType() == cfg::SwitchType::VOQ;
    auto portStatsBefore = getLatestPortStats(portId);
    auto switchDropStatsBefore = getAggregatedSwitchDropStats();
    pumpTraffic(true);
    pumpTraffic(false);
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(portId);
      auto switchDropStatsAfter = getAggregatedSwitchDropStats();

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
      // Route discards should not increment congestion discards
      EXPECT_EQ(
          *portStatsAfter.inCongestionDiscards_(),
          *portStatsBefore.inCongestionDiscards_());
      if (isVoqSwitch) {
        EXPECT_EVENTUALLY_EQ(
            *switchDropStatsAfter.ingressPacketPipelineRejectDrops() -
                *switchDropStatsBefore.ingressPacketPipelineRejectDrops(),
            2);
        // Pipeline reject drop, not a queue resolution drop,
        // which happens say when a pkt comes in with a non router
        // MAC
        EXPECT_EQ(
            *switchDropStatsAfter.queueResolutionDrops() -
                *switchDropStatsBefore.queueResolutionDrops(),
            0);
      }
    });
    // Collect once more and assert that counter remains same.
    // We expect this to be a cumulative counter and not a read
    // on clear counter. Assert that.
    auto portStatsAfter = getNextUpdatedPortStats(portId);
    auto switchDropStatsAfter = getAggregatedSwitchDropStats();
    EXPECT_EQ(
        2,
        *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
    EXPECT_EQ(
        2,
        *portStatsAfter.inDstNullDiscards_() -
            *portStatsBefore.inDstNullDiscards_());
    if (isVoqSwitch) {
      EXPECT_EQ(
          *switchDropStatsAfter.ingressPacketPipelineRejectDrops() -
              *switchDropStatsBefore.ingressPacketPipelineRejectDrops(),
          2);
      // Pipeline reject drop, not a queue resolution drop,
      // which happens say when a pkt comes in with a non router
      // MAC
      EXPECT_EQ(
          *switchDropStatsAfter.queueResolutionDrops() -
              *switchDropStatsBefore.queueResolutionDrops(),
          0);
    }
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
