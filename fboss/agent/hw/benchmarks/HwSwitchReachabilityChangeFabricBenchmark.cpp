// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/benchmarks/HwSwitchReachabilityChangeBenchmarkHelper.h"

namespace facebook::fboss {

BENCHMARK(HwSwitchReachabilityChangeFabricBenchmark) {
  utility::switchReachabilityChangeBenchmarkHelper(cfg::SwitchType::FABRIC);
}

} // namespace facebook::fboss
