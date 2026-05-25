// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

namespace facebook::fboss::thrift_cow::test {

void bm_storage_test(
    folly::UserCounters& counters,
    unsigned iters,
    test_data::RoleSelector selector,
    bool enableHybridStorage) {
  auto factory = test_data::TestDataFactory(selector);
  std::vector<int64_t> allocatedMeasurements;

  for (unsigned i = 0; i < iters; i++) {
    auto allocatedDelta = bm_storage_helper<test_data::TestDataFactory::RootT>(
        factory, enableHybridStorage);
    if (allocatedDelta > 0) {
      allocatedMeasurements.push_back(allocatedDelta);
    }
  }

  // Calculate and report metrics via UserCounters
  if (!allocatedMeasurements.empty()) {
    int64_t sum = 0;
    int64_t maxAlloc = *std::max_element(
        allocatedMeasurements.begin(), allocatedMeasurements.end());

    for (int64_t bytes : allocatedMeasurements) {
      sum += bytes;
    }

    int64_t avgAlloc = sum / static_cast<int64_t>(allocatedMeasurements.size());

    // Report metrics - jemalloc `stats.allocated` deltas, in KB.
    counters["avg_allocated_KB"] =
        folly::UserMetric(static_cast<double>(avgAlloc) / 1024.0);
    counters["max_allocated_KB"] =
        folly::UserMetric(static_cast<double>(maxAlloc) / 1024.0);
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
