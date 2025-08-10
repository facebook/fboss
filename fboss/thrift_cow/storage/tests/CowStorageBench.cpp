// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <sys/resource.h>

#include "fboss/thrift_cow/storage/CowStorage.h"
#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"

namespace facebook::fboss::thrift_cow::test {

using namespace facebook::fboss::fsdb;

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
void bm_storage_helper(
    test_data::IDataGenerator& factory,
    bool enableHybridStorage) {
  folly::BenchmarkSuspender suspender;

  TaggedOperState state = factory.getStateUpdate(0, false);

  suspender.dismiss();

  if (enableHybridStorage) {
    StorageBenchmarkHelper<true> helper;
    auto storage = helper.initStorage<RootType>(state);
  } else {
    StorageBenchmarkHelper<false> helper;
    auto storage = helper.initStorage<RootType>(state);
  }

  suspender.rehire();
}

void bm_storage_test(
    uint32_t /* unused */,
    test_data::RoleSelector selector,
    bool enableHybridStorage) {
  auto factory = test_data::TestDataFactory(selector);
  bm_storage_helper<test_data::TestDataFactory::RootT>(
      factory, enableHybridStorage);
}

BENCHMARK_NAMED_PARAM(
    bm_storage_test,
    CowStorageScale,
    test_data::RoleSelector::MaxScale,
    false);

BENCHMARK_NAMED_PARAM(
    bm_storage_test,
    HybridStorageScale,
    test_data::RoleSelector::MaxScale,
    true);

} // namespace facebook::fboss::thrift_cow::test

inline int64_t timevalToUsec(const timeval& tv) {
  const int64_t kUsecPerSecond = 1000000;

  return (int64_t(tv.tv_sec) * kUsecPerSecond) + tv.tv_usec;
}

int main(int argc, char* argv[]) {
  struct rusage startUsage {};
  struct rusage endUsage {};

  getrusage(RUSAGE_SELF, &startUsage);

  folly::Init init(&argc, &argv);
  folly::runBenchmarks();

  getrusage(RUSAGE_SELF, &endUsage);
  auto cpuTime =
      (timevalToUsec(endUsage.ru_stime) - timevalToUsec(startUsage.ru_stime)) +
      (timevalToUsec(endUsage.ru_utime) - timevalToUsec(startUsage.ru_utime));

  folly::dynamic rusageJson = folly::dynamic::object;
  rusageJson["cpu_time_usec"] = cpuTime;
  rusageJson["max_rss"] = endUsage.ru_maxrss;
  std::cout << toPrettyJson(rusageJson) << std::endl;

  return 0;
}
