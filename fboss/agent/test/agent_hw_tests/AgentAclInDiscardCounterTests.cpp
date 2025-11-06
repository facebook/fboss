// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

class AgentAclInDiscardsCounterTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::ACL_PORT_IN_DISCARDS_COUNTER};
  }

 private:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentHwTest::initialConfig(ensemble);
    cfg::AclEntry acl;
    acl.name() = "block all";
    acl.actionType() = cfg::AclActionType::DENY;
    acl.dstIp() = "::/0";

    utility::addAcl(&cfg, acl, cfg::AclStage::INGRESS);
    return cfg;
  }
};

TEST_F(AgentAclInDiscardsCounterTest, aclInDiscards) {
  auto setup = [=, this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6(
        getSw()->getState(), getSw()->needL2EntryForNeighbor());
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper6.programRoutes(&wrapper, 1);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper6.resolveNextHops(in, {ecmpHelper6.nhop(0).portDesc});
    });
  };
  auto verify = [=, this]() {
    auto port = masterLogicalInterfacePortIds()[1];
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
        4242);
    getSw()->sendPacketOutOfPortAsync(std::move(pkt), port, std::nullopt);
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(port);
      XLOG(INFO) << " In discards, before:" << *portStatsBefore.inDiscards_()
                 << " after:" << *portStatsAfter.inDiscards_() << std::endl
                 << " Acl discards, before:"
                 << *portStatsBefore.inAclDiscards_()
                 << " after:" << *portStatsAfter.inAclDiscards_();
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.inDiscards_(), *portStatsBefore.inDiscards_() + 1);
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.inAclDiscards_(),
          *portStatsBefore.inAclDiscards_() + 1);
    });
    // Collect once more and assert that counter remains same.
    // We expect this to be a cumulative counter and not a read
    // on clear counter. Assert that.
    auto portStatsAfter = getLatestPortStats(port);
    XLOG(INFO) << " In discards, before:" << *portStatsBefore.inDiscards_()
               << " after:" << *portStatsAfter.inDiscards_() << std::endl
               << " Acl discards, before:" << *portStatsBefore.inAclDiscards_()
               << " after:" << *portStatsAfter.inAclDiscards_();
    EXPECT_EQ(
        *portStatsAfter.inAclDiscards_(),
        *portStatsBefore.inAclDiscards_() + 1);
    // Assert that other ports did not see any in discard
    // counter increment
    auto allPortStats = getLatestPortStats(masterLogicalInterfacePortIds());
    for (const auto& [otherPort, otherPortStats] : allPortStats) {
      if (otherPort == masterLogicalInterfacePortIds()[1]) {
        continue;
      }
      EXPECT_EQ(*otherPortStats.inDiscards_(), 0);
      EXPECT_EQ(*otherPortStats.inAclDiscards_(), 0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
