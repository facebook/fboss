#include <folly/Benchmark.h>
#include "fboss/agent/hw/benchmarks/HwBGPRxSlowPathBenchmarkHelpers.h"

namespace facebook::fboss {

BENCHMARK(BgpRxSlowPathRouteChangeBenchmark) {
  rxSlowPathBGPRouteChangeBenchmark(utility::BgpRxMode::routeProgramming);
}

BENCHMARK(BgpRxSlowPathRouteChangePortFlapBenchmark) {
  rxSlowPathBGPRouteChangeBenchmark(
      utility::BgpRxMode::routeProgrammingWithPortFlap);
}
} // namespace facebook::fboss
