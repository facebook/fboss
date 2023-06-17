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
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwOverflowTest.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

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
  switch (getPlatform()->getType()) {
    case PlatformType::PLATFORM_WEDGE:
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
    case PlatformType::PLATFORM_LASSEN:
    case PlatformType::PLATFORM_SANDIA:
    case PlatformType::PLATFORM_MORGAN800CC:
      XLOG(WARNING) << " No overflow test for 400C yet";
      break;
    case PlatformType::PLATFORM_FUJI:
    case PlatformType::PLATFORM_ELBERT:
      // No overflow test for TH4 yet
      break;
    case PlatformType::PLATFORM_MERU400BIU:
    case PlatformType::PLATFORM_MERU800BIA:
    case PlatformType::PLATFORM_MERU800BFA:
      // No overflow test for MERU400BIU yet
      break;
    case PlatformType::PLATFORM_MERU400BIA:
      // No overflow test for MERU400BIA yet
      break;
    case PlatformType::PLATFORM_MERU400BFU:
      // No overflow test for MERU400BFU yet
      break;
    case PlatformType::PLATFORM_MONTBLANC:
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
    cfg.switchSettings()->maxRouteCounterIDs() = 1;
    return cfg;
  }
};

TEST_F(HwRouteCounterOverflowTest, overflowRouteCounters) {
  if (!utility::isRouteCounterSupported(getHwSwitch())) {
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
  // Add some V6 prefixes with a counterID
  updater.programRoutes(
      kRid,
      ClientID::BGPD,
      utility::RouteDistributionGenerator(
          getProgrammedState(), {{64, 5}}, {}, 4000, 4)
          .getThriftRoutes(counterID1));
  // Add another set of V6 prefixes with a second counterID
  routeChunks = utility::RouteDistributionGenerator(
                    getProgrammedState(), {{120, 5}}, {}, 4000, 4)
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
