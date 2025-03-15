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

namespace facebook::fboss {

BENCHMARK(HwVoqScaleRouteAddBenchmark) {
  // Measure 8x512 ECMP route add
  voqRouteBenchmark(true /* add */, 8 /* ecmpGroup */, 512 /* ecmpWidth */);
}

BENCHMARK(HwVoqScale2kWideEcmpRouteAddBenchmark) {
  // TODO(zecheng): Update benchmark to program 16x2048 ECMP route once
  // programming speed issue is resolved.

  // Measure 2x2048 ECMP route add
  voqRouteBenchmark(true /* add */, 2 /* ecmpGroup */, 2048 /* ecmpWidth */);
}
} // namespace facebook::fboss
