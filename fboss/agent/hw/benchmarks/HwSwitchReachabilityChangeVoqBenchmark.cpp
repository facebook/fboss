// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/benchmarks/HwSwitchReachabilityChangeBenchmarkHelper.h"

namespace facebook::fboss {

BENCHMARK(HwSwitchReachabilityChangeVoqBenchmark) {
  utility::switchReachabilityChangeBenchmarkHelper(cfg::SwitchType::VOQ);
}

} // namespace facebook::fboss
