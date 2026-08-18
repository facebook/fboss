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
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr auto kPeakSampleIntervalUsec = 500;
constexpr auto kInitialSyncPollIntervalMsec = 1;
constexpr auto kInitialSyncTimeoutSec = 60;
constexpr auto kUpdateTimeoutSec = 300;

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

using facebook::fboss::fsdb::SubscriberMessage;

// Awaits one message carrying a chunk, skipping heartbeats so they cannot
// satisfy a wait meant to observe served data. The message is moved out of the
// generator frame and destroyed before returning, so the subscriber's copy of
// the served chunk is not charged to the storage being measured.
template <typename Generator>
folly::coro::Task<void> awaitChunk(Generator& generator, int subIndex) {
  while (true) {
    auto item = co_await generator.next();
    XCHECK(item.has_value())
        << "subscriber " << subIndex << " stream ended before a chunk arrived";
    bool isChunk{false};
    {
      auto element = std::move(*item);
      isChunk = element.val.getType() == SubscriberMessage::Type::chunk;
    }
    // `element` is destroyed here, freeing the chunk before we return.
    if (isChunk) {
      co_return;
    }
  }
}
} // namespace

namespace facebook::fboss::fsdb::test {

// A subscription is registered, resolved and initial-synced within one serve
// cycle, so a subscriber stamped with an initial sync timestamp is ready to be
// served the next publish. Distinct subscriber ids are counted because a
// resolved child shares its parent's subscriber id. A non-zero
// enqueuedDataSize marks a subscriber that must drain an initial-sync chunk
// before the publish; heartbeats enqueue zero bytes.
template <typename RootT>
int StorageBenchmarkHelper<RootT>::waitForInitialSync(int numSubscribers) {
  auto deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(kInitialSyncTimeoutSec);
  while (true) {
    std::map<std::string, bool> syncedSubscribers;
    for (const auto& info : storage_.getSubscriptions()) {
      if (info.initialSyncCompletedAt().value_or(0) == 0) {
        continue;
      }
      // Only the extended parent writes to the pipe, so OR across the entries
      // sharing a subscriber id.
      syncedSubscribers[*info.subscriberId()] |=
          info.enqueuedDataSize().value_or(0) > 0;
    }
    if (syncedSubscribers.size() >= static_cast<size_t>(numSubscribers)) {
      auto numServed = std::count_if(
          syncedSubscribers.begin(),
          syncedSubscribers.end(),
          [](const auto& entry) { return entry.second; });
      XCHECK(
          numServed == 0 ||
          numServed == static_cast<int64_t>(syncedSubscribers.size()))
          << "initial sync served " << numServed << " of "
          << syncedSubscribers.size() << " subscribers; expected all or none";
      return numServed > 0 ? 1 : 0;
    }
    XCHECK(std::chrono::steady_clock::now() < deadline)
        << "timed out waiting for initial sync: " << syncedSubscribers.size()
        << " of " << numSubscribers << " subscribers synced";
    std::this_thread::sleep_for(
        std::chrono::milliseconds(kInitialSyncPollIntervalMsec));
  }
}

template <typename RootT>
int64_t StorageBenchmarkHelper<RootT>::measurePubSubMemory(
    int numSubscribers,
    SubscribeFn subscribeFn,
    bool measurePeak) {
  folly::BenchmarkSuspender suspender;

  // (a) empty storage, then start serving subscriptions.
  startStorage();

  // (b) subscribe numSubscribers subscribers. Registering on this thread lets
  // the initial sync shape be observed before the draining tasks are launched.
  std::vector<PatchStreamReader> readers;
  readers.reserve(numSubscribers);
  for (int i = 0; i < numSubscribers; i++) {
    readers.push_back(subscribeFn(
        storage_,
        SubscriptionIdentifier(SubscriberId(fmt::format("patch_sub_{}", i)))));
  }

  // (c) PATH publisher registration on root path. Effective only when
  // trackMetadata=true; an explicit no-op otherwise.
  std::vector<std::string> rootPath = getSubscriptionPath();
  storage_.registerPublisher(
      rootPath.begin(),
      rootPath.end(),
      /*skipThriftStreamLivenessCheck=*/true);

  // (d) wait for every subscription to be registered and past initial sync, and
  // learn how many chunks initial sync served each subscriber.
  auto numInitialSyncChunks = waitForInitialSync(numSubscribers);

  folly::coro::AsyncScope scope;
  auto executor = std::make_unique<folly::CPUThreadPoolExecutor>(
      std::max(numSubscribers, 1));

  folly::Baton<> initSyncDone;
  folly::Baton<> updateReceived;
  std::atomic<int> initSyncCount{0};
  std::atomic<int> updateCount{0};

  auto subscriberTask =
      [&](int subIndex, PatchStreamReader reader) -> folly::coro::Task<void> {
    auto generator = std::move(reader.generator_);
    for (int chunk = 0; chunk < numInitialSyncChunks; chunk++) {
      co_await awaitChunk(generator, subIndex);
    }
    if (initSyncCount.fetch_add(1) + 1 == numSubscribers) {
      initSyncDone.post();
    }
    // Initial sync is served as a single message, so one chunk is the whole
    // published update.
    co_await awaitChunk(generator, subIndex);
    if (updateCount.fetch_add(1) + 1 == numSubscribers) {
      updateReceived.post();
    }
  };

  for (int i = 0; i < numSubscribers; i++) {
    scope.add(
        folly::coro::co_withExecutor(
            executor.get(), subscriberTask(i, std::move(readers[i]))));
  }

  // (e) all subscribers have drained their initial sync chunks.
  XCHECK(
      initSyncDone.try_wait_for(std::chrono::seconds(kInitialSyncTimeoutSec)))
      << "timed out waiting for " << numSubscribers << " subscribers to drain "
      << numInitialSyncChunks << " initial sync chunk(s)";

  // (f) begin measurement around publish + fanout.
  auto startAllocated = thrift_cow::test::getJemallocAllocatedBytes();
  std::unique_ptr<PeakAllocatedSampler> sampler;
  if (measurePeak) {
    sampler = std::make_unique<PeakAllocatedSampler>(startAllocated);
  }
  suspender.dismiss();

  // (g) publish state; wait for all subscribers to receive the update.
  setStorageData(0);
  XCHECK(updateReceived.try_wait_for(std::chrono::seconds(kUpdateTimeoutSec)))
      << "timed out waiting for " << numSubscribers
      << " subscribers to receive the published update";

  // (h) end measurement.
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
          gen, Params().setStartWithInitializedData(false));
      auto delta = helper.measurePubSubMemory(
          numSubscribers, subscribeFn, /*measurePeak=*/false);
      if (delta > 0) {
        allocatedMeasurements.push_back(delta);
      }
    }
    {
      StorageBenchmarkHelper helper(
          gen, Params().setStartWithInitializedData(false));
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
