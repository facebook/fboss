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
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"

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
  std::unique_ptr<RoutingInformationBase> newRibFromLegacySwitchState;
  uint64_t legacyRibV4, legacyRibV6;
  {
    // First SwSwitch with legacy rib. Scoped block to
    // destroy this SwSwitch on block exit
    auto testHandle = createTestHandle(&config);
    auto sw = testHandle->getSw();
    auto generator = Generator(
        sw->getState(), sw->isStandaloneRibEnabled(), 1337, kEcmpWidth);
    const auto& routeChunks = generator.get();
    programRoutes(routeChunks, sw);
    auto swStateTables = RouteTableMap::fromFollyDynamic(
        sw->getState()->getRouteTables()->toFollyDynamic());
    newRibFromLegacySwitchState = switchStateToStandaloneRib(swStateTables);
    // From new rib back to legacy results in same route tables
    auto swStateLegacyRib =
        standaloneToSwitchStateRib(*newRibFromLegacySwitchState);
    EXPECT_EQ(
        swStateLegacyRib->toFollyDynamic(), swStateTables->toFollyDynamic());
    swStateTables->getRouteCount(&legacyRibV4, &legacyRibV6);
  }
  // create new switch with Standalone rib enabled
  auto testHandleWithRib =
      createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
  auto swWithRib = testHandleWithRib->getSw();
  // Program reconstructed RIB to FIB
  programRib(*newRibFromLegacySwitchState, swWithRib);

  auto [newRibV4, newRibV6] = swWithRib->getState()->getFibs()->getRouteCount();
  {
    // Test conversion to switch state api

    auto fib = fibFromStandaloneRib(*newRibFromLegacySwitchState);
    EXPECT_EQ(
        fib->toFollyDynamic(),
        swWithRib->getState()->getFibs()->toFollyDynamic());
  }

  EXPECT_EQ(legacyRibV4, newRibV4);
  EXPECT_EQ(legacyRibV6, newRibV6);
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
