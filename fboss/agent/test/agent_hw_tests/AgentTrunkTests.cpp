// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/TrunkTestUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace facebook::fboss {
class AgentTrunkTest : public AgentHwTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::oneL3IntfTwoPortConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1]);
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::LAG};
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& config) {
    applyNewConfig(config);
    applyNewState(
        [](const std::shared_ptr<SwitchState> state) {
          return utility::enableTrunkPorts(state);
        },
        "enable trunk ports");
  }
};

TEST_F(AgentTrunkTest, TrunkCreateHighLowKeyIds) {
  auto setup = [=, this]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    utility::addAggPort(
        std::numeric_limits<AggregatePortID>::max(),
        {masterLogicalPortIds()[0]},
        &cfg);
    utility::addAggPort(1, {masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=, this]() {
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          utility::getAggregatePortCount(*getAgentEnsemble()), 2);
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePort(
              *getAgentEnsemble(), AggregatePortID(1)));
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePort(
              *getAgentEnsemble(),
              AggregatePortID(std::numeric_limits<AggregatePortID>::max())));
      auto aggIDs = {
          AggregatePortID(1),
          AggregatePortID(std::numeric_limits<AggregatePortID>::max())};
      for (auto aggId : aggIDs) {
        EXPECT_EVENTUALLY_TRUE(
            utility::verifyAggregatePortMemberCount(
                *getAgentEnsemble(), aggId, 1));
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTrunkTest, TrunkCheckIngressPktAggPort) {
  auto setup = [=, this]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    utility::addAggPort(
        std::numeric_limits<AggregatePortID>::max(),
        {masterLogicalPortIds()[0]},
        &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=, this]() {
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyPktFromAggregatePort(
              *getAgentEnsemble(),
              AggregatePortID(std::numeric_limits<AggregatePortID>::max())));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTrunkTest, TrunkMemberPortDownMinLinksViolated) {
  auto aggId = AggregatePortID(std::numeric_limits<AggregatePortID>::max());

  auto setup = [=, this]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    utility::addAggPort(
        aggId, {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);

    // Member port count should drop to 1 now.
  };
  auto verify = [=, this]() {
    bringDownPort(PortID(masterLogicalPortIds()[0]));
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          utility::getAggregatePortCount(*getAgentEnsemble()), 1);
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePort(*getAgentEnsemble(), aggId));
      EXPECT_EVENTUALLY_TRUE(
          utility::verifyAggregatePortMemberCount(
              *getAgentEnsemble(), aggId, 1));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTrunkTest, TrunkPortStats) {
  auto setup = [=, this]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    utility::addAggPort(1, {masterLogicalPortIds()[1]}, &cfg);
    applyConfigAndEnableTrunks(cfg);
    auto ecmpHelper = utility::EcmpSetupTargetedPorts6(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          return ecmpHelper.resolveNextHops(
              in, {PortDescriptor(AggregatePortID(1))});
        },
        "resolve next hops");
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater, {PortDescriptor(AggregatePortID(1))});
  };
  auto verify = [=, this]() {
    const std::string kTrunkName = "AGG-1";
    auto vlanId = VlanID(utility::kBaseVlanId);
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    for (auto throughPort : {false, true}) {
      auto portStats = getLatestPortStats(masterLogicalPortIds()[1]);
      auto portPkts0 = *portStats.outUnicastPkts_() +
          *portStats.outMulticastPkts_() + *portStats.outBroadcastPkts_();

      auto getTrunkOutPkts = [&](const std::string& trunkName) -> int64_t {
        auto hwStats = getHwSwitchStats();
        for (const auto& [_, switchStats] : hwStats) {
          auto it = switchStats.hwTrunkStats()->find(trunkName);
          if (it != switchStats.hwTrunkStats()->end()) {
            return *it->second.outUnicastPkts_() +
                *it->second.outMulticastPkts_() +
                *it->second.outBroadcastPkts_();
          }
        }
        return 0;
      };

      int64_t trunkPkts0 = getTrunkOutPkts(kTrunkName);

      auto pkt = utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          intfMac,
          intfMac,
          folly::IPAddress("2401::1"),
          folly::IPAddress("2401::2"),
          10001,
          20001);
      throughPort
          ? getAgentEnsemble()->ensureSendPacketOutOfPort(
                std::move(pkt), masterLogicalPortIds()[0])
          : getAgentEnsemble()->ensureSendPacketSwitched(std::move(pkt));

      portStats = getLatestPortStats(masterLogicalPortIds()[1]);
      auto portPkts1 = *portStats.outUnicastPkts_() +
          *portStats.outMulticastPkts_() + *portStats.outBroadcastPkts_();

      int64_t trunkPkts1 = getTrunkOutPkts(kTrunkName);

      EXPECT_GT(portPkts1, portPkts0);
      EXPECT_GT(trunkPkts1, trunkPkts0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
