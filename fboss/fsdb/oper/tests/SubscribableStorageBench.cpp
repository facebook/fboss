// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/coro/AsyncScope.h>
#include <folly/coro/BlockingWait.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath

namespace {
constexpr auto kReadsPerTask = 1000;
constexpr auto kWritesPerTask = 200;
} // namespace

namespace facebook::fboss::fsdb::test {

class StorageBenchmarkHelper {
 public:
  using RootType = TestStruct;
  StorageBenchmarkHelper()
      : storage_(NaivePeriodicSubscribableCowStorage<RootType>({})) {
    storage_.setConvertToIDPaths(true);
    // initialize test data versions
    for (int version = 0; version < 2; version++) {
      TestStruct data;
      data.tx() = (version % 2) ? true : false;
      for (int i = 1; i <= 1 * 1000; i++) {
        TestStructSimple newStruct;
        newStruct.min() = i;
        newStruct.max() = version + i;
        data.structMap()[i] = newStruct;
      }
      testData_.emplace_back(std::move(data));
    }
    storage_.set(root, testData_[0]);
  }

  void startStorage() {
    storage_.start();
  }

  folly::coro::Task<void> getRequest(uint32_t numReads) {
    for (auto count = 0; count < numReads; count++) {
      storage_.get(this->root.structMap());
    }
    co_return;
  }

  folly::coro::Task<void> publishData(uint32_t numWrites, bool useLargeData) {
    for (auto count = 0; count < numWrites; count++) {
      int version = (count % 2);
      if (useLargeData) {
        storage_.set(root, testData_[version]);
      } else {
        storage_.set(root.structMap()[42], testData_[version].structMap()[42]);
      }
    }
    co_return;
  }

 private:
  thriftpath::RootThriftPath<RootType> root;
  NaivePeriodicSubscribableCowStorage<RootType> storage_;
  std::vector<RootType> testData_;
};

void bm_get(
    uint32_t /* unused */,
    uint32_t numThreads,
    uint32_t numReadsPerTask) {
  folly::BenchmarkSuspender suspender;

  StorageBenchmarkHelper helper;
  helper.startStorage();

  // launch get requests from multiple threads
  folly::coro::AsyncScope asyncScope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(numThreads);

  suspender.dismiss();

  for (int i = 0; i < numThreads; i++) {
    asyncScope.add(
        helper.getRequest(numReadsPerTask).scheduleOn(executor.get()));
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

  StorageBenchmarkHelper helper;
  helper.startStorage();

  folly::coro::AsyncScope asyncScope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(numThreads);

  suspender.dismiss();

  for (int i = 0; i < numThreads; i++) {
    asyncScope.add(helper.publishData(numWritesPerTask, useLargeData)
                       .scheduleOn(executor.get()));
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

} // namespace facebook::fboss::fsdb::test

int main(int argc, char* argv[]) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
