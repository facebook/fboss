// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/agent_hw_tests/AgentOverflowTestBase.h"
#include "fboss/agent/test/utils/RouteTestUtils.h"

namespace facebook::fboss {

class AgentRouteOverflowTest : public AgentOverflowTestBase {};

TEST_F(AgentRouteOverflowTest, overflowRoutes) {
  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return utility::EcmpSetupAnyNPorts6(in).resolveNextHops(
        in, utility::kDefaulEcmpWidth);
  });
  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return utility::EcmpSetupAnyNPorts4(in).resolveNextHops(
        in, utility::kDefaulEcmpWidth);
  });
  utility::RouteDistributionGenerator::ThriftRouteChunks routeChunks;
  auto updater = getSw()->getRouteUpdater();
  const RouterID kRid(0);
  switch (getSw()->getPlatformType()) {
    case PlatformType::PLATFORM_WEDGE100:
      routeChunks = utility::HgridUuRouteScaleGenerator(getProgrammedState())
                        .getThriftRoutes();
      break;
    default:
      XLOG(WARNING) << " No overflow test yet";
      /* Tomahawk3 platforms:
       * A route distribution 200,000 /128 does overflow the ASIC tables
       * but it takes 15min to generate, program and clean up such a
       * distribution. Since the code paths for overflow in our implementation
       * are platform independent, skip overflow on TH3 in the interest
       * of time.
       * Caveat: if for whatever reason, SDK stopped returning E_FULL on
       * table full on TH3, skipping overflow test here would hide such
       * a bug.
       */
      /* DSF platforms:
       * TODO: DSF deivces take longer time to program routes. We should try and
       * verify the logic.
       */
      break;
  }
  if (routeChunks.size() == 0) {
    return;
  }
  {
    startPacketTxRxVerify();
    SCOPE_EXIT {
      stopPacketTxRxVerify();
    };
    EXPECT_THROW(
        utility::programRoutes(updater, kRid, ClientID::BGPD, routeChunks),
        FbossError);

    auto programmedState = getProgrammedState();
    EXPECT_TRUE(programmedState->isPublished());
    auto updater2 = getSw()->getRouteUpdater();
    updater2.program();
    EXPECT_EQ(programmedState, getProgrammedState());
  }
  verifyInvariants();
}

class AgentRouteCounterOverflowTest : public AgentOverflowTestBase {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentOverflowTestBase::initialConfig(ensemble);
    cfg.switchSettings()->maxRouteCounterIDs() = 1;
    return cfg;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::ROUTE_COUNTERS,
        production_features::ProductionFeature::COPP,
        production_features::ProductionFeature::ECMP_LOAD_BALANCER,
        production_features::ProductionFeature::L3_QOS};
  }
};

TEST_F(AgentRouteCounterOverflowTest, overflowRouteCounters) {
  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return utility::EcmpSetupAnyNPorts6(in).resolveNextHops(
        in, utility::kDefaulEcmpWidth);
  });
  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return utility::EcmpSetupAnyNPorts4(in).resolveNextHops(
        in, utility::kDefaulEcmpWidth);
  });
  auto updater = getSw()->getRouteUpdater();
  const RouterID kRid(0);
  auto counterID1 = std::optional<RouteCounterID>("route.counter.0");
  auto counterID2 = std::optional<RouteCounterID>("route.counter.1");
  // Add some V6 prefixes with a counterID
  utility::programRoutes(
      updater,
      kRid,
      ClientID::BGPD,
      utility::RouteDistributionGenerator(
          getProgrammedState(), {{64, 5}}, {}, 4000, 4)
          .getThriftRoutes(counterID1));
  // Add another set of V6 prefixes with a second counterID
  utility::RouteDistributionGenerator::ThriftRouteChunks routeChunks =
      utility::RouteDistributionGenerator(
          getProgrammedState(), {{120, 5}}, {}, 4000, 4)
          .getThriftRoutes(counterID2);

  {
    startPacketTxRxVerify();
    SCOPE_EXIT {
      stopPacketTxRxVerify();
    };
    EXPECT_THROW(
        utility::programRoutes(updater, kRid, ClientID::BGPD, routeChunks),
        FbossError);

    auto programmedState = getProgrammedState();
    EXPECT_TRUE(programmedState->isPublished());
    auto updater2 = getSw()->getRouteUpdater();
    updater2.program();
    EXPECT_EQ(programmedState, getProgrammedState());
  }
  verifyInvariants();
}

} // namespace facebook::fboss
