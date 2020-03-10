/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Benchmark.h>

#include <fboss/agent/platforms/wedge/facebook/minipack/MinipackPlatform.h>
#include <fboss/lib/fpga/MinipackFpga.h>

#include "PhyInitBenchmark-defs.h"

namespace facebook::fboss {

BENCHMARK(MinipackPhyInitAllForce) {
  PhyInitAllForce<MinipackPlatform, MinipackFpga>();
}

BENCHMARK(MinipackPhyInitAllAuto) {
  PhyInitAllAuto<MinipackPlatform, MinipackFpga>();
}

} // namespace facebook::fboss
