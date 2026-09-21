/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwVoqRouteCompetingRemoteNeighborBenchmarkHelper.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(HwVoqRouteCompetingRemoteNeighborBenchmark) {
  voqRouteCompetingRemoteNeighborBenchmark(
      16 /*ecmpGroup*/,
      2048 /*ecmpWidth*/,
      1000 /*neighborUpdateIntervalMs*/,
      10 /*numRemoteNode*/,
      1 /*numNbrToUpdate*/);
}
} // namespace facebook::fboss
