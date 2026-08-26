// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <sys/resource.h>

#include "fboss/thrift_cow/storage/CowStorage.h"
#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"

DECLARE_int32(bm_memory_iters);
DECLARE_string(bm_fsdb_path);

namespace facebook::fboss::thrift_cow::test {

using facebook::fboss::fsdb::CowStorage;
using facebook::fboss::fsdb::TaggedOperState;

std::vector<std::string> parseFsdbPath(const std::string& path);

template <bool EnableHybridStorage>
class StorageBenchmarkHelper {
 public:
  template <typename RootType>
  auto initStorage(TaggedOperState& state) {
    auto val = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::structure, RootType>(
            *state.state()->protocol(), std::move(*state.state()->contents()));
    if constexpr (EnableHybridStorage) {
      return CowStorage<
          RootType,
          facebook::fboss::thrift_cow::ThriftStructNode<
              RootType,
              facebook::fboss::thrift_cow::ThriftStructResolver<RootType, true>,
              true>>(val);
    } else {
      return CowStorage<
          RootType,
          facebook::fboss::thrift_cow::ThriftStructNode<
              RootType,
              facebook::fboss::thrift_cow::
                  ThriftStructResolver<RootType, false>,
              false>>(val);
    }
  }
};

// Reads jemalloc's `stats.allocated` (bytes currently allocated to the
// application). Requires jemalloc; returns 0 if unavailable so callers can
// drop the measurement. Defined in CowStorageBenchHelper.cpp.
int64_t getJemallocAllocatedBytes();

struct MemoryStats {
  double avgKB{0.0};
  double maxKB{0.0};
  double stddevKB{0.0};
};

std::optional<MemoryStats> computeMemoryStats(
    const std::vector<int64_t>& measurements);

void reportMemoryCounters(
    folly::UserCounters& counters,
    std::string_view prefix,
    const MemoryStats& stats);

template <typename RootType>
int64_t bm_storage_helper(
    test_data::IDataGenerator& factory,
    bool enableHybridStorage) {
  folly::BenchmarkSuspender suspender;
  TaggedOperState state = factory.getStateUpdate(0, false);

  auto startingAllocatedBytes = getJemallocAllocatedBytes();
  int64_t endingAllocatedBytes = 0;
  suspender.dismiss();

  // Measure allocator stats inside each branch to ensure `storage` stays alive
  // during measurement. Moving these lines outside the braces would measure
  // after storage is destroyed, giving incorrect results.
  if (enableHybridStorage) {
    StorageBenchmarkHelper<true> helper;
    auto storage = helper.initStorage<RootType>(state);
    suspender.rehire();
    endingAllocatedBytes = getJemallocAllocatedBytes();
  } else {
    StorageBenchmarkHelper<false> helper;
    auto storage = helper.initStorage<RootType>(state);
    suspender.rehire();
    endingAllocatedBytes = getJemallocAllocatedBytes();
  }

  return (endingAllocatedBytes - startingAllocatedBytes);
}

template <typename RootType>
void bm_storage_metrics_helper(
    test_data::IDataGenerator& factory,
    folly::UserCounters& counters,
    unsigned /* iters */,
    test_data::RoleSelector /* selector */,
    bool enableHybridStorage) {
  std::vector<int64_t> allocatedMeasurements;

  // Set path filter on factory if --bm_fsdb_path is specified
  auto pathTokens = parseFsdbPath(FLAGS_bm_fsdb_path);
  if (!pathTokens.empty()) {
    factory.setPathFilter(pathTokens);
  }

  // Use FLAGS_bm_memory_iters instead of folly's iters for memory measurement
  int iterations = FLAGS_bm_memory_iters;

  for (int i = 0; i < iterations; i++) {
    auto allocatedDelta =
        bm_storage_helper<RootType>(factory, enableHybridStorage);
    if (allocatedDelta > 0) {
      allocatedMeasurements.push_back(allocatedDelta);
    }
  }

  // Values are deltas of jemalloc `stats.allocated` across building one
  // storage instance (bytes currently allocated to the application).
  if (auto stats = computeMemoryStats(allocatedMeasurements)) {
    reportMemoryCounters(counters, "allocated", *stats);
  }
}

// Custom macro that combines BENCHMARK_NAMED_PARAM with UserCounters support
#define BENCHMARK_COUNTERS_NAME_PARAM(name, counters, param_name, ...) \
  BENCHMARK_IMPL_COUNTERS(                                             \
      FB_CONCATENATE(name, FB_CONCATENATE(_, param_name)),             \
      FOLLY_PP_STRINGIZE(name) "(" FOLLY_PP_STRINGIZE(param_name) ")", \
      counters,                                                        \
      iters,                                                           \
      unsigned,                                                        \
      iters) {                                                         \
    name(counters, iters, ##__VA_ARGS__);                              \
  }

int64_t timevalToUsec(const timeval& tv);
void printBenchmarkResults(const struct rusage& startUsage);
} // namespace facebook::fboss::thrift_cow::test
