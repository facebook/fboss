/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "common/init/Init.h"
#include "fboss/agent/StandaloneRibConversions.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/sim/SimPlatform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/Benchmark.h>
#include <folly/MacAddress.h>
#include <folly/dynamic.h>

using namespace facebook::fboss;

template <typename Generator>
static void runConversionBenchmark() {
  auto constexpr kEcmpWidth = 4;

  SimPlatform plat(folly::MacAddress(), 128);
  std::vector<PortID> ports;
  for (int i = 0; i < 128; ++i) {
    ports.push_back(PortID(i));
  }
  std::unique_ptr<rib::RoutingInformationBase> newRibFromLegacySwitchState;
  cfg::SwitchConfig config =
      utility::onePortPerVlanConfig(plat.getHwSwitch(), ports);
  {
    // First SwSwitch with legacy rib. Scoped block to
    // destroy this SwSwitch on block exit
    auto testHandle = createTestHandle(&config);
    auto sw = testHandle->getSw();

    auto generator = Generator(
        sw->getState(), sw->isStandaloneRibEnabled(), 1337, kEcmpWidth);
    const auto& routeChunks = generator.get();
    programRoutes(routeChunks, sw);

    auto swStateTables = sw->getState()->getRouteTables();
    newRibFromLegacySwitchState = switchStateToStandaloneRib(swStateTables);
    auto swStateRib = standaloneToSwitchStateRib(*newRibFromLegacySwitchState);
  }
  // create new switch with Standalone rib enabled
  auto testHandleWithRib =
      createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
  auto swWithRib = testHandleWithRib->getSw();
  // Program reconstructed RIB to FIB
  programRib(*newRibFromLegacySwitchState, swWithRib);
}

BENCHMARK(RibConversionFSW) {
  runConversionBenchmark<utility::FSWRouteScaleGenerator>();
}

BENCHMARK(RibConversionTHAlpm) {
  runConversionBenchmark<utility::THAlpmRouteScaleGenerator>();
}

BENCHMARK(RibConversionHgridDu) {
  runConversionBenchmark<utility::HgridDuRouteScaleGenerator>();
}

BENCHMARK(RibConversionHgridUu) {
  runConversionBenchmark<utility::HgridUuRouteScaleGenerator>();
}

int main(int argc, char** argv) {
  facebook::initFacebook(&argc, &argv);
  folly::runBenchmarks();
  return EXIT_SUCCESS;
}
