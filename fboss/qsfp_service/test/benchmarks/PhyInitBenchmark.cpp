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

#include "fboss/qsfp_service/PortManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"

namespace facebook::fboss {

BENCHMARK(PhyInitAllColdBoot) {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<WedgeManager> wedgeMgr;
  std::unique_ptr<PortManager> portMgr;
  std::tie(wedgeMgr, portMgr) = setupForColdboot();
  suspender.dismiss();

  if (FLAGS_port_manager_mode) {
    portMgr->initExternalPhyMap();
  } else {
    wedgeMgr->initExternalPhyMap();
  }
}

BENCHMARK(PhyInitAllWarmBoot) {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<WedgeManager> wedgeMgr;
  std::unique_ptr<PortManager> portMgr;
  std::tie(wedgeMgr, portMgr) = setupForWarmboot();
  suspender.dismiss();

  if (FLAGS_port_manager_mode) {
    portMgr->initExternalPhyMap();
  } else {
    wedgeMgr->initExternalPhyMap();
  }
}

} // namespace facebook::fboss
