/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/Benchmark.h>

namespace facebook {
namespace fboss {

/*
 * Helper function to benchmark speed of route insertion, deletion
 * in HW. This function inits the ASIC, generate switch states for
 * a given route distribution and then measures the time it takes
 * to add (or delete post addition) these routes
 */
template <typename SwitchEnsembleT, typename RouteScaleGeneratorT>
void routeAddDelBenhmarker(bool measureAdd) {
  folly::BenchmarkSuspender suspender;
  static SwitchEnsembleT ensemble;
  auto config = utility::onePortPerVlanConfig(
      ensemble.getHwSwitch(), ensemble.getPlatform()->masterLogicalPortIds());
  ensemble.applyInitialConfigAndBringUpPorts(config);
  static const auto states =
      RouteScaleGeneratorT(ensemble.getProgrammedState()).get();
  if (measureAdd) {
    // Activate benchmarker before applying switch states
    // for adding routes to h/w
    suspender.dismiss();
  }
  for (auto& state : states) {
    ensemble.applyNewState(state);
  }
  // We are about to blow away all routes, before that
  // - Deactivate benchmark measurement if we are measuring
  // route addition
  // - Activate benchmark if we are measuring route deletion
  measureAdd ? suspender.rehire() : suspender.dismiss();
  ensemble.revertToInitCfgState();
}

#define ROUTE_ADD_BENCHMARK(name, EnsembleT, RouteScaleGeneratorT) \
  BENCHMARK(name) {                                                \
    routeAddDelBenhmarker<EnsembleT, RouteScaleGeneratorT>(true);  \
  }

#define ROUTE_DEL_BENCHMARK(name, EnsembleT, RouteScaleGeneratorT) \
  BENCHMARK(name) {                                                \
    routeAddDelBenhmarker<EnsembleT, RouteScaleGeneratorT>(false); \
  }
} // namespace fboss
} // namespace facebook
