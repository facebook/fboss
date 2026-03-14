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
    EDSW_ThriftCow,
    test_data::RoleSelector::EDSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    EDSW_HybridCow,
    test_data::RoleSelector::EDSW,
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

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    SDSW_ThriftCow,
    test_data::RoleSelector::SDSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    SDSW_HybridCow,
    test_data::RoleSelector::SDSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    FA_ThriftCow,
    test_data::RoleSelector::FA,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    FA_HybridCow,
    test_data::RoleSelector::FA,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    RSW_ThriftCow,
    test_data::RoleSelector::RSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    RSW_HybridCow,
    test_data::RoleSelector::RSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    SSW_ThriftCow,
    test_data::RoleSelector::SSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    SSW_HybridCow,
    test_data::RoleSelector::SSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    RTSW_ThriftCow,
    test_data::RoleSelector::RTSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    RTSW_HybridCow,
    test_data::RoleSelector::RTSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    FTSW_ThriftCow,
    test_data::RoleSelector::FTSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    FTSW_HybridCow,
    test_data::RoleSelector::FTSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    STSW_ThriftCow,
    test_data::RoleSelector::STSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    STSW_HybridCow,
    test_data::RoleSelector::STSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    XSW_ThriftCow,
    test_data::RoleSelector::XSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    XSW_HybridCow,
    test_data::RoleSelector::XSW,
    true);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    MA_ThriftCow,
    test_data::RoleSelector::MA,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    fsdb_stats_storage,
    counters,
    MA_HybridCow,
    test_data::RoleSelector::MA,
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
