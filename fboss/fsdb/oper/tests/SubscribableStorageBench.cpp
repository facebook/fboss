// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/coro/AsyncScope.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/synchronization/Baton.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <vector>

#include <fboss/fsdb/if/FsdbModel.h>
#include <fboss/fsdb/oper/ExtendedPathBuilder.h>
#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/thrift_cow/storage/tests/TestDataFactory.h>
#include "fboss/fsdb/oper/tests/SubscribableStorageBenchHelper.h"
#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

DEFINE_int32(
    bm_subbench_memory_iters,
    3,
    "Memory measurement iterations for subscribable storage memory benchmarks.");

namespace {
constexpr auto kReadsPerTask = 1000;
constexpr auto kWritesPerTask = 200;
constexpr auto kNumIncrementalUpdates = 10;
} // namespace

namespace facebook::fboss::fsdb::test {

void bm_get(
    uint32_t /* unused */,
    uint32_t numThreads,
    uint32_t numReadsPerTask) {
  folly::BenchmarkSuspender suspender;

  test_data::TestDataFactory dataGen(test_data::RoleSelector::MaxScale);
  StorageBenchmarkHelper<> helper(dataGen);
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
  StorageBenchmarkHelper<> helper(
      dataGen,
      StorageBenchmarkHelper<>::Params()
          .setLargeUpdates(useLargeData)
          .setNumUpdates(2));
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
  StorageBenchmarkHelper<> helper(
      dataGen,
      StorageBenchmarkHelper<>::Params()
          .setLargeUpdates(useLargeData)
          .setNumUpdates(2)
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

void bm_serve_initialSync(
    uint32_t /* unused */,
    uint32_t numPatchSubs,
    uint32_t numPathSubs,
    uint32_t numDeltaSubs) {
  folly::BenchmarkSuspender suspender;

  test_data::TestDataFactory dataGen(test_data::RoleSelector::MaxScale);
  StorageBenchmarkHelper<> helper(
      dataGen,
      StorageBenchmarkHelper<>::Params().setStartWithInitializedData(false));
  helper.startStorage();

  folly::coro::AsyncScope asyncScope;
  auto numThreads = numPatchSubs + numPathSubs + numDeltaSubs;
  int nExpectedValues = 1;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(numThreads);

  std::map<int, SubscriptionIdentifier> subIds;
  int nSubs = 0;

  for (int i = 0; i < numPatchSubs; i++) {
    subIds.emplace(nSubs, SubscriberId(fmt::format("patch_sub_{}", i)));
    asyncScope.add(co_withExecutor(
        executor.get(),
        helper.addPatchSubscription(
            std::move(subIds.at(nSubs)), nExpectedValues)));
    nSubs++;
  }

  for (int i = 0; i < numPathSubs; i++) {
    subIds.emplace(nSubs, SubscriberId(fmt::format("path_sub_{}", i)));
    asyncScope.add(co_withExecutor(
        executor.get(),
        helper.addPathSubscription(
            std::move(subIds.at(nSubs)), nExpectedValues)));
    nSubs++;
  }

  for (int i = 0; i < numDeltaSubs; i++) {
    subIds.emplace(nSubs, SubscriberId(fmt::format("delta_sub_{}", i)));
    asyncScope.add(co_withExecutor(
        executor.get(),
        helper.addDeltaSubscription(
            std::move(subIds.at(nSubs)), nExpectedValues)));
    nSubs++;
  }

  suspender.dismiss();

  helper.setStorageData();

  folly::coro::blockingWait(asyncScope.joinAsync());

  suspender.rehire();
}

void bm_serve_update_state(
    uint32_t /* unused */,
    uint32_t numPatchSubs,
    uint32_t numPathSubs,
    uint32_t numDeltaSubs) {
  folly::BenchmarkSuspender suspender;

  test_data::TestDataFactory dataGen(test_data::RoleSelector::MaxScale);
  StorageBenchmarkHelper<> helper(
      dataGen,
      StorageBenchmarkHelper<>::Params().setNumUpdates(kNumIncrementalUpdates));
  helper.startStorage();

  folly::coro::AsyncScope asyncScope;
  auto numThreads = numPatchSubs + numPathSubs + numDeltaSubs;
  int nExpectedValues = kNumIncrementalUpdates + 1;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(numThreads);

  folly::Baton<> updateReceived;

  std::optional<std::function<void()>> updateReceivedCb = [&]() {
    updateReceived.post();
  };

  std::map<int, SubscriptionIdentifier> subIds;
  int nSubs = 0;

  for (int i = 0; i < numPatchSubs; i++) {
    subIds.emplace(nSubs, SubscriberId(fmt::format("patch_sub_{}", i)));
    asyncScope.add(co_withExecutor(
        executor.get(),
        helper.addPatchSubscription(
            std::move(subIds.at(nSubs)), nExpectedValues, updateReceivedCb)));
    nSubs++;
    updateReceivedCb = std::nullopt;
  }

  for (int i = 0; i < numPathSubs; i++) {
    subIds.emplace(nSubs, SubscriberId(fmt::format("path_sub_{}", i)));
    asyncScope.add(co_withExecutor(
        executor.get(),
        helper.addPathSubscription(
            std::move(subIds.at(nSubs)), nExpectedValues, updateReceivedCb)));
    nSubs++;
    updateReceivedCb = std::nullopt;
  }

  for (int i = 0; i < numDeltaSubs; i++) {
    subIds.emplace(nSubs, SubscriberId(fmt::format("delta_sub_{}", i)));
    asyncScope.add(co_withExecutor(
        executor.get(),
        helper.addDeltaSubscription(
            std::move(subIds.at(nSubs)), nExpectedValues, updateReceivedCb)));
    nSubs++;
    updateReceivedCb = std::nullopt;
  }

  // wait for initial sync to complete
  updateReceived.wait();
  updateReceived.reset();

  suspender.dismiss();

  for (int version = 1; version <= kNumIncrementalUpdates; version++) {
    helper.setStorageData(version);
    updateReceived.wait();
    updateReceived.reset();
  }

  folly::coro::blockingWait(asyncScope.joinAsync());

  suspender.rehire();
}

namespace {

void bm_ribmap_pubsub_mem(
    folly::UserCounters& counters,
    unsigned /* iters */,
    int prefixScale,
    int paths,
    int num_subscribers) {
  auto scale =
      test_data::BgpRibMapDataGenerator::makeGtswScale(prefixScale, paths);
  test_data::BgpRibMapDataGenerator gen(test_data::RoleSelector::GTSW, scale);
  std::vector<std::string> rootPath;
  auto subscribeFunc = [&](auto& storage, SubscriptionIdentifier&& subId) {
    return storage.subscribe_patch(
        std::move(subId), rootPath.begin(), rootPath.end());
  };
  StorageBenchmarkHelper<test_data::BgpRibMapDataGenerator::RootT>::
      reportPubSubMemStats(
          counters,
          gen,
          num_subscribers,
          FLAGS_bm_subbench_memory_iters,
          subscribeFunc);
}

// FPF canonicalRib pub/sub memory: measures fanout of the compact,
// best-path-only bgpData.canonicalRib() payload (numPods x numPrefixesPerPod
// entries) that HostReachTracker subscribes to, instead of ribMap.
void bm_canonicalrib_pubsub_mem(
    folly::UserCounters& counters,
    unsigned /* iters */,
    int numPods,
    int numPrefixesPerPod,
    int num_subscribers) {
  auto scale = test_data::BgpRibMapDataGenerator::makeGtswScale(
      /*isFPF=*/true, numPods, numPrefixesPerPod);
  test_data::BgpRibMapDataGenerator gen(test_data::RoleSelector::GTSW, scale);
  auto subscribeFunc = [&](auto& storage, SubscriptionIdentifier&& subId) {
    auto extPath = ext_path_builder::raw("bgp")
                       .raw("canonicalRib")
                       .raw("rib_entries")
                       .any()
                       .raw("best_path")
                       .get();
    return storage.subscribe_patch_extended(
        std::move(subId), {{0, std::move(extPath)}});
  };
  StorageBenchmarkHelper<test_data::BgpRibMapDataGenerator::RootT>::
      reportPubSubMemStats(
          counters,
          gen,
          num_subscribers,
          FLAGS_bm_subbench_memory_iters,
          subscribeFunc);
}

} // namespace

BENCHMARK_COUNTERS_NAME_PARAM(
    bm_ribmap_pubsub_mem,
    counters,
    GTSW_10K_36P_S1,
    10000,
    36,
    1);

BENCHMARK_COUNTERS_NAME_PARAM(
    bm_ribmap_pubsub_mem,
    counters,
    GTSW_10K_120P_S1,
    10000,
    120,
    1);

BENCHMARK_COUNTERS_NAME_PARAM(
    bm_ribmap_pubsub_mem,
    counters,
    GTSW_10K_36P_S144,
    10000,
    36,
    144);

BENCHMARK_COUNTERS_NAME_PARAM(
    bm_ribmap_pubsub_mem,
    counters,
    GTSW_70K_1P_S1,
    70000,
    1,
    1);

BENCHMARK_COUNTERS_NAME_PARAM(
    bm_ribmap_pubsub_mem,
    counters,
    GTSW_70K_120P_S1,
    70000,
    120,
    1);

// FPF canonicalRib: 144 pods x 240 prefixes/pod = 34560 entries, single
// subscriber (matches inject_bgp_prefixes --pods 144 --prefixes-per-pod 240).
BENCHMARK_COUNTERS_NAME_PARAM(
    bm_canonicalrib_pubsub_mem,
    counters,
    FPF_144x240_S1,
    144,
    240,
    1);

BENCHMARK_COUNTERS_NAME_PARAM(
    bm_canonicalrib_pubsub_mem,
    counters,
    FPF_144x240_S240,
    144,
    240,
    240);

// YUGE scale: 196 pods * 216 GPUs/pod = ~42K prefixes / vf
BENCHMARK_COUNTERS_NAME_PARAM(
    bm_canonicalrib_pubsub_mem,
    counters,
    FPF_196x216_S216,
    196,
    216,
    216);

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

BENCHMARK_NAMED_PARAM(bm_serve_initialSync, subscribers_1_path, 0, 1, 0);

BENCHMARK_NAMED_PARAM(bm_serve_initialSync, subscribers_1_delta, 0, 0, 1);

BENCHMARK_NAMED_PARAM(bm_serve_initialSync, subscribers_1_patch, 1, 0, 0);

BENCHMARK_NAMED_PARAM(bm_serve_update_state, subs_1_path, 0, 1, 0);

BENCHMARK_NAMED_PARAM(bm_serve_update_state, subs_1_delta, 0, 0, 1);

BENCHMARK_NAMED_PARAM(bm_serve_update_state, subs_1_patch, 1, 0, 0);

BENCHMARK_NAMED_PARAM(bm_serve_update_state, subs_1_patch_1_delta, 1, 0, 1);

} // namespace facebook::fboss::fsdb::test

int main(int argc, char* argv[]) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
