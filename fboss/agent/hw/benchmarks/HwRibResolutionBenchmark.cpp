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

BENCHMARK(RibResolutionBenchmark) {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<AgentEnsemble> ensemble{};

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](HwSwitch* hwSwitch, const std::vector<PortID>& ports) {
        CHECK_GT(ports.size(), 0);
        return utility::onePortPerInterfaceConfig(
            hwSwitch,
            ports,
            hwSwitch->getPlatform()->getAsic()->desiredLoopbackModes());
      };
  ensemble = createAgentEnsemble(initialConfigFn);
  auto ports = ensemble->masterLogicalPortIds();

  utility::THAlpmRouteScaleGenerator gen(ensemble->getSw()->getState());
  const auto& routeChunks = gen.getThriftRoutes();
  // Create a dummy rib since we don't want to go through
  // HwSwitchEnsemble and write to HW
  auto rib = RoutingInformationBase::fromThrift(
      ensemble->getSw()->getRib()->toThrift(), nullptr, nullptr);
  auto switchState = ensemble->getProgrammedState();
  suspender.dismiss();
  std::for_each(
      routeChunks.begin(),
      routeChunks.end(),
      [&switchState, &rib, resolver = ensemble->getSw()->getScopeResolver()](
          const auto& routeChunk) {
        rib->update(
            resolver,
            RouterID(0),
            ClientID::BGPD,
            AdminDistance::EBGP,
            routeChunk,
            {},
            false,
            "resolution only",
            ribToSwitchStateUpdate,
            static_cast<void*>(&switchState));
      });
  suspender.rehire();
}

} // namespace facebook::fboss
