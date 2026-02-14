// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

namespace facebook::fboss::thrift_cow::test {

void fsdb_stats_storage(
    folly::UserCounters& counters,
    unsigned iters,
    test_data::RoleSelector selector,
    bool enableHybridStorage) {
  auto factory = test_data::FsdbStatsDataFactory(selector);
  bm_storage_metrics_helper<test_data::FsdbStatsDataFactory::RootT>(
      factory, counters, iters, selector, enableHybridStorage);
}

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    RDSW_ThriftCow,
    test_data::RoleSelector::RDSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    RDSW_HybridCow,
    test_data::RoleSelector::RDSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    FDSW_ThriftCow,
    test_data::RoleSelector::FDSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    FDSW_HybridCow,
    test_data::RoleSelector::FDSW,
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
