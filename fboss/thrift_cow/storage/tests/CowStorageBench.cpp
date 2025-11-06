// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

namespace facebook::fboss::thrift_cow::test {

void bm_storage_test(
    folly::UserCounters& counters,
    unsigned iters,
    test_data::RoleSelector selector,
    bool enableHybridStorage) {
  auto factory = test_data::TestDataFactory(selector);
  std::vector<int64_t> memoryMeasurements;

  for (unsigned i = 0; i < iters; i++) {
    // Use memory-aware helper that reports via UserCounters
    auto memoryUsage = bm_storage_helper<test_data::TestDataFactory::RootT>(
        factory, enableHybridStorage);
    if (memoryUsage > 0) {
      memoryMeasurements.push_back(memoryUsage);
    }
  }

  // Calculate and report metrics via UserCounters
  if (!memoryMeasurements.empty()) {
    int64_t sum = 0;
    int64_t maxMem =
        *std::max_element(memoryMeasurements.begin(), memoryMeasurements.end());

    for (int64_t mem : memoryMeasurements) {
      sum += mem;
    }

    int64_t avgMem = sum / static_cast<int64_t>(memoryMeasurements.size());

    // Report metrics - these will appear as columns in benchmark output
    counters["avg_memory_KB"] =
        folly::UserMetric(static_cast<double>(avgMem) / 1024.0);
    counters["max_memory_KB"] =
        folly::UserMetric(static_cast<double>(maxMem) / 1024.0);
  }
}

// Original benchmarks using TestDataFactory
BENCHMARK_COUNTERS_NAME_PARAM(
    bm_storage_test,
    counters,
    CowStorageScale,
    test_data::RoleSelector::MaxScale,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    bm_storage_test,
    counters,
    HybridStorageScale,
    test_data::RoleSelector::MaxScale,
    true);

} // namespace facebook::fboss::thrift_cow::test

int main(int argc, char* argv[]) {
  struct rusage startUsage{};
  getrusage(RUSAGE_SELF, &startUsage);

  folly::Init init(&argc, &argv);
  folly::runBenchmarks();

  facebook::fboss::thrift_cow::test::printBenchmarkResults(startUsage);
  return 0;
}
