// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

namespace facebook::fboss::thrift_cow::test {

void fsdb_state_storage(
    folly::UserCounters& counters,
    unsigned iters,
    test_data::RoleSelector selector,
    bool enableHybridStorage) {
  auto factory = test_data::FsdbStateDataFactory(selector);
  bm_storage_metrics_helper<test_data::FsdbStateDataFactory::RootT>(
      factory, counters, iters, selector, enableHybridStorage);
}

void ribmap_state_storage(
    folly::UserCounters& counters,
    unsigned iters,
    test_data::RoleSelector selector,
    bool enableHybridStorage) {
  auto factory = test_data::BgpRibMapDataGenerator(selector);
  bm_storage_metrics_helper<test_data::BgpRibMapDataGenerator::RootT>(
      factory, counters, iters, selector, enableHybridStorage);
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

BENCHMARK_COUNTERS_NAME_PARAM(
    ribmap_state_storage,
    counters,
    RibMap_RSW_ThriftCow,
    test_data::RoleSelector::RSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    ribmap_state_storage,
    counters,
    RibMap_FSW_ThriftCow,
    test_data::RoleSelector::FSW,
    false);

BENCHMARK_COUNTERS_NAME_PARAM(
    ribmap_state_storage,
    counters,
    RibMap_FA_ThriftCow,
    test_data::RoleSelector::FA,
    false);

} // namespace facebook::fboss::thrift_cow::test

int main(int argc, char* argv[]) {
  struct rusage startUsage{};
  getrusage(RUSAGE_SELF, &startUsage);

  folly::Init init(&argc, &argv);
  folly::runBenchmarks();

  facebook::fboss::thrift_cow::test::printBenchmarkResults(startUsage);
  return 0;
}
