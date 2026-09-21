// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/benchmarks/HwCpuLatencyBenchmarkHelper.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(cpuLatencyBenchmark) {
  runCpuLatencyBenchmark();
}

} // namespace facebook::fboss
