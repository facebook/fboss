// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/agent_hw_tests/AgentOverflowTestBase.h"
#include "fboss/agent/test/utils/RouteTestUtils.h"

namespace facebook::fboss {

class AgentRouteOverflowTest : public AgentOverflowTestBase {};

TEST_F(AgentRouteOverflowTest, overflowRoutes) {
  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return utility::EcmpSetupAnyNPorts6(in, getSw()->needL2EntryForNeighbor())
        .resolveNextHops(in, utility::kDefaulEcmpWidth);
  });
  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return utility::EcmpSetupAnyNPorts4(in, getSw()->needL2EntryForNeighbor())
        .resolveNextHops(in, utility::kDefaulEcmpWidth);
  });
  utility::RouteDistributionGenerator::ThriftRouteChunks routeChunks;
  auto updater = getSw()->getRouteUpdater();
  const RouterID kRid(0);
  switch (getSw()->getPlatformType()) {
    case PlatformType::PLATFORM_WEDGE100:
      routeChunks = utility::HgridUuRouteScaleGenerator(
                        getProgrammedState(), getSw()->needL2EntryForNeighbor())
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

} // namespace facebook::fboss
