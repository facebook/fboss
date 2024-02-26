// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/json/dynamic.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <utility>

#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/lib/CommonUtils.h>
#include <fboss/thrift_cow/visitors/PatchBuilder.h>
#include <folly/Random.h>
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/GtestHelpers.h>
#include <folly/experimental/coro/Task.h>
#include <folly/experimental/coro/Timeout.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

using folly::dynamic;
using namespace testing;

namespace {

using namespace facebook::fboss::fsdb;

// TODO: templatize test cases
using TestSubscribableStorage = NaivePeriodicSubscribableCowStorage<TestStruct>;

dynamic createTestDynamic() {
  return dynamic::object("tx", true)(
      "rx", false)("name", "testname")("optionalString", "bla")("enumeration", 1)("enumMap", dynamic::object)("member", dynamic::object("min", 10)("max", 20))("variantMember", dynamic::object("integral", 99))("structMap", dynamic::object(3, dynamic::object("min", 100)("max", 200)))("structList", dynamic::array())("enumSet", dynamic::array())("integralSet", dynamic::array())("mapOfStringToI32", dynamic::object())("listOfPrimitives", dynamic::array())("setOfI32", dynamic::array());
}

TestStruct createTestStructForExtendedTests() {
  auto testDyn = createTestDynamic();
  for (int i = 0; i <= 20; ++i) {
    testDyn["mapOfStringToI32"][fmt::format("test{}", i)] = i;
    testDyn["listOfPrimitives"].push_back(i);
    testDyn["setOfI32"].push_back(i);
  }

  return facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
}

constexpr auto kSubscriber = "testSubscriber";

template <typename Gen>
folly::coro::Task<typename Gen::value_type> consumeOne(Gen& generator) {
  auto item = co_await generator.next();
  auto&& value = *item;
  co_return std::move(value);
}

} // namespace

TEST(SubscribableStorageTests, GetThrift) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));
  EXPECT_EQ(storage.get(root).value(), testStruct);
}

TEST(SubscribableStorageTests, SubscribeUnsubscribe) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto txPath = root.tx();
  storage.start();
  {
    auto generator = storage.subscribe(kSubscriber, std::move(txPath));
    EXPECT_EQ(storage.numSubscriptions(), 1);
  }
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), 0));
}

TEST(SubscribableStorageTests, SubscribeOne) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);

  auto txPath = root.tx();
  storage.start();
  auto generator = storage.subscribe(kSubscriber, std::move(txPath));
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.newVal, true);
  EXPECT_EQ(deltaVal.oldVal, std::nullopt);
  EXPECT_EQ(storage.set(root.tx(), false), std::nullopt);

  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.oldVal, true);
  EXPECT_EQ(deltaVal.newVal, false);
}

TEST(SubscribableStorageTests, SubscribePathAddRemoveParent) {
  // add subscription for a path that doesn't exist yet, then add parent
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto path = root.structMap()[99].min();
  auto generator = storage.subscribe(kSubscriber, std::move(path));
  storage.start();

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(root.structMap()[99], newStruct), std::nullopt);
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.oldVal, std::nullopt);
  EXPECT_EQ(deltaVal.newVal, 999);

  // now delete the parent and verify we see the deletion delta too
  storage.remove(root.structMap()[99]);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.oldVal, 999);
  EXPECT_EQ(deltaVal.newVal, std::nullopt);
}

TEST(SubscribableStorageTests, SubscribeDelta) {
  using namespace facebook::fboss::fsdb;

  FLAGS_serveHeartbeats = true;
  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto generator =
      storage.subscribe_delta(kSubscriber, root, OperProtocol::SIMPLE_JSON);
  storage.start();
  // First sync post subscription setup
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);
  auto first = deltaVal.changes()->at(0);
  // Synced entire tree from root
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({})));

  // Make changes, we should see that come in as delta now
  EXPECT_EQ(storage.set(root.tx(), false), std::nullopt);

  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.changes()->size(), 1);
  first = deltaVal.changes()->at(0);
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({"tx"})));

  // Should eventually recv heartbeat
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  EXPECT_EQ(deltaVal.changes()->size(), 0);
}

TEST(SubscribableStorageTests, SubscribeDeltaUpdate) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);
  storage.start();

  const auto& path = root.stringToStruct()["test"].max();
  auto generator =
      storage.subscribe_delta(kSubscriber, path, OperProtocol::SIMPLE_JSON);

  // set value and subscribe
  EXPECT_EQ(storage.set(path, 1), std::nullopt);
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);
  auto first = deltaVal.changes()->at(0);
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({})));
  EXPECT_FALSE(first.oldState());
  EXPECT_TRUE(first.newState());

  // update value
  EXPECT_EQ(storage.set(path, 10), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);
  auto second = deltaVal.changes()->at(0);
  EXPECT_THAT(
      *second.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({})));
  EXPECT_TRUE(second.oldState());
  EXPECT_TRUE(second.newState());
}

TEST(SubscribableStorageTests, SubscribeDeltaAddRemoveParent) {
  // add subscription for a path that doesn't exist yet, then add parent
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto path = root.structMap()[99].min();
  auto generator = storage.subscribe_delta(
      kSubscriber, std::move(path), OperProtocol::SIMPLE_JSON);
  storage.start();

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(root.structMap()[99], newStruct), std::nullopt);
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.changes()->size(), 1);
  EXPECT_TRUE(deltaVal.metadata());
  auto first = deltaVal.changes()->at(0);

  // in this case, the delta has the relative path, which should be empty
  EXPECT_EQ(first.path()->raw()->size(), 0);

  EXPECT_FALSE(first.oldState());

  EXPECT_EQ(folly::to<int>(*first.newState()), 999);

  // now delete the parent and verify we see the deletion delta too
  storage.remove(root.structMap()[99]);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.changes()->size(), 1);
  EXPECT_TRUE(deltaVal.metadata());
  first = deltaVal.changes()->at(0);

  // in this case, the delta has the relative path, which should be empty
  EXPECT_EQ(first.path()->raw()->size(), 0);

  EXPECT_FALSE(first.newState());
  EXPECT_EQ(folly::to<int>(*first.oldState()), 999);
}

TEST(SubscribableStorageTests, SubscribeEncodedPathSimple) {
  using namespace facebook::fboss::fsdb;
  FLAGS_serveHeartbeats = true;
  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  const auto& path = root.stringToStruct()["test"].max();
  auto generator =
      storage.subscribe_encoded(kSubscriber, path, OperProtocol::SIMPLE_JSON);
  storage.start();

  EXPECT_EQ(
      storage.set(root.stringToStruct()["test"].max(), 123), std::nullopt);
  auto deltaState = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  ASSERT_TRUE(deltaState.newVal.has_value());

  auto operState = deltaState.newVal;

  auto deserialized = facebook::fboss::thrift_cow::
      deserialize<apache::thrift::type_class::integral, int>(
          OperProtocol::SIMPLE_JSON, *operState->contents());

  EXPECT_EQ(deserialized, 123);

  // Should eventually recv heartbeat
  deltaState = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  ASSERT_TRUE(deltaState.newVal.has_value());
  EXPECT_EQ(deltaState.newVal->isHeartbeat(), true);
}

TEST(SubscribableStorageTests, SubscribeExtendedPathSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
  using namespace facebook::fboss::fsdb;

  FLAGS_serveHeartbeats = true;
  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto generator = storage.subscribe_encoded_extended(
      kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
  storage.start();

  EXPECT_EQ(storage.set(root.mapOfStringToI32()["test1"], 998), std::nullopt);
  auto taggedVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(taggedVal.size(), 1);
  auto oldVal = taggedVal.at(0).oldVal;
  auto newVal = taggedVal.at(0).newVal;
  ASSERT_TRUE(oldVal);
  ASSERT_TRUE(newVal);

  // Old state should not be null, but should have empty contents in
  // the TaggedOperState object
  ASSERT_FALSE(oldVal->state()->contents());

  EXPECT_THAT(
      *newVal->path()->path(),
      ::testing::ElementsAre("mapOfStringToI32", "test1"));

  auto deserialized = facebook::fboss::thrift_cow::
      deserialize<apache::thrift::type_class::integral, int>(
          OperProtocol::SIMPLE_JSON, *newVal->state()->contents());

  EXPECT_EQ(deserialized, 998);

  // Should eventually recv heartbeat
  taggedVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  EXPECT_EQ(taggedVal.size(), 0);
}

TEST(SubscribableStorageTests, SubscribeExtendedPathMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto generator = storage.subscribe_encoded_extended(
      kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
  storage.start();

  EXPECT_EQ(
      storage.set(root, createTestStructForExtendedTests()), std::nullopt);
  auto streamedVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  std::map<std::vector<std::string>, int> expected = {
      {{"mapOfStringToI32", "test1"}, 1},
      {{"mapOfStringToI32", "test10"}, 10},
      {{"mapOfStringToI32", "test11"}, 11},
      {{"mapOfStringToI32", "test12"}, 12},
      {{"mapOfStringToI32", "test13"}, 13},
      {{"mapOfStringToI32", "test14"}, 14},
      {{"mapOfStringToI32", "test15"}, 15},
      {{"mapOfStringToI32", "test16"}, 16},
      {{"mapOfStringToI32", "test17"}, 17},
      {{"mapOfStringToI32", "test18"}, 18},
      {{"mapOfStringToI32", "test19"}, 19},
  };

  EXPECT_EQ(streamedVal.size(), expected.size());
  for (const auto& deltaVal : streamedVal) {
    auto oldVal = deltaVal.oldVal;
    auto newVal = deltaVal.newVal;
    ASSERT_TRUE(oldVal);
    ASSERT_TRUE(newVal);

    // Old state should not be null, but should have empty contents in
    // the TaggedOperState object
    ASSERT_FALSE(oldVal->state()->contents());

    const auto& elemPath = *newVal->path()->path();
    const auto& contents = *newVal->state()->contents();
    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, contents);
    EXPECT_EQ(expected[elemPath], deserialized)
        << "Mismatch at /" + folly::join('/', elemPath);
  }
}

TEST(SubscribableStorageTests, SubscribeExtendedDeltaSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto generator = storage.subscribe_delta_extended(
      kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
  storage.start();

  EXPECT_EQ(storage.set(root.mapOfStringToI32()["test1"], 998), std::nullopt);
  auto streamedVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(streamedVal.size(), 1);
  EXPECT_EQ(streamedVal.at(0).delta()->changes()->size(), 1);

  EXPECT_THAT(
      *streamedVal.at(0).path()->path(),
      ::testing::ElementsAre("mapOfStringToI32", "test1"));

  auto deltaUnit = streamedVal.at(0).delta()->changes()->at(0);
  ASSERT_FALSE(deltaUnit.oldState());
  ASSERT_TRUE(deltaUnit.newState());

  // deltas are relative to the subscribed root, so we expect this to be
  // empty;
  EXPECT_TRUE(deltaUnit.path()->raw()->empty());

  auto deserialized = facebook::fboss::thrift_cow::
      deserialize<apache::thrift::type_class::integral, int>(
          OperProtocol::SIMPLE_JSON, *deltaUnit.newState());

  EXPECT_EQ(deserialized, 998);
}

class SubscribableStorageTestsPathDelta : public Test,
                                          public WithParamInterface<bool> {};

INSTANTIATE_TEST_SUITE_P(
    SubscribableStorageTests,
    SubscribableStorageTestsPathDelta,
    Bool());

CO_TEST_P(SubscribableStorageTestsPathDelta, UnregisterSubscriber) {
  using namespace facebook::fboss::fsdb;

  const bool isPath = GetParam();

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);
  storage.start();

  const auto path =
      ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  EXPECT_EQ(storage.set(root.mapOfStringToI32()["test1"], 1), std::nullopt);

  auto subscribeOneAndUnregister = [&](bool isPath) -> folly::coro::Task<void> {
    if (isPath) {
      auto generator = storage.subscribe_encoded_extended(
          kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
      auto ret = co_await co_awaitTry(
          folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
      EXPECT_FALSE(ret.hasException());
    } else {
      auto generator = storage.subscribe_delta_extended(
          kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
      auto ret = co_await co_awaitTry(
          folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
      EXPECT_FALSE(ret.hasException());
    }
  };

  // register sub1, get a update and unregister it
  co_await subscribeOneAndUnregister(isPath);

  // update cow-thrift value
  EXPECT_EQ(storage.set(root.mapOfStringToI32()["test1"], 2), std::nullopt);

  // register a new sub2 and wait for next serveSubscription cycle.
  // previous stale fully-resolved-subscription states from sub1 should be
  // cleaned up
  co_await subscribeOneAndUnregister(isPath);
}

CO_TEST_P(SubscribableStorageTestsPathDelta, UnregisterSubscriberMulti) {
  using namespace facebook::fboss::fsdb;

  const bool isPath = GetParam();

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);
  storage.start();

  const auto path =
      ext_path_builder::raw("mapOfStringToI32").regex("test.*").get();
  EXPECT_EQ(storage.set(root.mapOfStringToI32()["test1"], 1), std::nullopt);

  auto subscribeOneAndUnregister = [&](bool isPath) -> folly::coro::Task<void> {
    if (isPath) {
      auto generator = storage.subscribe_encoded_extended(
          kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
      co_await folly::coro::sleep(
          std::chrono::milliseconds(folly::Random::rand32(0, 5000)));
    } else {
      auto generator = storage.subscribe_delta_extended(
          kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
      co_await folly::coro::sleep(
          std::chrono::milliseconds(folly::Random::rand32(0, 5000)));
    }
  };

  auto subscribeManyAndUnregister = [&](bool isPath,
                                        int count) -> folly::coro::Task<void> {
    std::vector<folly::coro::Task<void>> tasks;
    for (int i = 0; i < count; ++i) {
      tasks.emplace_back(subscribeOneAndUnregister(isPath));
    }
    co_await folly::coro::collectAllRange(std::move(tasks));
  };

  folly::coro::AsyncScope backgroundScope;
  backgroundScope.add(subscribeManyAndUnregister(isPath, 50)
                          .scheduleOn(folly::getGlobalCPUExecutor()));

  for (int j = 0; j < 5; ++j) {
    for (int i = 0; i < 10; ++i) {
      // update cow-thrift value
      EXPECT_EQ(
          storage.set(root.mapOfStringToI32()[fmt::format("test{}", i)], j),
          std::nullopt);
    }
    co_await folly::coro::sleep(std::chrono::seconds(1));
  }

  co_await backgroundScope.joinAsync();
  co_return;
}

CO_TEST(SubscribableStorageTests, SubscribeExtendedDeltaUpdate) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);
  storage.start();

  const auto& path =
      ext_path_builder::raw("stringToStruct").regex("test1.*").raw("max").get();
  auto generator = storage.subscribe_delta_extended(
      kSubscriber, {path}, OperProtocol::SIMPLE_JSON);

  const auto& setPath = root.stringToStruct()["test1"].max();
  EXPECT_EQ(storage.set(setPath, 1), std::nullopt);
  auto ret = co_await co_awaitTry(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_FALSE(ret.hasException());

  // update value
  EXPECT_EQ(storage.set(setPath, 10), std::nullopt);
  ret = co_await co_awaitTry(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_FALSE(ret.hasException());
}

TEST(SubscribableStorageTests, SubscribeExtendedDeltaMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto generator = storage.subscribe_delta_extended(
      kSubscriber, {path}, OperProtocol::SIMPLE_JSON);
  storage.start();

  EXPECT_EQ(
      storage.set(root, createTestStructForExtendedTests()), std::nullopt);
  auto streamedVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  std::map<std::vector<std::string>, int> expected = {
      {{"mapOfStringToI32", "test1"}, 1},
      {{"mapOfStringToI32", "test10"}, 10},
      {{"mapOfStringToI32", "test11"}, 11},
      {{"mapOfStringToI32", "test12"}, 12},
      {{"mapOfStringToI32", "test13"}, 13},
      {{"mapOfStringToI32", "test14"}, 14},
      {{"mapOfStringToI32", "test15"}, 15},
      {{"mapOfStringToI32", "test16"}, 16},
      {{"mapOfStringToI32", "test17"}, 17},
      {{"mapOfStringToI32", "test18"}, 18},
      {{"mapOfStringToI32", "test19"}, 19},
  };

  EXPECT_EQ(streamedVal.size(), expected.size());
  for (const auto& taggedDelta : streamedVal) {
    auto rawPath = *taggedDelta.path()->path();
    ASSERT_TRUE(expected.find(rawPath) != expected.end());

    auto expectedValue = expected[rawPath];
    ASSERT_EQ(taggedDelta.delta()->changes()->size(), 1);
    auto deltaUnit = taggedDelta.delta()->changes()->at(0);
    ASSERT_FALSE(deltaUnit.oldState());
    ASSERT_TRUE(deltaUnit.newState());

    // deltas are relative to the subscribed root, so we expect this to be
    // empty;
    EXPECT_TRUE(deltaUnit.path()->raw()->empty());

    auto deserialized = facebook::fboss::thrift_cow::
        deserialize<apache::thrift::type_class::integral, int>(
            OperProtocol::SIMPLE_JSON, *deltaUnit.newState());

    EXPECT_EQ(deserialized, expectedValue);
  }
}

TEST(SubscribableStorageTests, SetPatchWithPathSpec) {
  // test set/patch on different path spec
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;

  // verify set API works as expected
  EXPECT_EQ(storage.set(root.structMap()[99], newStruct), std::nullopt);

  auto struct99 = storage.get(root.structMap()[99]);
  EXPECT_EQ(*struct99->min(), 999);
  EXPECT_EQ(*struct99->max(), 1001);

  // test invalid patching logic
  {
    // create oper delta
    OperDelta delta;
    delta.protocol() = OperProtocol::BINARY;

    // bring unit
    OperDeltaUnit unit;
    // intentionally set path as root aka empty path->raw list
    unit.path()->raw() = {};
    unit.newState() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(OperProtocol::BINARY, newStruct);

    // add unit to delta
    delta.changes()->push_back(std::move(unit));

    // patch which suppose to fail, instead, it wipes out root struct
    // oper delta has the type(TestStructSimple) of root->structMap->entry
    // the patch is applied on root, which has TestStruct type.
    // Due to the generous nature of thrift deserialization,
    // when it deserializes the TestStructSimple binary, none of the fields
    // match TestStruct, it returns an empty TestStruct instead of error.
    // Therefore the end result is basically patching an empty but valid
    // root struct, and wipes out any existing field with default value.
    EXPECT_EQ(storage.patch(delta), std::nullopt);

    // confirm the new value didn't get in
    // instead it wiped entire content previously set
    auto updatedMap = storage.get(root.structMap());
    EXPECT_EQ(updatedMap->size(), 0);
  }

  // test correct patching logic
  {
    // confirm the structMap is empty at beginning
    auto currentMap = storage.get(root.structMap());
    EXPECT_EQ(currentMap->size(), 0);

    // update min,max values
    newStruct.min() = 99;
    newStruct.max() = 101;

    // create oper delta
    OperDelta delta;
    delta.protocol() = OperProtocol::BINARY;

    // bring unit
    OperDeltaUnit unit;
    auto correctPath = root.structMap()[99].tokens();
    unit.path()->raw() = std::move(correctPath);
    unit.newState() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(OperProtocol::BINARY, newStruct);

    // add unit to delta
    delta.changes()->push_back(std::move(unit));

    // patch which suppose to succeed
    // oper delta has the type of root->structMap->struct{min, max}
    // the patch is applied on root->structMap[99], which has the same type
    EXPECT_EQ(storage.patch(delta), std::nullopt);

    // confirm the new value has been stamped to root->structMap[99]
    auto updatedMapEntry = storage.get(root.structMap()[99]);
    EXPECT_EQ(*updatedMapEntry->min(), *newStruct.min());
    EXPECT_EQ(*updatedMapEntry->max(), *newStruct.max());
  }
}

// Similar test to SetPatchWithPathSpec except we are testing patching
// TaggedOperState
TEST(SubscribableStorageTests, SetPatchWithPathSpecOnTaggedState) {
  // test set/patch on different path spec
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;

  // verify set API works as expected
  EXPECT_EQ(storage.set(root.structMap()[99], newStruct), std::nullopt);

  auto struct99 = storage.get(root.structMap()[99]);
  EXPECT_EQ(*struct99->min(), 999);
  EXPECT_EQ(*struct99->max(), 1001);

  // test invalid patching logic
  {
    // create tagged oper delta
    TaggedOperState operState;

    operState.path()->path() = {};
    operState.state()->protocol() = OperProtocol::BINARY;
    operState.state()->contents() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(OperProtocol::BINARY, newStruct);

    // patch which suppose to fail, instead, it wipes out root struct
    // oper delta has the type(TestStructSimple) of root->structMap->entry
    // the patch is applied on root, which has TestStruct type.
    // Due to the generous nature of thrift deserialization,
    // when it deserializes the TestStructSimple binary, none of the fields
    // match TestStruct, it returns an empty TestStruct instead of error.
    // Therefore the end result is basically patching an empty but valid
    // root struct, and wipes out any existing field with default value.
    EXPECT_EQ(storage.patch(operState), std::nullopt);

    // confirm the new value didn't get in
    // instead it wiped entire content previously set
    auto updatedMap = storage.get(root.structMap());
    EXPECT_EQ(updatedMap->size(), 0);
  }

  // test correct patching logic
  {
    // confirm the structMap is empty at beginning
    auto currentMap = storage.get(root.structMap());
    EXPECT_EQ(currentMap->size(), 0);

    // update min,max values
    newStruct.min() = 99;
    newStruct.max() = 101;

    // create tagged oper delta
    TaggedOperState operState;
    operState.state()->protocol() = OperProtocol::BINARY;

    auto correctPath = root.structMap()[99].tokens();
    operState.path()->path() = std::move(correctPath);
    operState.state()->contents() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(OperProtocol::BINARY, newStruct);

    // patch which suppose to succeed
    // tagged oper delta has the type of root->structMap->struct{min, max}
    // the patch is applied on root->structMap[99], which has the same type
    EXPECT_EQ(storage.patch(operState), std::nullopt);

    // confirm the new value has been stamped to root->structMap[99]
    auto updatedMapEntry = storage.get(root.structMap()[99]);
    EXPECT_EQ(*updatedMapEntry->min(), *newStruct.min());
    EXPECT_EQ(*updatedMapEntry->max(), *newStruct.max());
  }
}

TEST(SubscribableStorageTests, PruneSubscriptionPathStores) {
  // add and remove paths, and verify PathStores are pruned
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(root.structMap()[3], newStruct), std::nullopt);
  storage.start();

  // create subscriber
  const auto& path = root.stringToStruct()["test"].max();
  auto generator =
      storage.subscribe_delta(kSubscriber, path, OperProtocol::SIMPLE_JSON);

  // add path and wait for it to be served (publishAndAddPaths)
  EXPECT_EQ(storage.set(path, 1), std::nullopt);
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);

  auto initialNumPathStores = storage.numPathStores();
  XLOG(DBG2) << "initialNumPathStores: " << initialNumPathStores;

  // add another path and check PathStores count
  EXPECT_EQ(storage.set(root.structMap()[99], newStruct), std::nullopt);
  EXPECT_EQ(storage.set(path, 2), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);

  auto maxNumPathStores = storage.numPathStores();
  XLOG(DBG2) << "maxNumPathStores: " << maxNumPathStores;
  EXPECT_GT(maxNumPathStores, initialNumPathStores);

  // now delete the added path
  storage.remove(root.structMap()[99]);
  EXPECT_EQ(storage.set(path, 3), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);

  // after path deletion, numPathStores should drop
  auto finalNumPathStores = storage.numPathStores();
  XLOG(DBG2) << "finalNumPathStores : " << finalNumPathStores;
  EXPECT_LT(finalNumPathStores, maxNumPathStores);
  EXPECT_EQ(finalNumPathStores, initialNumPathStores);
}

TEST(SubscribableStorageTests, ApplyPatch) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;
  thriftpath::RootThriftPath<TestStruct> root;
  auto testDyn = createTestDynamic();
  auto testStruct = facebook::thrift::from_dynamic<TestStruct>(
      testDyn, facebook::thrift::dynamic_format::JSON_1);
  auto storage = TestSubscribableStorage(testStruct);

  TestStructSimple curStruct = *testStruct.member();
  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;

  // make some nodes so we can use PatchBuilder
  auto nodeA = std::make_shared<ThriftStructNode<TestStructSimple>>(curStruct);
  auto nodeB = std::make_shared<ThriftStructNode<TestStructSimple>>(
      std::move(newStruct));

  using TestStructMembers = apache::thrift::reflect_struct<TestStruct>::member;
  std::vector<std::string> path = {
      folly::to<std::string>(TestStructMembers::member::id::value)};
  auto patch = PatchBuilder::build(nodeA, nodeB, std::move(path));

  auto memberStruct = storage.get(root.member());
  EXPECT_EQ(*memberStruct->min(), 10);
  EXPECT_EQ(*memberStruct->max(), 20);

  storage.patch(std::move(patch));

  memberStruct = storage.get(root.member());
  EXPECT_EQ(*memberStruct->min(), 999);
  EXPECT_EQ(*memberStruct->max(), 1001);
}
