// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

class AgentEgressForwardingDiscardsCounterTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentHwTest::initialConfig(ensemble);
    auto firstPortId = ensemble.masterLogicalInterfacePortIds()[0];
    for (auto& port : *cfg.ports()) {
      if (PortID(*port.logicalID()) == firstPortId) {
        port.maxFrameSize() = 1500;
      }
    }
    return cfg;
  }
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::EGRESS_FORWARDING_DISCARDS_COUNTER};
  }
};

TEST_F(AgentEgressForwardingDiscardsCounterTest, outForwardingDiscards) {
  auto egressPortDesc = PortDescriptor(masterLogicalInterfacePortIds()[0]);
  auto setup = [=, this]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6(
        getSw()->getState(), getSw()->needL2EntryForNeighbor());
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper6.programRoutes(&wrapper, {egressPortDesc});
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper6.resolveNextHops(in, {egressPortDesc});
    });
  };
  auto verify = [=, this]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6(
        getSw()->getState(), getSw()->needL2EntryForNeighbor());
    auto port = egressPortDesc.phyPortID();
    auto portStatsBefore = getLatestPortStats(port);
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto pkt = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        folly::IPAddress("1001::1"),
        folly::IPAddress("2001::1"),
        4242,
        4242,
        0,
        255,
        std::vector<uint8_t>(6000, 0xff));

    getSw()->sendPacketOutOfPortAsync(
        std::move(pkt), masterLogicalInterfacePortIds()[1]);
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(port);
      XLOG(INFO) << " Out forwarding discards, before:"
                 << *portStatsBefore.outForwardingDiscards_()
                 << " after:" << *portStatsAfter.outForwardingDiscards_()
                 << std::endl
                 << " Out discards, before:" << *portStatsBefore.outDiscards_()
                 << " after:" << *portStatsAfter.outDiscards_() << std::endl
                 << " Out congestion discards, before:"
                 << *portStatsBefore.outCongestionDiscardPkts_()
                 << " after:" << *portStatsAfter.outCongestionDiscardPkts_();
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.outDiscards_(), *portStatsBefore.outDiscards_() + 1);
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.outForwardingDiscards_(),
          *portStatsBefore.outForwardingDiscards_() + 1);
      // Out congestion discards should not increment
      EXPECT_EQ(
          *portStatsAfter.outCongestionDiscardPkts_(),
          *portStatsBefore.outCongestionDiscardPkts_());
    });
    // Collect once more and assert that counter remains same.
    // We expect this to be a cumulative counter and not a read
    // on clear counter. Assert that.
    auto portStatsAfter = getLatestPortStats(port);
    XLOG(INFO) << " Out forwading discards, before:"
               << *portStatsBefore.outForwardingDiscards_()
               << " after:" << *portStatsAfter.outForwardingDiscards_()
               << std::endl
               << " Out dicards, before:" << *portStatsBefore.outDiscards_()
               << " after:" << *portStatsAfter.outDiscards_() << std::endl
               << " Out congestion discards, before:"
               << *portStatsBefore.outCongestionDiscardPkts_()
               << " after:" << *portStatsAfter.outCongestionDiscardPkts_();
    EXPECT_EQ(
        *portStatsAfter.outDiscards_(), *portStatsBefore.outDiscards_() + 1);
    EXPECT_EQ(
        *portStatsAfter.outForwardingDiscards_(),
        *portStatsBefore.outForwardingDiscards_() + 1);
    // Out congestion discards should NOT increment
    EXPECT_EQ(
        *portStatsAfter.outCongestionDiscardPkts_(),
        *portStatsBefore.outCongestionDiscardPkts_());
    // Assert that other ports did not see any in discard
    // counter increment
    auto allPortStats = getLatestPortStats(masterLogicalInterfacePortIds());
    for (const auto& [p, otherPortStats] : allPortStats) {
      if (port == port) {
        continue;
      }
      EXPECT_EQ(otherPortStats.outDiscards_(), 0);
      EXPECT_EQ(otherPortStats.outForwardingDiscards_(), 0);
      EXPECT_EQ(otherPortStats.outCongestionDiscardPkts_(), 0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
