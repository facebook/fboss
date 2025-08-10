// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/coro/AsyncScope.h>
#include <folly/coro/BlockingWait.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <gtest/gtest.h>

#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/thrift_cow/storage/tests/TestDataFactory.h>
#include "fboss/fsdb/oper/tests/SubscribableStorageBenchHelper.h"

namespace {
constexpr auto kReadsPerTask = 1000;
constexpr auto kWritesPerTask = 200;
} // namespace

namespace facebook::fboss::fsdb::test {

void bm_get(
    uint32_t /* unused */,
    uint32_t numThreads,
    uint32_t numReadsPerTask) {
  folly::BenchmarkSuspender suspender;

  test_data::TestDataFactory dataGen(test_data::RoleSelector::MaxScale);
  StorageBenchmarkHelper helper(dataGen);
  helper.startStorage();

  // launch get requests from multiple threads
  folly::coro::AsyncScope asyncScope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(numThreads);

  suspender.dismiss();

  for (int i = 0; i < numThreads; i++) {
    asyncScope.add(
        co_withExecutor(executor.get(), helper.getRequest(numReadsPerTask)));
  }

  folly::coro::blockingWait(asyncScope.joinAsync());

  suspender.rehire();
}

void bm_set(
    uint32_t /* unused */,
    uint32_t numThreads,
    uint32_t numWritesPerTask,
    bool useLargeData) {
  folly::BenchmarkSuspender suspender;

  test_data::TestDataFactory dataGen(test_data::RoleSelector::MaxScale);
  StorageBenchmarkHelper helper(
      dataGen, StorageBenchmarkHelper::Params().setLargeUpdates(useLargeData));
  helper.startStorage();

  folly::coro::AsyncScope asyncScope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(numThreads);

  suspender.dismiss();

  for (int i = 0; i < numThreads; i++) {
    asyncScope.add(
        co_withExecutor(executor.get(), helper.publishData(numWritesPerTask)));
  }

  folly::coro::blockingWait(asyncScope.joinAsync());

  suspender.rehire();
}

void bm_concurrent_get_set(
    uint32_t /* unused */,
    uint32_t numThreads,
    uint32_t numReadsPerTask,
    uint32_t numWritesPerTask,
    bool useLargeData,
    bool serveGetRequestsWithLastPublishedState = true) {
  CHECK_GT(numThreads, 1);

  folly::BenchmarkSuspender suspender;

  test_data::TestDataFactory dataGen(test_data::RoleSelector::MaxScale);
  StorageBenchmarkHelper helper(
      dataGen,
      StorageBenchmarkHelper::Params()
          .setLargeUpdates(useLargeData)
          .setServeGetWithLastPublished(
              serveGetRequestsWithLastPublishedState));
  helper.startStorage();

  folly::coro::AsyncScope asyncScope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(numThreads);

  suspender.dismiss();

  asyncScope.add(
      co_withExecutor(executor.get(), helper.publishData(numWritesPerTask)));

  for (int i = 1; i < numThreads; i++) {
    asyncScope.add(
        co_withExecutor(executor.get(), helper.getRequest(numReadsPerTask)));
  }

  folly::coro::blockingWait(asyncScope.joinAsync());

  suspender.rehire();
}

BENCHMARK_NAMED_PARAM(bm_get, threads_1, 1, kReadsPerTask);

BENCHMARK_NAMED_PARAM(bm_get, threads_2, 2, kReadsPerTask);

BENCHMARK_NAMED_PARAM(bm_get, threads_4, 4, kReadsPerTask);

BENCHMARK_NAMED_PARAM(bm_get, threads_8, 8, kReadsPerTask);

BENCHMARK_NAMED_PARAM(bm_get, threads_12, 12, kReadsPerTask);

BENCHMARK_NAMED_PARAM(bm_get, threads_16, 16, kReadsPerTask);

BENCHMARK_NAMED_PARAM(bm_set, threads_1, 1, kWritesPerTask, true);

BENCHMARK_NAMED_PARAM(bm_set, threads_4, 4, kWritesPerTask, true);

BENCHMARK_NAMED_PARAM(bm_set, threads_8, 8, kWritesPerTask, true);

BENCHMARK_NAMED_PARAM(
    bm_concurrent_get_set,
    threads_2,
    2,
    kReadsPerTask,
    kWritesPerTask,
    true);

BENCHMARK_NAMED_PARAM(
    bm_concurrent_get_set,
    threads_4,
    4,
    kReadsPerTask,
    kWritesPerTask,
    true);

BENCHMARK_NAMED_PARAM(
    bm_concurrent_get_set,
    threads_8,
    8,
    kReadsPerTask,
    kWritesPerTask,
    true);

BENCHMARK_NAMED_PARAM(
    bm_concurrent_get_set,
    threads_12,
    12,
    kReadsPerTask,
    kWritesPerTask,
    true);

BENCHMARK_NAMED_PARAM(
    bm_concurrent_get_set,
    threads_16,
    16,
    kReadsPerTask,
    kWritesPerTask,
    true);

BENCHMARK_NAMED_PARAM(
    bm_concurrent_get_set,
    threads_16_serveGetWithCurrentState,
    16,
    kReadsPerTask,
    kWritesPerTask,
    true,
    false);

} // namespace facebook::fboss::fsdb::test

int main(int argc, char* argv[]) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
