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

#include "fboss/agent/platforms/tests/utils/CreateTestPlatform.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class HwRouteScaleTest : public HwTest {
 protected:
  template <typename RouteScaleGeneratorT>
  void runTest(const std::set<PlatformMode>& applicablePlatforms) {
    if (applicablePlatforms.find(getPlatform()->getMode()) ==
        applicablePlatforms.end()) {
      return;
    }
    if (!getHwSwitchEnsemble()->isRouteScaleEnabled()) {
      return;
    }
    auto setup = [this]() {
      applyNewConfig(
          utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds()));
      auto states =
          RouteScaleGeneratorT(getProgrammedState()).getSwitchStates();
      applyNewState(states.back());
    };
    auto verify = [] {};
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwRouteScaleTest, rswRouteScale) {
  runTest<utility::RSWRouteScaleGenerator>({PlatformMode::WEDGE,
                                            PlatformMode::WEDGE100,
                                            PlatformMode::GALAXY_LC,
                                            PlatformMode::GALAXY_FC,
                                            PlatformMode::MINIPACK,
                                            PlatformMode::YAMP,
                                            PlatformMode::WEDGE400});
}

TEST_F(HwRouteScaleTest, fswRouteScale) {
  runTest<utility::FSWRouteScaleGenerator>({PlatformMode::WEDGE100,
                                            PlatformMode::GALAXY_LC,
                                            PlatformMode::GALAXY_FC,
                                            PlatformMode::MINIPACK,
                                            PlatformMode::YAMP,
                                            PlatformMode::WEDGE400});
}

TEST_F(HwRouteScaleTest, thAlpmScale) {
  runTest<utility::THAlpmRouteScaleGenerator>({PlatformMode::WEDGE100,
                                               PlatformMode::GALAXY_LC,
                                               PlatformMode::GALAXY_FC});
}

TEST_F(HwRouteScaleTest, hgridDuScaleTest) {
  runTest<utility::HgridDuRouteScaleGenerator>(
      {PlatformMode::MINIPACK, PlatformMode::YAMP, PlatformMode::WEDGE400});
}

TEST_F(HwRouteScaleTest, hgridUuScaleTest) {
  runTest<utility::HgridUuRouteScaleGenerator>(
      {PlatformMode::MINIPACK, PlatformMode::YAMP, PlatformMode::WEDGE400});
}

TEST_F(HwRouteScaleTest, turboFabricScaleTest) {
  runTest<utility::TurboFSWRouteScaleGenerator>({PlatformMode::MINIPACK,
                                                 PlatformMode::YAMP,
                                                 PlatformMode::WEDGE400,
                                                 PlatformMode::WEDGE400C});
}
} // namespace facebook::fboss
