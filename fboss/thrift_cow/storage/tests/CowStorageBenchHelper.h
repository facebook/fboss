// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <gtest/gtest.h>
#include <sys/resource.h>

#include <common/base/Proc.h>
#include "fboss/thrift_cow/storage/CowStorage.h"
#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"

namespace facebook::fboss::thrift_cow::test {

using facebook::fboss::fsdb::CowStorage;
using facebook::fboss::fsdb::TaggedOperState;

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

template <typename RootType>
int64_t bm_storage_helper(
    test_data::IDataGenerator& factory,
    bool enableHybridStorage) {
  folly::BenchmarkSuspender suspender;
  TaggedOperState state = factory.getStateUpdate(0, false);
  sleep(2); // Sleep to allow memory to be freed

  auto startingMemoryBytes = facebook::Proc::getMemoryUsage();
  int64_t endingMemoryBytes = 0;
  suspender.dismiss();

  if (enableHybridStorage) {
    StorageBenchmarkHelper<true> helper;
    auto storage = helper.initStorage<RootType>(state);
  } else {
    StorageBenchmarkHelper<false> helper;
    auto storage = helper.initStorage<RootType>(state);
  }

  suspender.rehire();
  endingMemoryBytes = facebook::Proc::getMemoryUsage();

  return (endingMemoryBytes - startingMemoryBytes);
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
