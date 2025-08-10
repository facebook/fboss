// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/tests/SubscribableStorageBenchHelper.h"
#include <folly/coro/BlockingWait.h>

namespace facebook::fboss::fsdb::test {

StorageBenchmarkHelper::StorageBenchmarkHelper(
    test_data::IDataGenerator& gen,
    Params params)
    : gen_(gen),
      params_(params),
      storage_(NaivePeriodicSubscribableCowStorage<RootType>(
          {},
          NaivePeriodicSubscribableStorageBase::StorageParams()
              .setServeGetRequestsWithLastPublishedState(
                  params_.serveGetRequestsWithLastPublishedState))) {
  storage_.setConvertToIDPaths(true);
  // initialize test data versions
  testData_.emplace_back(gen_.getStateUpdate(0, false));
  for (int version = 0; version < 2; version++) {
    testData_.emplace_back(gen_.getStateUpdate(version, !params_.largeUpdates));
  }
  if (params_.startWithInitializedData) {
    setStorageData();
  }
}

void StorageBenchmarkHelper::startStorage() {
  storage_.start();
}

void StorageBenchmarkHelper::setStorageData(int version) {
  storage_.set_encoded(
      *testData_[version].path()->path(), *testData_[version].state());
}

folly::coro::Task<void> StorageBenchmarkHelper::getRequest(uint32_t numReads) {
  for (auto count = 0; count < numReads; count++) {
    storage_.get_encoded(this->root.mapOfStructs(), OperProtocol::BINARY);
  }
  co_return;
}

folly::coro::Task<void> StorageBenchmarkHelper::publishData(
    uint32_t numWrites) {
  for (auto count = 0; count < numWrites; count++) {
    int version = 1 + (count % 2);
    storage_.set_encoded(
        *testData_[version].path()->path(), *testData_[version].state());
  }
  co_return;
}

template <typename Gen>
auto makeConsumer(Gen& generator, int nExpectedValues) {
  auto nextSubscribedValue =
      [](Gen& generator) -> folly::coro::Task<typename Gen::value_type> {
    auto item = co_await generator.next();
    auto&& value = *item;
    co_return std::move(value);
  };

  return [&generator, nExpectedValues, nextSubscribedValue]() mutable {
    while (nExpectedValues-- > 0) {
      auto val = folly::coro::blockingWait(folly::coro::timeout(
          nextSubscribedValue(generator), std::chrono::seconds(5)));
    }
  };
}

folly::coro::Task<void> StorageBenchmarkHelper::addPathSubscription(
    SubscriptionIdentifier&& subscriberId,
    int nExpectedValues) {
  std::vector<std::string> path = getSubscriptionPath();
  auto generator =
      storage_.subscribe<RootType, apache::thrift::type_class::structure>(
          std::move(subscriberId), path);
  auto consumer = makeConsumer(generator, nExpectedValues);
  consumer();
  co_return;
}

folly::coro::Task<void> StorageBenchmarkHelper::addDeltaSubscription(
    SubscriptionIdentifier&& subscriberId,
    int nExpectedValues) {
  std::vector<std::string> path = getSubscriptionPath();
  auto generator = storage_.subscribe_delta(
      std::move(subscriberId), path, OperProtocol::COMPACT);
  auto consumer = makeConsumer(generator, nExpectedValues);
  consumer();
  co_return;
}

folly::coro::Task<void> StorageBenchmarkHelper::addPatchSubscription(
    SubscriptionIdentifier&& subscriberId,
    int nExpectedValues) {
  std::vector<std::string> path = getSubscriptionPath();
  auto generator = storage_.subscribe_patch(std::move(subscriberId), path);
  auto consumer = makeConsumer(generator, nExpectedValues);
  consumer();
  co_return;
}

std::vector<std::string> StorageBenchmarkHelper::getSubscriptionPath() {
  std::vector<std::string> path;
  return path;
}

} // namespace facebook::fboss::fsdb::test
