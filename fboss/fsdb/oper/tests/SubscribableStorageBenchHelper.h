// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/thrift_cow/storage/tests/TestDataFactory.h>
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath

namespace facebook::fboss::fsdb::test {

class StorageBenchmarkHelper {
 public:
  using RootType = TestStruct;

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
      Params params = Params());

 public:
  void startStorage();

  void setStorageData(int version = 0);

  folly::coro::Task<void> getRequest(uint32_t numReads);

  folly::coro::Task<void> publishData(uint32_t numWrites);

  folly::coro::Task<void> addPathSubscription(
      SubscriptionIdentifier&& subscriberId,
      int nExpectedValues,
      std::optional<std::function<void()>> onDataReceived = std::nullopt);

  folly::coro::Task<void> addDeltaSubscription(
      SubscriptionIdentifier&& subscriberId,
      int nExpectedValues,
      std::optional<std::function<void()>> onDataReceived = std::nullopt);

  folly::coro::Task<void> addPatchSubscription(
      SubscriptionIdentifier&& subscriberId,
      int nExpectedValues,
      std::optional<std::function<void()>> onDataReceived = std::nullopt);

 private:
  std::vector<std::string> getSubscriptionPath();

  test_data::IDataGenerator& gen_;
  Params params_;
  thriftpath::RootThriftPath<RootType> root;
  NaivePeriodicSubscribableCowStorage<RootType> storage_;
  std::vector<TaggedOperState> testData_;
};

} // namespace facebook::fboss::fsdb::test
