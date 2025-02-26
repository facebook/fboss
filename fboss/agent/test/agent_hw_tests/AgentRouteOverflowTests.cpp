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
    case PlatformType::PLATFORM_WEDGE:
      /*
       * On BRCM SAI TD2 scales better then TH in ALPM
       * mode. Hence we need 11K + Hgrid scale to actually
       * cause overflow
       */
      utility::programRoutes(
          updater,
          kRid,
          ClientID::BGPD,
          utility::RouteDistributionGenerator(
              getProgrammedState(), {{64, 10000}}, {{24, 1000}}, 20000, 4)
              .getThriftRoutes());
      routeChunks = utility::HgridUuRouteScaleGenerator(getProgrammedState())
                        .getThriftRoutes();
      break;

    case PlatformType::PLATFORM_GALAXY_FC:
    case PlatformType::PLATFORM_GALAXY_LC:
    case PlatformType::PLATFORM_WEDGE100:
      routeChunks = utility::HgridUuRouteScaleGenerator(getProgrammedState())
                        .getThriftRoutes();
      break;
    case PlatformType::PLATFORM_WEDGE400:
    case PlatformType::PLATFORM_WEDGE400_GRANDTETON:
    case PlatformType::PLATFORM_MINIPACK:
    case PlatformType::PLATFORM_YAMP:
    case PlatformType::PLATFORM_DARWIN:
    case PlatformType::PLATFORM_DARWIN48V:
      /*
       * A route distribution 200,000 /128 does overflow the ASIC tables
       * but it takes 15min to generate, program and clean up such a
       * distribution. Since the code paths for overflow in our implementation
       * are platform independent, skip overflow on TH3 in the interest
       * of time.
       * Caveat: if for whatever reason, SDK stopped returning E_FULL on
       * table full on TH3, skipping overflow test here would hide such
       * a bug.
       */
      break;

    case PlatformType::PLATFORM_FAKE_WEDGE:
    case PlatformType::PLATFORM_FAKE_WEDGE40:
    case PlatformType::PLATFORM_FAKE_SAI:
      // No limits to overflow for fakes
      break;
    case PlatformType::PLATFORM_WEDGE400C:
    case PlatformType::PLATFORM_WEDGE400C_SIM:
    case PlatformType::PLATFORM_WEDGE400C_VOQ:
    case PlatformType::PLATFORM_WEDGE400C_FABRIC:
    case PlatformType::PLATFORM_WEDGE400C_GRANDTETON:
    case PlatformType::PLATFORM_CLOUDRIPPER:
    case PlatformType::PLATFORM_CLOUDRIPPER_VOQ:
    case PlatformType::PLATFORM_CLOUDRIPPER_FABRIC:
    case PlatformType::PLATFORM_LASSEN_DEPRECATED:
    case PlatformType::PLATFORM_SANDIA:
    case PlatformType::PLATFORM_MORGAN800CC:
      XLOG(WARNING) << " No overflow test for 400C yet";
      // TODO: Add overflow test for 400C
      break;
    case PlatformType::PLATFORM_FUJI:
    case PlatformType::PLATFORM_ELBERT:
      // No overflow test for TH4 yet
      break;
    case PlatformType::PLATFORM_MERU400BIU:
    case PlatformType::PLATFORM_MERU800BIA:
    case PlatformType::PLATFORM_MERU800BIAB:
    case PlatformType::PLATFORM_MERU800BFA:
    case PlatformType::PLATFORM_MERU800BFA_P1:
      // No overflow test for MERU400BIU yet
      // TODO: DSF deivces take longer time to program routes. We should try and
      // verify the logic.
      break;
    case PlatformType::PLATFORM_MERU400BIA:
      // No overflow test for MERU400BIA yet
      break;
    case PlatformType::PLATFORM_MERU400BFU:
      // No overflow test for MERU400BFU yet
      break;
    case PlatformType::PLATFORM_MONTBLANC:
      break;
    case PlatformType::PLATFORM_JANGA800BIC:
      break;
    case PlatformType::PLATFORM_TAHAN800BC:
      break;
    case PlatformType::PLATFORM_YANGRA:
    case PlatformType::PLATFORM_MINIPACK3N:
      XLOG(WARNING) << " No overflow test for YANGRA/MP3N yet";
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
