/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"

#include <folly/Benchmark.h>
#include "fboss/lib/FunctionCallTimeReporter.h"

namespace facebook::fboss {

/*
 * Helper function to benchmark speed of route insertion, deletion
 * in HW. This function inits the ASIC, generate switch states for
 * a given route distribution and then measures the time it takes
 * to add (or delete post addition) these routes
 */
template <typename RouteScaleGeneratorT>
void routeAddDelBenchmarker(bool measureAdd) {
  folly::BenchmarkSuspender suspender;
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  auto config = utility::onePortPerVlanConfig(
      ensemble->getHwSwitch(), ensemble->masterLogicalPortIds());
  ensemble->applyInitialConfig(config);
  auto routeGenerator = RouteScaleGeneratorT(ensemble->getProgrammedState());
  if (!routeGenerator.isSupported(ensemble->getPlatform()->getMode())) {
    // skip if this is not supported for a platform
    return;
  }
  static const auto states = routeGenerator.getSwitchStates();

  if (measureAdd) {
    ScopedCallTimer timeIt;
    // Activate benchmarker before applying switch states
    // for adding routes to h/w
    suspender.dismiss();
    for (auto& state : states) {
      ensemble->applyNewState(state);
    }
    // We are about to blow away all routes, before that
    // deactivate benchmark measurement.
    suspender.rehire();
  } else {
    for (auto& state : states) {
      ensemble->applyNewState(state);
    }
    ScopedCallTimer timeIt;
    // We are about to blow away all routes, before that
    // activate benchmark measurement.
    suspender.dismiss();
    ensemble.reset();
  }
}

#define ROUTE_ADD_BENCHMARK(name, RouteScaleGeneratorT) \
  BENCHMARK(name) {                                     \
    routeAddDelBenchmarker<RouteScaleGeneratorT>(true); \
  }

#define ROUTE_DEL_BENCHMARK(name, RouteScaleGeneratorT)  \
  BENCHMARK(name) {                                      \
    routeAddDelBenchmarker<RouteScaleGeneratorT>(false); \
  }

} // namespace facebook::fboss
