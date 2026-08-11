// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/coro/AsyncScope.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/coro/Timeout.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/synchronization/Baton.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include <fboss/fsdb/if/FsdbModel.h>
#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/thrift_cow/nodes/Serializer.h>
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
constexpr auto kSubscriptionServeIntervalMsec = 1;
constexpr auto kPeakSampleIntervalUsec = 500;
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
      dataGen,
      StorageBenchmarkHelper::Params()
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
  StorageBenchmarkHelper helper(
      dataGen,
      StorageBenchmarkHelper::Params()
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
  StorageBenchmarkHelper helper(
      dataGen,
      StorageBenchmarkHelper::Params().setStartWithInitializedData(false));
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
  StorageBenchmarkHelper helper(
      dataGen,
      StorageBenchmarkHelper::Params().setNumUpdates(kNumIncrementalUpdates));
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

// Empty-storage construction. trackMetadata stays false because subscribing at
// the root path triggers OperPathToPublisherRoot::checkNonEmpty() and throws
// when trackMetadata is true. With trackMetadata=false, registerPublisher() is
// an explicit no-op (early-returns), but set_encoded() still drives the
// per-subscription serve loop and delivers updates to patch subscribers.
// requireResponseOnInitialSync=true ensures each subscriber receives an initial
// sync value on attach even when the storage is empty, giving the helper a
// known sync point before measurement starts.
template <typename RootT>
std::unique_ptr<NaivePeriodicSubscribableCowStorage<RootT>>
initEmptySubscribableStorage() {
  return std::make_unique<NaivePeriodicSubscribableCowStorage<RootT>>(
      RootT{},
      NaivePeriodicSubscribableStorageBase::StorageParams(
          std::chrono::milliseconds(kSubscriptionServeIntervalMsec),
          std::chrono::seconds(5),
          /*trackMetadata=*/false,
          "fsdb",
          /*convertToIDPaths=*/true,
          /*requireResponseOnInitialSync=*/true));
}

// Samples jemalloc `stats.allocated` on a background thread and keeps the
// maximum. Needed because the peak of the publish + fanout path is transient:
// patch buffers are built and freed within the measured region, so start/end
// snapshots alone never see it.
class PeakAllocatedSampler {
 public:
  explicit PeakAllocatedSampler(int64_t baselineBytes)
      : peakBytes_(baselineBytes), thread_([this]() {
          while (!stop_.load(std::memory_order_relaxed)) {
            peakBytes_ = std::max(
                peakBytes_, thrift_cow::test::getJemallocAllocatedBytes());
            std::this_thread::sleep_for(
                std::chrono::microseconds(kPeakSampleIntervalUsec));
          }
        }) {}

  ~PeakAllocatedSampler() {
    stop();
  }

  // Stops sampling and returns the max of all samples and `endBytes`.
  int64_t stopAndGetPeak(int64_t endBytes) {
    stop();
    return std::max(peakBytes_, endBytes);
  }

 private:
  void stop() {
    if (thread_.joinable()) {
      stop_.store(true, std::memory_order_relaxed);
      thread_.join();
    }
  }

  int64_t peakBytes_;
  std::atomic<bool> stop_{false};
  std::thread thread_;
};

// Measures memory consumed by publishing one state update and fanning it out
// to `num_subscribers` patch subscribers. Returns the delta in jemalloc
// `stats.allocated` across the measured region, or the peak delta observed
// within it when `measurePeak` is set.
int64_t bm_ribmap_pubsub_mem_helper(
    test_data::BgpRibMapDataGenerator& gen,
    int num_subscribers,
    bool measurePeak = false) {
  using RootT = test_data::BgpRibMapDataGenerator::RootT;

  folly::BenchmarkSuspender suspender;
  auto state = gen.getStateUpdate(0, false);

  // (a) empty storage, then start serving subscriptions.
  auto storage = initEmptySubscribableStorage<RootT>();
  storage->start();

  // (b) num_subscribers patch-subscriber tasks subscribed to root path.
  std::vector<std::string> rootPath;
  folly::coro::AsyncScope scope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(
      std::max(num_subscribers, 1));

  folly::Baton<> initSyncDone;
  folly::Baton<> updateReceived;
  std::atomic<int> initSyncCount{0};
  std::atomic<int> updateCount{0};

  auto subscriberTask = [&](int subIndex) -> folly::coro::Task<void> {
    auto streamReader = storage->subscribe_patch(
        SubscriptionIdentifier(
            SubscriberId(fmt::format("patch_sub_{}", subIndex))),
        rootPath.begin(),
        rootPath.end());
    auto generator = std::move(streamReader.generator_);
    co_await generator.next();
    if (initSyncCount.fetch_add(1) + 1 == num_subscribers) {
      initSyncDone.post();
    }
    co_await generator.next();
    if (updateCount.fetch_add(1) + 1 == num_subscribers) {
      updateReceived.post();
    }
  };

  for (int i = 0; i < num_subscribers; i++) {
    scope.add(co_withExecutor(executor.get(), subscriberTask(i)));
  }

  // (c) PATH publisher registration on root path. Effective only when
  // trackMetadata=true (see initEmptySubscribableStorage).
  storage->registerPublisher(
      rootPath.begin(),
      rootPath.end(),
      /*skipThriftStreamLivenessCheck=*/true);

  // (d) wait for all subscribers to receive initial sync.
  initSyncDone.wait();

  // (e) begin measurement around publish + fanout.
  auto startAllocated = thrift_cow::test::getJemallocAllocatedBytes();
  std::unique_ptr<PeakAllocatedSampler> sampler;
  if (measurePeak) {
    sampler = std::make_unique<PeakAllocatedSampler>(startAllocated);
  }
  suspender.dismiss();

  // (f) publish state; wait for all subscribers to receive the update.
  storage->set_encoded(*state.path()->path(), *state.state());
  updateReceived.wait();

  // (g) end measurement.
  suspender.rehire();
  auto endAllocated = thrift_cow::test::getJemallocAllocatedBytes();
  auto measuredAllocated =
      sampler ? sampler->stopAndGetPeak(endAllocated) : endAllocated;

  folly::coro::blockingWait(scope.joinAsync());
  return measuredAllocated - startAllocated;
}

struct MemoryStats {
  double avgKB{0.0};
  double maxKB{0.0};
  double stddevKB{0.0};
};

std::optional<MemoryStats> computeMemoryStats(
    const std::vector<int64_t>& measurements) {
  if (measurements.empty()) {
    return std::nullopt;
  }

  int64_t sum = 0;
  for (int64_t m : measurements) {
    sum += m;
  }
  int64_t avg = sum / static_cast<int64_t>(measurements.size());
  int64_t max = *std::max_element(measurements.begin(), measurements.end());

  // Sample stddev requires at least 2 points.
  double stddev = 0.0;
  if (measurements.size() > 1) {
    double variance = 0.0;
    for (int64_t m : measurements) {
      double diff = static_cast<double>(m - avg);
      variance += diff * diff;
    }
    variance /= static_cast<double>(measurements.size() - 1);
    stddev = std::sqrt(variance);
  }

  return MemoryStats{
      static_cast<double>(avg) / 1024.0,
      static_cast<double>(max) / 1024.0,
      stddev / 1024.0};
}

void bm_pubsub_mem_stats(
    folly::UserCounters& counters,
    test_data::BgpRibMapDataGenerator& gen,
    int num_subscribers) {
  std::vector<int64_t> allocatedMeasurements;
  std::vector<int64_t> peakMeasurements;
  for (int i = 0; i < FLAGS_bm_subbench_memory_iters; i++) {
    // Separate passes: the peak sampler polls the allocator during the measured
    // region, so keeping it out of the delta pass leaves that value
    // unperturbed.
    auto delta = bm_ribmap_pubsub_mem_helper(gen, num_subscribers);
    if (delta > 0) {
      allocatedMeasurements.push_back(delta);
    }
    auto peak =
        bm_ribmap_pubsub_mem_helper(gen, num_subscribers, /*measurePeak=*/true);
    if (peak > 0) {
      peakMeasurements.push_back(peak);
    }
  }

  // Deltas of jemalloc `stats.allocated` across publish + fanout.
  if (auto stats = computeMemoryStats(allocatedMeasurements)) {
    counters["avg_allocated_KB"] = folly::UserMetric(stats->avgKB);
    counters["max_allocated_KB"] = folly::UserMetric(stats->maxKB);
    counters["stddev_allocated_KB"] = folly::UserMetric(stats->stddevKB);
  }
  if (auto stats = computeMemoryStats(peakMeasurements)) {
    counters["avg_peak_KB"] = folly::UserMetric(stats->avgKB);
    counters["max_peak_KB"] = folly::UserMetric(stats->maxKB);
    counters["stddev_peak_KB"] = folly::UserMetric(stats->stddevKB);
  }
}

void bm_ribmap_pubsub_mem(
    folly::UserCounters& counters,
    unsigned /* iters */,
    int prefixScale,
    int paths,
    int num_subscribers) {
  auto scale =
      test_data::BgpRibMapDataGenerator::makeGtswScale(prefixScale, paths);
  test_data::BgpRibMapDataGenerator gen(test_data::RoleSelector::GTSW, scale);
  bm_pubsub_mem_stats(counters, gen, num_subscribers);
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
  bm_pubsub_mem_stats(counters, gen, num_subscribers);
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
