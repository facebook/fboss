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

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"

namespace facebook::fboss {

BENCHMARK(PhyInitAllColdBoot) {
  folly::BenchmarkSuspender suspender;
  auto wedgeMgr = setupForColdboot();
  suspender.dismiss();

  wedgeMgr->initExternalPhyMap();
}

BENCHMARK(PhyInitAllWarmBoot) {
  folly::BenchmarkSuspender suspender;
  auto wedgeMgr = setupForWarmboot();
  suspender.dismiss();

  wedgeMgr->initExternalPhyMap();
}

} // namespace facebook::fboss
