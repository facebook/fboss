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
#include "fboss/agent/test/EcmpSetupHelper.h"

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

      applyNewState(utility::EcmpSetupAnyNPorts6(getProgrammedState())
                        .resolveNextHops(
                            getProgrammedState(), utility::kDefaulEcmpWidth));
      applyNewState(utility::EcmpSetupAnyNPorts4(getProgrammedState())
                        .resolveNextHops(
                            getProgrammedState(), utility::kDefaulEcmpWidth));
      auto routeChunks = RouteScaleGeneratorT(
                             getProgrammedState(),
                             getHwSwitchEnsemble()->isStandaloneRibEnabled())
                             .get();
      HwSwitchEnsembleRouteUpdateWrapper updater(getHwSwitchEnsemble());
      for (const auto& routeChunk : routeChunks) {
        for (const auto& route : routeChunk) {
          RouteNextHopSet nhops;
          for (const auto& ip : route.nhops) {
            nhops.emplace(UnresolvedNextHop(ip, ECMP_WEIGHT));
          }
          updater.addRoute(
              RouterID(0),
              route.prefix.first,
              route.prefix.second,
              ClientID::BGPD,
              RouteNextHopEntry(nhops, AdminDistance::EBGP));
        }
      }
      updater.program();
    };
    auto verify = [] {};
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwRouteScaleTest, rswRouteScale) {
  runTest<utility::RSWRouteScaleGenerator>(
      {PlatformMode::WEDGE,
       PlatformMode::WEDGE100,
       PlatformMode::GALAXY_LC,
       PlatformMode::GALAXY_FC,
       PlatformMode::MINIPACK,
       PlatformMode::YAMP,
       PlatformMode::WEDGE400,
       PlatformMode::ELBERT,
       PlatformMode::FUJI});
}

TEST_F(HwRouteScaleTest, fswRouteScale) {
  runTest<utility::FSWRouteScaleGenerator>(
      {PlatformMode::WEDGE100,
       PlatformMode::GALAXY_LC,
       PlatformMode::GALAXY_FC,
       PlatformMode::MINIPACK,
       PlatformMode::YAMP,
       PlatformMode::WEDGE400,
       PlatformMode::ELBERT,
       PlatformMode::FUJI});
}

TEST_F(HwRouteScaleTest, thAlpmScale) {
  runTest<utility::THAlpmRouteScaleGenerator>(
      {PlatformMode::WEDGE100,
       PlatformMode::GALAXY_LC,
       PlatformMode::GALAXY_FC});
}

TEST_F(HwRouteScaleTest, hgridDuScaleTest) {
  runTest<utility::HgridDuRouteScaleGenerator>(
      {PlatformMode::MINIPACK,
       PlatformMode::YAMP,
       PlatformMode::WEDGE400,
       PlatformMode::ELBERT,
       PlatformMode::FUJI});
}

TEST_F(HwRouteScaleTest, hgridUuScaleTest) {
  runTest<utility::HgridUuRouteScaleGenerator>(
      {PlatformMode::MINIPACK,
       PlatformMode::YAMP,
       PlatformMode::WEDGE400,
       PlatformMode::ELBERT,
       PlatformMode::FUJI});
}

TEST_F(HwRouteScaleTest, turboFabricScaleTest) {
  runTest<utility::TurboFSWRouteScaleGenerator>(
      {PlatformMode::MINIPACK,
       PlatformMode::YAMP,
       PlatformMode::WEDGE400,
       PlatformMode::WEDGE400C,
       PlatformMode::ELBERT,
       PlatformMode::FUJI});
}
} // namespace facebook::fboss
