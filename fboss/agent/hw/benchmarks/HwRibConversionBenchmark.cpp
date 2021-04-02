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
#include "fboss/agent/StandaloneRibConversions.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/test/RouteGeneratorTestUtils.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>
DECLARE_bool(enable_standalone_rib);

namespace facebook::fboss {

BENCHMARK(RibConversionBenchmark) {
  folly::BenchmarkSuspender suspender;
  FLAGS_enable_standalone_rib = true;
  SCOPE_EXIT {
    FLAGS_enable_standalone_rib = true;
  };
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  auto config = utility::onePortPerVlanConfig(
      ensemble->getHwSwitch(), ensemble->masterLogicalPortIds());
  ensemble->applyInitialConfig(config);
  utility::THAlpmRouteScaleGenerator gen(ensemble->getProgrammedState(), true);
  const auto& routeChunks = gen.getThriftRoutes();
  auto rib = ensemble->getRib();
  auto switchState = ensemble->getProgrammedState();
  std::for_each(
      routeChunks.begin(),
      routeChunks.end(),
      [&switchState, rib](const auto& routeChunk) {
        rib->update(
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
  // New RIB to old RIB
  auto newRibJson = ensemble->gracefulExitState();
  auto convert = [&suspender](
                     const folly::dynamic& json,
                     const std::shared_ptr<SwitchState> state,
                     const std::string& name) {
    HwInitResult result{state};
    suspender.dismiss();
    StopWatch watch(name, false);
    handleStandaloneRIBTransition(json, result);
    suspender.rehire();
    return result;
  };
  FLAGS_enable_standalone_rib = false;
  // New RIB -> Old
  auto newRibToOldRib = convert(newRibJson, switchState, "newRibToOld");
  auto oldRibJson = newRibToOldRib.switchState->toFollyDynamic();
  // Old RIB -> Old
  convert(oldRibJson, switchState, "oldRibToOld");
  // Old rib -> new rib
  FLAGS_enable_standalone_rib = true;
  auto oldRibToNewRib =
      convert(oldRibJson, newRibToOldRib.switchState, "oldRibToNew");
  // New rib -> new
  convert(newRibJson, oldRibToNewRib.switchState, "newRibToNew");
  // Assert that we recreated the original RIB, fib
  RouterID kRid0(0);
  CHECK(
      rib->getRouteTableDetails(kRid0) ==
      oldRibToNewRib.rib->getRouteTableDetails(kRid0));
  CHECK(
      switchState->getFibs()->toFollyDynamic() ==
      oldRibToNewRib.switchState->getFibs()->toFollyDynamic());
}

} // namespace facebook::fboss
