/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/test/RouteGeneratorTestUtils.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

BENCHMARK(RibSyncFibBenchmark) {
  folly::BenchmarkSuspender suspender;
  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](HwSwitch* hwSwitch, const std::vector<PortID>& ports) {
        auto config = utility::onePortPerInterfaceConfig(hwSwitch, ports);
        return config;
      };

  auto ensemble = createAgentEnsemble(initialConfigFn);
  auto state = ensemble->getSw()->getState();
  utility::THAlpmRouteScaleGenerator gen(state, 50000);
  const auto& routeChunks = gen.getThriftRoutes();
  CHECK_EQ(1, routeChunks.size());
  // Create a dummy rib since we don't want to go through
  // AgentSwitchEnsemble and write to HW
  auto rib = RoutingInformationBase::fromFollyDynamic(
      ensemble->getSw()->getRib()->toFollyDynamic(), nullptr, nullptr);
  auto switchState = ensemble->getSw()->getState();
  rib->update(
      RouterID(0),
      ClientID::BGPD,
      AdminDistance::EBGP,
      routeChunks[0],
      {},
      false,
      "resolution only",
      ribToSwitchStateUpdate,
      static_cast<void*>(&switchState));
  switchState = ensemble->getSw()->getState();
  suspender.dismiss();
  // Sync fib with the same routes
  rib->update(
      RouterID(0),
      ClientID::BGPD,
      AdminDistance::EBGP,
      routeChunks[0],
      {},
      true,
      "sync fib",
      ribToSwitchStateUpdate,
      static_cast<void*>(&switchState));
  suspender.rehire();
}
} // namespace facebook::fboss
