// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/thrift_cow/storage/tests/TestDataFactory.h>
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/coro/Timeout.h>

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace facebook::fboss::fsdb::test {

namespace detail {
constexpr auto kSubscriptionServeIntervalMsec = 1;
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

  class Params {
   public:
    Params()
        : largeUpdates(false),
          serveGetRequestsWithLastPublishedState(true),
          startWithInitializedData(true),
          requireResponseOnInitialSync(false),
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

    Params& setRequireResponseOnInitialSync(bool val) {
      requireResponseOnInitialSync = val;
      return *this;
    }

    Params& setNumUpdates(int val) {
      numUpdates = val;
      return *this;
    }

    bool largeUpdates;
    bool serveGetRequestsWithLastPublishedState;
    bool startWithInitializedData;
    bool requireResponseOnInitialSync;
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
        // patch subscriptions). requireResponseOnInitialSync, when set, ensures
        // each subscriber receives an initial sync value on attach even when
        // the storage is empty, giving callers a known sync point before
        // measurement starts.
        storage_(
            NaivePeriodicSubscribableCowStorage<RootType>(
                {},
                NaivePeriodicSubscribableStorageBase::StorageParams(
                    std::chrono::milliseconds(
                        detail::kSubscriptionServeIntervalMsec),
                    std::chrono::seconds(5),
                    /*trackMetadata=*/false,
                    "fsdb",
                    /*convertToIDPaths=*/true,
                    /*requireResponseOnInitialSync=*/
                    params.requireResponseOnInitialSync)
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

 private:
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
