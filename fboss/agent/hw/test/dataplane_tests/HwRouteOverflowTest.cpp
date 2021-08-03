/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/test/dataplane_tests/HwOverflowTest.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

namespace facebook::fboss {

TEST_F(HwOverflowTest, overflowRoutes) {
  applyNewState(
      utility::EcmpSetupAnyNPorts6(getProgrammedState())
          .resolveNextHops(getProgrammedState(), utility::kDefaulEcmpWidth));
  applyNewState(
      utility::EcmpSetupAnyNPorts4(getProgrammedState())
          .resolveNextHops(getProgrammedState(), utility::kDefaulEcmpWidth));
  utility::RouteDistributionGenerator::ThriftRouteChunks routeChunks;
  auto updater = getHwSwitchEnsemble()->getRouteUpdater();
  const RouterID kRid(0);
  switch (getPlatform()->getMode()) {
    case PlatformMode::WEDGE:
      /*
       * On BRCM SAI TD2 scales better then TH in ALPM
       * mode. Hence we need 11K + Hgrid scale to actually
       * cause overflow
       */
      updater.programRoutes(
          kRid,
          ClientID::BGPD,
          utility::RouteDistributionGenerator(
              getProgrammedState(), {{64, 10000}}, {{24, 1000}}, 20000, 4)
              .getThriftRoutes());
      routeChunks = utility::HgridUuRouteScaleGenerator(getProgrammedState())
                        .getThriftRoutes();
      break;

    case PlatformMode::GALAXY_FC:
    case PlatformMode::GALAXY_LC:
    case PlatformMode::WEDGE100:
      routeChunks = utility::HgridUuRouteScaleGenerator(getProgrammedState())
                        .getThriftRoutes();
      break;
    case PlatformMode::WEDGE400:
    case PlatformMode::MINIPACK:
    case PlatformMode::YAMP:
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

    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
      // No limits to overflow for fakes
      break;
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
    case PlatformMode::CLOUDRIPPER:
      XLOG(WARNING) << " No overflow test for 400C yet";
      break;
    case PlatformMode::FUJI:
    case PlatformMode::ELBERT:
      // No overflow test for TH4 yet
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
        updater.programRoutes(kRid, ClientID::BGPD, routeChunks),
        FbossHwUpdateError);

    auto programmedState = getProgrammedState();
    EXPECT_TRUE(programmedState->isPublished());
    auto updater2 = getHwSwitchEnsemble()->getRouteUpdater();
    updater2.program();
    EXPECT_EQ(programmedState, getProgrammedState());
  }
  verifyInvariants();
}

class HwRouteCounterOverflowTest : public HwOverflowTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = HwOverflowTest::initialConfig();
    cfg.switchSettings_ref()->maxRouteCounterIDs_ref() = 1;
    return cfg;
  }
};

TEST_F(HwRouteCounterOverflowTest, overflowRouteCounters) {
  if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::ROUTE_COUNTERS)) {
    return;
  }
  applyNewState(
      utility::EcmpSetupAnyNPorts6(getProgrammedState())
          .resolveNextHops(getProgrammedState(), utility::kDefaulEcmpWidth));
  applyNewState(
      utility::EcmpSetupAnyNPorts4(getProgrammedState())
          .resolveNextHops(getProgrammedState(), utility::kDefaulEcmpWidth));
  utility::RouteDistributionGenerator::ThriftRouteChunks routeChunks;
  auto updater = getHwSwitchEnsemble()->getRouteUpdater();
  const RouterID kRid(0);
  auto counterID1 = std::optional<RouteCounterID>("route.counter.0");
  auto counterID2 = std::optional<RouteCounterID>("route.counter.1");
  updater.programRoutes(
      kRid,
      ClientID::BGPD,
      utility::RouteDistributionGenerator(
          getProgrammedState(), {{64, 5}}, {}, 4000, 4)
          .getThriftRoutes(counterID1));
  routeChunks = utility::RouteDistributionGenerator(
                    getProgrammedState(), {}, {{24, 5}}, 4000, 4)
                    .getThriftRoutes(counterID2);

  {
    startPacketTxRxVerify();
    SCOPE_EXIT {
      stopPacketTxRxVerify();
    };
    EXPECT_THROW(
        updater.programRoutes(kRid, ClientID::BGPD, routeChunks),
        FbossHwUpdateError);

    auto programmedState = getProgrammedState();
    EXPECT_TRUE(programmedState->isPublished());
    auto updater2 = getHwSwitchEnsemble()->getRouteUpdater();
    updater2.program();
    EXPECT_EQ(programmedState, getProgrammedState());
  }
  verifyInvariants();
}
} // namespace facebook::fboss
