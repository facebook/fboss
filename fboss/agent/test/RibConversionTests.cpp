/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StandaloneRibConversions.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/sim/SimPlatform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/MacAddress.h>
#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

template <typename Generator>
static void runConversionTest() {
  auto constexpr kEcmpWidth = 4;

  SimPlatform plat(folly::MacAddress(), 128);
  std::vector<PortID> ports;
  for (int i = 0; i < 128; ++i) {
    ports.push_back(PortID(i));
  }
  cfg::SwitchConfig config =
      utility::onePortPerVlanConfig(plat.getHwSwitch(), ports);
  auto testHandle =
      createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
  auto sw = testHandle->getSw();

  // TODO(sas): This needs to go away after we are done transitionning to the
  // new RIB mechanism. RouteDistributionGenerator expects `RouteTables` to
  // have an entry for VRF 0 even if we are not using `RouteTables` at all
  // (i.e.: when the ENABLE_STANDALONE_RIB flag is set).
  sw->updateStateBlocking(
      "add VRF0", [=](const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};
        auto newRouteTables = newState->getRouteTables()->modify(&newState);
        newRouteTables->addRouteTable(
            std::make_shared<RouteTable>(RouterID(0)));
        return newState;
      });

  auto generator = Generator(sw->getAppliedState(), 1337, kEcmpWidth);
  const auto& states = generator.getSwitchStates();
  auto state = states[states.size() - 1];

  auto swStateTables = state->getRouteTables();
  auto standaloneRib = switchStateToStandaloneRib(swStateTables);
  auto swStateRib = standaloneToSwitchStateRib(standaloneRib);
  EXPECT_EQ(swStateRib->toFollyDynamic(), swStateTables->toFollyDynamic());

  syncFibWithStandaloneRib(standaloneRib, sw);

  size_t ribRouteCount = 0;
  for (auto routerID : standaloneRib.getVrfList()) {
    auto table = swStateTables->getRouteTable(routerID);
    for (const auto& entry : *table->getRibV4()->routes()) {
      if (entry->isResolved())
        ++ribRouteCount;
    }
    for (const auto& entry : *table->getRibV6()->routes()) {
      if (entry->isResolved())
        ++ribRouteCount;
    }
  }

  size_t fibRouteCount = 0;
  const auto& fibs = sw->getState()->getFibs();
  for (auto routerID : standaloneRib.getVrfList()) {
    auto container = fibs->getFibContainer(routerID);
    fibRouteCount += container->getFibV4()->size();
    fibRouteCount += container->getFibV6()->size();
  }

  EXPECT_EQ(fibRouteCount, ribRouteCount);
}

#if defined(__OPTIMIZE__)
#define OPTIMIZED_ONLY_TEST(CATEGORY, NAME) TEST(CATEGORY, NAME)
#else
#define OPTIMIZED_ONLY_TEST(CATEGORY, NAME) TEST(CATEGORY, DISABLED_##NAME)
#endif

OPTIMIZED_ONLY_TEST(RibConversions, RibConversionFSW) {
  runConversionTest<utility::FSWRouteScaleGenerator>();
}

OPTIMIZED_ONLY_TEST(RibConversions, RibConversionTHAlpm) {
  runConversionTest<utility::THAlpmRouteScaleGenerator>();
}

OPTIMIZED_ONLY_TEST(RibConversions, RibConversionHgridDu) {
  runConversionTest<utility::HgridDuRouteScaleGenerator>();
}

OPTIMIZED_ONLY_TEST(RibConversions, RibConversionHgridUu) {
  runConversionTest<utility::HgridUuRouteScaleGenerator>();
}
