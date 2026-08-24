// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/thrift_cow/storage/tests/TestDataFactory.h>
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath

#include <folly/Benchmark.h>
#include <folly/Function.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/coro/Timeout.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace facebook::fboss::fsdb::test {

namespace detail {
constexpr auto kSubscriptionServeIntervalMsec = 1;
// Heartbeats share the subscriber stream with data chunks, so a heartbeat can
// satisfy an await that is meant to observe a chunk -- which would cut a
// measured publish + fanout region short. Benchmarks never rely on heartbeats,
// so push the interval past any run.
constexpr auto kSubscriptionHeartbeatIntervalHours = 1;
} // namespace detail

template <typename Gen>
auto makeConsumer(
    Gen& generator,
    int nExpectedValues,
    const std::optional<std::function<void()>>& onDataReceived) {
  auto nextSubscribedValue =
      [](Gen& generator) -> folly::coro::Task<typename Gen::value_type> {
    auto item = co_await generator.next();
    auto&& value = *item;
    co_return std::move(value);
  };

  return [&generator,
          nExpectedValues,
          onDataReceived = onDataReceived,
          nextSubscribedValue]() mutable {
    while (nExpectedValues-- > 0) {
      auto val = folly::coro::blockingWait(
          folly::coro::timeout(
              nextSubscribedValue(generator), std::chrono::seconds(5)));
      if (onDataReceived.has_value()) {
        onDataReceived.value()();
      }
    }
  };
}

template <typename RootT = TestStruct>
class StorageBenchmarkHelper {
 public:
  using RootType = RootT;

  // The pub/sub memory members are de-templated on the subscription callable so
  // their definitions can live in the .cpp. The callable subscribes at the root
  // path and returns the patch stream reader whose `generator_` the helper
  // drives; patch-extended shares this type. (Delta returns a different stream
  // element and would need its own overload.)
  using PatchStreamReader = SubscriptionStreamReader<
      SubscriptionServeQueueElement<SubscriberMessage>>;
  using SubscribeFn = folly::FunctionRef<PatchStreamReader(
      NaivePeriodicSubscribableCowStorage<RootType>&,
      SubscriptionIdentifier&&)>;

  class Params {
   public:
    Params()
        : largeUpdates(false),
          serveGetRequestsWithLastPublishedState(true),
          startWithInitializedData(true),
          numUpdates(0) {}

    Params& setLargeUpdates(bool val) {
      largeUpdates = val;
      return *this;
    }

    Params& setStartWithInitializedData(bool val) {
      startWithInitializedData = val;
      return *this;
    }

    Params& setServeGetWithLastPublished(bool val) {
      serveGetRequestsWithLastPublishedState = val;
      return *this;
    }

    Params& setNumUpdates(int val) {
      numUpdates = val;
      return *this;
    }

    bool largeUpdates;
    bool serveGetRequestsWithLastPublishedState;
    bool startWithInitializedData;
    int numUpdates;
  };

  explicit StorageBenchmarkHelper(
      test_data::IDataGenerator& gen,
      Params params = Params())
      : gen_(gen),
        params_(params),
        // trackMetadata stays false because subscribing at the root path
        // triggers OperPathToPublisherRoot::checkNonEmpty() and throws when
        // trackMetadata is true. convertToIDPaths is forced on (required for
        // patch subscriptions). requireResponseOnInitialSync stays off: it only
        // covers resolved subscriptions, so it cannot provide a sync point for
        // a wildcard subscription that resolves to no paths on an empty root.
        storage_(
            NaivePeriodicSubscribableCowStorage<RootType>(
                {},
                NaivePeriodicSubscribableStorageBase::StorageParams(
                    std::chrono::milliseconds(
                        detail::kSubscriptionServeIntervalMsec),
                    std::chrono::hours(
                        detail::kSubscriptionHeartbeatIntervalHours),
                    /*trackMetadata=*/false,
                    "fsdb",
                    /*convertToIDPaths=*/true,
                    /*requireResponseOnInitialSync=*/false)
                    .setServeGetRequestsWithLastPublishedState(
                        params.serveGetRequestsWithLastPublishedState))) {
    // initialize test data versions
    testData_.emplace_back(gen_.getStateUpdate(0, false));
    for (int version = 0; version < params_.numUpdates; version++) {
      testData_.emplace_back(
          gen_.getStateUpdate(version, !params_.largeUpdates));
    }
    if (params_.startWithInitializedData) {
      setStorageData();
    }
  }

  void startStorage() {
    storage_.start();
  }

  void setStorageData(int version = 0) {
    CHECK_LE(version, params_.numUpdates);
    storage_.set_encoded(
        *testData_[version].path()->path(), *testData_[version].state());
  }

  folly::coro::Task<void> getRequest(uint32_t numReads) {
    for (auto count = 0; count < numReads; count++) {
      storage_.get_encoded(this->root.mapOfStructs(), OperProtocol::BINARY);
    }
    co_return;
  }

  folly::coro::Task<void> publishData(uint32_t numWrites) {
    CHECK_GE(params_.numUpdates, 1);
    for (auto count = 0; count < numWrites; count++) {
      int version = 1 + (count % 2);
      storage_.set_encoded(
          *testData_[version].path()->path(), *testData_[version].state());
    }
    co_return;
  }

  folly::coro::Task<void> addPathSubscription(
      SubscriptionIdentifier&& subscriberId,
      int nExpectedValues,
      std::optional<std::function<void()>> onDataReceived = std::nullopt) {
    std::vector<std::string> path = getSubscriptionPath();
    auto generator = storage_.template subscribe<
        RootType,
        apache::thrift::type_class::structure>(std::move(subscriberId), path);
    auto consumer = makeConsumer(generator, nExpectedValues, onDataReceived);
    consumer();
    co_return;
  }

  folly::coro::Task<void> addDeltaSubscription(
      SubscriptionIdentifier&& subscriberId,
      int nExpectedValues,
      std::optional<std::function<void()>> onDataReceived = std::nullopt) {
    std::vector<std::string> path = getSubscriptionPath();
    auto streamReader = storage_.subscribe_delta(
        std::move(subscriberId), path, OperProtocol::COMPACT);
    auto generator = std::move(streamReader.generator_);
    auto consumer = makeConsumer(generator, nExpectedValues, onDataReceived);
    consumer();
    co_return;
  }

  folly::coro::Task<void> addPatchSubscription(
      SubscriptionIdentifier&& subscriberId,
      int nExpectedValues,
      std::optional<std::function<void()>> onDataReceived = std::nullopt) {
    std::vector<std::string> path = getSubscriptionPath();
    auto streamReader = storage_.subscribe_patch(std::move(subscriberId), path);
    auto generator = std::move(streamReader.generator_);
    auto consumer = makeConsumer(generator, nExpectedValues, onDataReceived);
    consumer();
    co_return;
  }

  // Measures memory consumed by publishing one state update and fanning it out
  // to `numSubscribers` subscribers created by `subscribeFn`. Returns the delta
  // in jemalloc `stats.allocated` across the measured region, or the peak delta
  // observed within it when `measurePeak` is set. `subscribeFn` is called as
  // `subscribeFn(storage_, SubscriptionIdentifier&&)` and returns the patch
  // stream reader whose `generator_` the helper drives.
  //
  // Subscribers free each message as they receive it: this measures
  // storage-side memory while serving the publish, not the subscribers' copies
  // of it.
  //
  // One instance measures once: the storage and its subscriptions are consumed.
  // Defined in the .cpp (explicitly instantiated) so the AsyncScope / executor
  // / Baton / jemalloc machinery stays out of this header.
  int64_t measurePubSubMemory(
      int numSubscribers,
      SubscribeFn subscribeFn,
      bool measurePeak);

  // Runs `iterations` measurement passes and reports avg/max/stddev counters
  // for both the allocated delta and the observed peak. Each pass constructs a
  // fresh helper (storage + subscriptions are consumed by measurePubSubMemory),
  // running the delta and peak passes separately so the peak sampler's polling
  // does not perturb the delta measurement. Defined in the .cpp.
  static void reportPubSubMemStats(
      folly::UserCounters& counters,
      test_data::IDataGenerator& gen,
      int numSubscribers,
      int iterations,
      SubscribeFn subscribeFn);

 private:
  // Blocks until the storage reports `numSubscribers` distinct subscribers
  // registered and past initial sync, and returns how many chunks initial sync
  // served each of them. That is 1 for a fully resolved path (its node exists
  // on the empty root, so its encoded value is served) and 0 for a wildcard
  // path (nothing matches an empty root, so it resolves to no paths). Defined
  // in the .cpp.
  int waitForInitialSync(int numSubscribers);

  std::vector<std::string> getSubscriptionPath() {
    std::vector<std::string> path;
    return path;
  }

  test_data::IDataGenerator& gen_;
  Params params_;
  thriftpath::RootThriftPath<RootType> root;
  NaivePeriodicSubscribableCowStorage<RootType> storage_;
  std::vector<TaggedOperState> testData_;
};

} // namespace facebook::fboss::fsdb::test
