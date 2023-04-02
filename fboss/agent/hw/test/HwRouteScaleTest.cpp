/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/test/RouteScaleGenerators.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace facebook::fboss {

class HwRouteScaleTest : public HwTest {
 protected:
  template <typename RouteScaleGeneratorT>
  void runTest(const std::set<PlatformType>& applicablePlatforms) {
    if (applicablePlatforms.find(getPlatform()->getType()) ==
        applicablePlatforms.end()) {
      return;
    }
    if (!getHwSwitchEnsemble()->isRouteScaleEnabled()) {
      return;
    }
    auto setup = [this]() {
      applyNewConfig(utility::onePortPerInterfaceConfig(
          getHwSwitch(), masterLogicalPortIds()));
      auto routeGen = RouteScaleGeneratorT(getProgrammedState());

      applyNewState(routeGen.resolveNextHops(
          getHwSwitchEnsemble()->getProgrammedState()));
      auto routeChunks = routeGen.getThriftRoutes();
      auto updater = getHwSwitchEnsemble()->getRouteUpdater();
      updater.programRoutes(RouterID(0), ClientID::BGPD, routeChunks);
    };
    auto verify = [] {};
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwRouteScaleTest, rswRouteScale) {
  runTest<utility::RSWRouteScaleGenerator>(
      {PlatformType::PLATFORM_WEDGE,
       PlatformType::PLATFORM_WEDGE100,
       PlatformType::PLATFORM_GALAXY_LC,
       PlatformType::PLATFORM_GALAXY_FC,
       PlatformType::PLATFORM_MINIPACK,
       PlatformType::PLATFORM_YAMP,
       PlatformType::PLATFORM_WEDGE400,
       PlatformType::PLATFORM_WEDGE400C,
       PlatformType::PLATFORM_CLOUDRIPPER,
       PlatformType::PLATFORM_ELBERT,
       PlatformType::PLATFORM_FUJI});
}

TEST_F(HwRouteScaleTest, fswRouteScale) {
  runTest<utility::FSWRouteScaleGenerator>(
      {PlatformType::PLATFORM_WEDGE100,
       PlatformType::PLATFORM_GALAXY_LC,
       PlatformType::PLATFORM_GALAXY_FC,
       PlatformType::PLATFORM_MINIPACK,
       PlatformType::PLATFORM_YAMP,
       PlatformType::PLATFORM_WEDGE400,
       PlatformType::PLATFORM_WEDGE400C,
       PlatformType::PLATFORM_CLOUDRIPPER,
       PlatformType::PLATFORM_ELBERT,
       PlatformType::PLATFORM_FUJI});
}

TEST_F(HwRouteScaleTest, thAlpmScale) {
  runTest<utility::THAlpmRouteScaleGenerator>(
      {PlatformType::PLATFORM_WEDGE100,
       PlatformType::PLATFORM_GALAXY_LC,
       PlatformType::PLATFORM_GALAXY_FC,
       PlatformType::PLATFORM_WEDGE400C,
       PlatformType::PLATFORM_CLOUDRIPPER});
}

TEST_F(HwRouteScaleTest, hgridDuScaleTest) {
  runTest<utility::HgridDuRouteScaleGenerator>(
      {PlatformType::PLATFORM_MINIPACK,
       PlatformType::PLATFORM_YAMP,
       PlatformType::PLATFORM_WEDGE400,
       PlatformType::PLATFORM_WEDGE400C,
       PlatformType::PLATFORM_CLOUDRIPPER,
       PlatformType::PLATFORM_ELBERT,
       PlatformType::PLATFORM_FUJI});
}

TEST_F(HwRouteScaleTest, hgridUuScaleTest) {
  runTest<utility::HgridUuRouteScaleGenerator>(
      {PlatformType::PLATFORM_MINIPACK,
       PlatformType::PLATFORM_YAMP,
       PlatformType::PLATFORM_WEDGE400,
       PlatformType::PLATFORM_WEDGE400C,
       PlatformType::PLATFORM_CLOUDRIPPER,
       PlatformType::PLATFORM_ELBERT,
       PlatformType::PLATFORM_FUJI});
}

TEST_F(HwRouteScaleTest, turboFabricScaleTest) {
  runTest<utility::TurboFSWRouteScaleGenerator>(
      {PlatformType::PLATFORM_MINIPACK,
       PlatformType::PLATFORM_YAMP,
       PlatformType::PLATFORM_ELBERT,
       PlatformType::PLATFORM_FUJI});
}

TEST_F(HwRouteScaleTest, anticipatedRouteScaleGenerator) {
  runTest<utility::AnticipatedRouteScaleGenerator>(
      {PlatformType::PLATFORM_MINIPACK,
       PlatformType::PLATFORM_YAMP,
       PlatformType::PLATFORM_WEDGE400,
       PlatformType::PLATFORM_WEDGE400C,
       PlatformType::PLATFORM_CLOUDRIPPER,
       PlatformType::PLATFORM_ELBERT,
       PlatformType::PLATFORM_FUJI});
}
} // namespace facebook::fboss
