// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

// StorageBenchmarkHelper is a class template. Its lightweight members are
// header-inline; the pub/sub memory members are de-templated (SubscribeFn is a
// concrete folly::FunctionRef) and defined here, explicitly instantiated for
// the root type used by the pub/sub benchmarks. This keeps the AsyncScope /
// executor / Baton / jemalloc machinery out of the exported header.
#include "fboss/fsdb/oper/tests/SubscribableStorageBenchHelper.h"
#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

#include <folly/Benchmark.h>
#include <folly/coro/AsyncScope.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/synchronization/Baton.h>

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace {
constexpr auto kPeakSampleIntervalUsec = 500;

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
                peakBytes_,
                facebook::fboss::thrift_cow::test::getJemallocAllocatedBytes());
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
} // namespace

namespace facebook::fboss::fsdb::test {

template <typename RootT>
int64_t StorageBenchmarkHelper<RootT>::measurePubSubMemory(
    int numSubscribers,
    SubscribeFn subscribeFn,
    bool measurePeak) {
  folly::BenchmarkSuspender suspender;

  // (a) empty storage, then start serving subscriptions.
  startStorage();

  // (b) numSubscribers subscriber tasks subscribed to the root path.
  std::vector<std::string> rootPath = getSubscriptionPath();
  folly::coro::AsyncScope scope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(
      std::max(numSubscribers, 1));

  folly::Baton<> initSyncDone;
  folly::Baton<> updateReceived;
  std::atomic<int> initSyncCount{0};
  std::atomic<int> updateCount{0};

  auto subscriberTask = [&](int subIndex) -> folly::coro::Task<void> {
    auto streamReader = subscribeFn(
        storage_,
        SubscriptionIdentifier(
            SubscriberId(fmt::format("patch_sub_{}", subIndex))));
    auto generator = std::move(streamReader.generator_);
    co_await generator.next();
    if (initSyncCount.fetch_add(1) + 1 == numSubscribers) {
      initSyncDone.post();
    }
    co_await generator.next();
    if (updateCount.fetch_add(1) + 1 == numSubscribers) {
      updateReceived.post();
    }
  };

  for (int i = 0; i < numSubscribers; i++) {
    scope.add(folly::coro::co_withExecutor(executor.get(), subscriberTask(i)));
  }

  // (c) PATH publisher registration on root path. Effective only when
  // trackMetadata=true; an explicit no-op otherwise.
  storage_.registerPublisher(
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
  setStorageData(0);
  updateReceived.wait();

  // (g) end measurement.
  suspender.rehire();
  auto endAllocated = thrift_cow::test::getJemallocAllocatedBytes();
  auto measuredAllocated =
      sampler ? sampler->stopAndGetPeak(endAllocated) : endAllocated;

  folly::coro::blockingWait(scope.joinAsync());
  return measuredAllocated - startAllocated;
}

template <typename RootT>
void StorageBenchmarkHelper<RootT>::reportPubSubMemStats(
    folly::UserCounters& counters,
    test_data::IDataGenerator& gen,
    int numSubscribers,
    int iterations,
    SubscribeFn subscribeFn) {
  std::vector<int64_t> allocatedMeasurements;
  std::vector<int64_t> peakMeasurements;
  for (int i = 0; i < iterations; i++) {
    {
      StorageBenchmarkHelper helper(
          gen,
          Params()
              .setStartWithInitializedData(false)
              .setRequireResponseOnInitialSync(true));
      auto delta = helper.measurePubSubMemory(numSubscribers, subscribeFn);
      if (delta > 0) {
        allocatedMeasurements.push_back(delta);
      }
    }
    {
      StorageBenchmarkHelper helper(
          gen,
          Params()
              .setStartWithInitializedData(false)
              .setRequireResponseOnInitialSync(true));
      auto peak = helper.measurePubSubMemory(
          numSubscribers, subscribeFn, /*measurePeak=*/true);
      if (peak > 0) {
        peakMeasurements.push_back(peak);
      }
    }
  }

  // Deltas of jemalloc `stats.allocated` across publish + fanout.
  if (auto stats =
          thrift_cow::test::computeMemoryStats(allocatedMeasurements)) {
    thrift_cow::test::reportMemoryCounters(counters, "allocated", *stats);
  }
  if (auto stats = thrift_cow::test::computeMemoryStats(peakMeasurements)) {
    thrift_cow::test::reportMemoryCounters(counters, "peak", *stats);
  }
}

// Explicit instantiation for the root type used by the pub/sub benchmarks.
// Only these members are instantiated, so root types whose model lacks the
// header-inline members' fields (e.g. TestStruct's mapOfStructs()) are never
// forced to compile those bodies.
using RibRoot = test_data::BgpRibMapDataGenerator::RootT;
using RibHelper = StorageBenchmarkHelper<RibRoot>;

template int64_t
RibHelper::measurePubSubMemory(int, RibHelper::SubscribeFn, bool);
template void RibHelper::reportPubSubMemStats(
    folly::UserCounters&,
    test_data::IDataGenerator&,
    int,
    int,
    RibHelper::SubscribeFn);

} // namespace facebook::fboss::fsdb::test
