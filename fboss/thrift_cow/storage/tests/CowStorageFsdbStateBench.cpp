// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

namespace facebook::fboss::thrift_cow::test {

void fsdb_state_storage(
    folly::UserCounters& counters,
    unsigned iters,
    test_data::RoleSelector selector,
    bool enableHybridStorage) {
  auto factory = test_data::FsdbStateDataFactory(selector);
  std::vector<int64_t> memoryMeasurements;

  for (unsigned i = 0; i < iters; i++) {
    // Use memory-aware helper that reports via UserCounters
    auto memoryUsage =
        bm_storage_helper<test_data::FsdbStateDataFactory::RootT>(
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

// FSDB State-based benchmarks using different production scales
BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    RSW_ThriftCow,
    test_data::RoleSelector::RSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    RSW_HybridCow,
    test_data::RoleSelector::RSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    FSW_ThriftCow,
    test_data::RoleSelector::FSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    FSW_HybridCow,
    test_data::RoleSelector::FSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    SSW_ThriftCow,
    test_data::RoleSelector::SSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    SSW_HybridCow,
    test_data::RoleSelector::SSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    XSW_ThriftCow,
    test_data::RoleSelector::XSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    XSW_HybridCow,
    test_data::RoleSelector::XSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    MA_ThriftCow,
    test_data::RoleSelector::MA,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    MA_HybridCow,
    test_data::RoleSelector::MA,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    FA_ThriftCow,
    test_data::RoleSelector::FA,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    FA_HybridCow,
    test_data::RoleSelector::FA,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    RTSW_ThriftCow,
    test_data::RoleSelector::RTSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    RTSW_HybridCow,
    test_data::RoleSelector::RTSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    FTSW_ThriftCow,
    test_data::RoleSelector::FTSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    FTSW_HybridCow,
    test_data::RoleSelector::FTSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    STSW_ThriftCow,
    test_data::RoleSelector::STSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    STSW_HybridCow,
    test_data::RoleSelector::STSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    RDSW_ThriftCow,
    test_data::RoleSelector::RDSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    RDSW_HybridCow,
    test_data::RoleSelector::RDSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    EDSW_ThriftCow,
    test_data::RoleSelector::EDSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_state_storage,
    counters,
    EDSW_HybridCow,
    test_data::RoleSelector::EDSW,
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
