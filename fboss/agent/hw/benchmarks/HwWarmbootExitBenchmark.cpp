/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <folly/Benchmark.h>
#include <folly/IPAddressV6.h>
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>

#include <chrono>
#include <iostream>

namespace facebook::fboss {

void runBenchmark() {
  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](SwSwitch* swSwitch, const std::vector<PortID>& ports) {
        // Before m-mpu agent test, use first Asic for initialization.
        auto switchIds = swSwitch->getHwAsicTable()->getSwitchIDs();
        CHECK_GE(switchIds.size(), 1);
        auto asic = swSwitch->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
        return utility::onePortPerInterfaceConfig(
            swSwitch->getPlatformMapping(),
            asic,
            ports,
            asic->desiredLoopbackModes());
        ;
      };
  auto ensemble = createAgentEnsemble(initialConfigFn);

  utility::RouteDistributionGenerator::ThriftRouteChunks routeChunks;
  // TODO(zecheng): Deprecate ensemble access to platform
  auto platform =
      static_cast<MonolithicAgentInitializer*>(ensemble->agentInitializer())
          ->platform();
  if (platform->getType() == PlatformType::PLATFORM_WEDGE) {
    routeChunks = utility::RouteDistributionGenerator(
                      ensemble->getProgrammedState(),
                      // Distribution taken from a random wedge in fleet
                      {
                          // v6 distribution - mask to num unique prefixes
                          {37, 7},
                          {44, 11},
                          {47, 6},
                          {48, 3},
                          {52, 61},
                          {54, 340},
                          {64, 1542},
                          {80, 1},
                          {127, 4},
                      },
                      {// v4 distribution - mask to num unique prefixes
                       {15, 4},
                       {16, 1},
                       {17, 1},
                       {21, 10},
                       {24, 31},
                       {26, 6},
                       {27, 26},
                       {31, 40}},
                      4000,
                      4)
                      .getThriftRoutes();
  } else {
    routeChunks =
        utility::FSWRouteScaleGenerator(ensemble->getProgrammedState())
            .getThriftRoutes();
  }
  ensemble->programRoutes(RouterID(0), ClientID::BGPD, routeChunks);
  // Static such that the object destructor runs as late as possible. In
  // particular in this case, destructor (and thus the duration calculation)
  // will run at the time of program exit when static variable destructors run
  static StopWatch timer("warm_boot_msecs", FLAGS_json);
  // @lint-ignore CLANGTIDY
  FLAGS_setup_for_warmboot = true;
  ensemble.reset();
}
} // namespace facebook::fboss
