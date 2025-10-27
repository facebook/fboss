// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
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
} // namespace facebook::fboss
