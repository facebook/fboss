/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwRouteScaleBenchmarkHelpers.h"

#include "fboss/agent/test/RouteScaleGenerators.h"

namespace facebook::fboss {

ROUTE_ADD_BENCHMARK(
    HwHgridDUScaleRouteAddBenchmark,
    utility::HgridDuRouteScaleGenerator);
} // namespace facebook::fboss
