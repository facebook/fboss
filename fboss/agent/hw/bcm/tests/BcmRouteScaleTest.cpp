/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/platforms/test_platforms/CreateTestPlatform.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook {
namespace fboss {

class BcmRouteScaleTest : public BcmTest {
 protected:
  template <typename RouteScaleGeneratorT>
  void runTest(const std::set<PlatformMode>& applicablePlatforms) {
    if (applicablePlatforms.find(getPlatformMode()) ==
        applicablePlatforms.end()) {
      return;
    }
    auto setup = [this]() {
      applyNewConfig(
          utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds()));
      auto states = RouteScaleGeneratorT(getProgrammedState()).get();
      applyNewState(states.back());
    };
    auto verify = [] {};
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(BcmRouteScaleTest, fswRouteScale) {
  runTest<utility::FSWRouteScaleGenerator>({PlatformMode::WEDGE100,
                                            PlatformMode::GALAXY_LC,
                                            PlatformMode::GALAXY_FC,
                                            PlatformMode::MINIPACK,
                                            PlatformMode::YAMP,
                                            PlatformMode::WEDGE400DQ});
}

TEST_F(BcmRouteScaleTest, thAlpmScale) {
  if (!getHwSwitch()->isAlpmEnabled()) {
    return;
  }
  runTest<utility::THAlpmRouteScaleGenerator>({PlatformMode::WEDGE100,
                                               PlatformMode::GALAXY_LC,
                                               PlatformMode::GALAXY_FC});
}

TEST_F(BcmRouteScaleTest, hgridDuScaleTest) {
  runTest<utility::HgridDuRouteScaleGenerator>(
      {PlatformMode::MINIPACK, PlatformMode::YAMP, PlatformMode::WEDGE400DQ});
}

TEST_F(BcmRouteScaleTest, hgridUuScaleTest) {
  runTest<utility::HgridUuRouteScaleGenerator>(
      {PlatformMode::MINIPACK, PlatformMode::YAMP, PlatformMode::WEDGE400DQ});
}
} // namespace fboss
} // namespace facebook
