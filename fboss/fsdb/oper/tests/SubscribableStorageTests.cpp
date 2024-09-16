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
#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/coro/Task.h>
#include <folly/coro/Timeout.h>
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
using TestStructMembers = apache::thrift::reflect_struct<TestStruct>::member;

dynamic createTestDynamic() {
  return dynamic::object("tx", true)("rx", false)("name", "testname")(
      "optionalString", "bla")("enumeration", 1)("enumMap", dynamic::object)(
      "member", dynamic::object("min", 10)("max", 20))(
      "variantMember", dynamic::object("integral", 99))(
      "structMap", dynamic::object(3, dynamic::object("min", 100)("max", 200)))(
      "structList", dynamic::array())("enumSet", dynamic::array())(
      "integralSet", dynamic::array())("mapOfStringToI32", dynamic::object())(
      "listOfPrimitives", dynamic::array())("setOfI32", dynamic::array());
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

class SubscribableStorageTests : public Test, public WithParamInterface<bool> {
 public:
  void SetUp() override {
    FLAGS_lazyPathStoreCreation = GetParam();
    auto testDyn = createTestDynamic();
    testStruct = facebook::thrift::from_dynamic<TestStruct>(
        testDyn, facebook::thrift::dynamic_format::JSON_1);
  }

 protected:
  thriftpath::RootThriftPath<TestStruct> root;
  TestStruct testStruct;
};

INSTANTIATE_TEST_SUITE_P(
    SubscribableStorageTest,
    SubscribableStorageTests,
    Bool());

TEST_P(SubscribableStorageTests, GetThrift) {
  auto storage = TestSubscribableStorage(testStruct);
  EXPECT_EQ(storage.get(root.tx()).value(), true);
  EXPECT_EQ(storage.get(root.rx()).value(), false);
  EXPECT_EQ(storage.get(root.member()).value(), testStruct.member().value());
  EXPECT_EQ(
      storage.get(root.structMap()[3]).value(), testStruct.structMap()->at(3));
  EXPECT_EQ(storage.get(root).value(), testStruct);
}

TEST_P(SubscribableStorageTests, SubscribeUnsubscribe) {
  auto storage = TestSubscribableStorage(testStruct);
  auto txPath = root.tx();
  storage.start();
  {
    auto generator = storage.subscribe(kSubscriber, std::move(txPath));
    WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), 1));
  }
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), 0));
}

TEST_P(SubscribableStorageTests, SubscribeOne) {
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

TEST_P(SubscribableStorageTests, SubscribePathAddRemoveParent) {
  // add subscription for a path that doesn't exist yet, then add parent
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

TEST_P(SubscribableStorageTests, SubscribeDelta) {
  FLAGS_serveHeartbeats = true;
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

TEST_P(SubscribableStorageTests, SubscribePatch) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;

  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);

  auto generator = storage.subscribe_patch(kSubscriber, root);
  storage.start();

  // Initial sync post subscription setup
  auto msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  auto patchGroups = *msg.get_chunk().patchGroups();
  EXPECT_EQ(patchGroups.size(), 1);
  auto patches = patchGroups.begin()->second;
  EXPECT_EQ(patches.size(), 1);
  auto patch = patches.front();
  auto rootPatch = patch.patch()->val_ref();
  EXPECT_TRUE(rootPatch);
  //   initial sync should just be a whole blob
  auto deserialized = facebook::fboss::thrift_cow::
      deserializeBuf<apache::thrift::type_class::structure, TestStruct>(
          *patch.protocol(), std::move(*rootPatch));
  EXPECT_EQ(deserialized, testStruct);

  // Make changes, we should see that come in as a patch now
  EXPECT_EQ(storage.set(root.tx(), false), std::nullopt);
  msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  patchGroups = *msg.get_chunk().patchGroups();
  EXPECT_EQ(patchGroups.size(), 1);
  patches = patchGroups.begin()->second;
  EXPECT_EQ(patches.size(), 1);
  patch = patches.front();
  using TestStructMembers = apache::thrift::reflect_struct<TestStruct>::member;
  auto newVal = patch.patch()
                    ->struct_node_ref()
                    ->children()
                    ->at(TestStructMembers::tx::id::value)
                    .val_ref();
  auto deserializedVal = facebook::fboss::thrift_cow::
      deserializeBuf<apache::thrift::type_class::integral, bool>(
          OperProtocol::COMPACT, std::move(*newVal));
  EXPECT_FALSE(deserializedVal);
}

TEST_P(SubscribableStorageTests, SubscribePatchUpdate) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;

  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  const auto& path = root.stringToStruct()["test"].max();
  auto generator = storage.subscribe_patch(kSubscriber, path);

  // set and check
  EXPECT_EQ(storage.set(path, 1), std::nullopt);
  auto msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  auto patchGroups = *msg.get_chunk().patchGroups();
  EXPECT_EQ(patchGroups.size(), 1);
  auto patches = patchGroups.begin()->second;
  EXPECT_EQ(patches.size(), 1);
  auto patch = patches.front();
  auto newVal = *patch.patch()->val_ref();
  auto deserializedVal = facebook::fboss::thrift_cow::
      deserializeBuf<apache::thrift::type_class::integral, int>(
          *patch.protocol(), std::move(newVal));
  EXPECT_EQ(deserializedVal, 1);

  // update and check
  EXPECT_EQ(storage.set(path, 10), std::nullopt);
  msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  patchGroups = *msg.get_chunk().patchGroups();
  EXPECT_EQ(patchGroups.size(), 1);
  patches = patchGroups.begin()->second;
  EXPECT_EQ(patches.size(), 1);
  patch = patches.front();
  newVal = *patch.patch()->val_ref();
  deserializedVal = facebook::fboss::thrift_cow::
      deserializeBuf<apache::thrift::type_class::integral, int>(
          OperProtocol::COMPACT, std::move(newVal));
  EXPECT_EQ(deserializedVal, 10);
}

TEST_P(SubscribableStorageTests, SubscribePatchMulti) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;

  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  const auto& path1 = root.stringToStruct()["test1"].max();
  const auto& path2 = root.stringToStruct()["test2"].max();
  RawOperPath p1, p2;
  // validate both tokens and idTokens work
  p1.path() = path1.tokens();
  p2.path() = path2.idTokens();
  auto generator = storage.subscribe_patch(
      kSubscriber,
      {
          {1, std::move(p1)},
          {2, std::move(p2)},
      });

  // set and check, should only recv one patch on the path that exists
  EXPECT_EQ(storage.set(path1, 123), std::nullopt);
  auto msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  auto patchGroups = *msg.get_chunk().patchGroups();
  EXPECT_EQ(patchGroups.size(), 1);
  EXPECT_EQ(patchGroups.begin()->first, 1);
  auto patches = patchGroups.begin()->second;
  EXPECT_EQ(patches.size(), 1);
  auto patch = patches.front();
  EXPECT_EQ(patch.basePath()[1], "test1");
  auto rootPatch = patch.patch()->move_val();
  //   initial sync should just be a whole blob
  auto deserialized = facebook::fboss::thrift_cow::
      deserializeBuf<apache::thrift::type_class::integral, int32_t>(
          *patch.protocol(), std::move(rootPatch));
  EXPECT_EQ(deserialized, 123);

  std::map<std::string, TestStructSimple> stringToStruct =
      storage.get(root.stringToStruct()).value();
  // update both structs now, should recv both patches
  stringToStruct["test1"].max() = 100;
  stringToStruct["test2"].max() = 200;
  EXPECT_EQ(storage.set(root.stringToStruct(), stringToStruct), std::nullopt);
  msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  patchGroups = *msg.get_chunk().patchGroups();

  EXPECT_EQ(patchGroups.size(), 2);
  for (auto& [key, patchGroup] : patchGroups) {
    EXPECT_EQ(patchGroup.size(), 1); // only one patch per raw path
    auto& patch = patchGroup.front();
    EXPECT_EQ(patch.basePath()[1], fmt::format("test{}", key));
    auto rootPatch = patch.patch()->move_val();
    deserialized = facebook::fboss::thrift_cow::
        deserializeBuf<apache::thrift::type_class::integral, int32_t>(
            *patch.protocol(), std::move(rootPatch));
    // above we set values such that it's sub key * 100 for easier testing
    EXPECT_EQ(deserialized, key * 100);
  }
}

TEST_P(SubscribableStorageTests, SubscribePatchHeartbeat) {
  FLAGS_serveHeartbeats = true;
  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  auto generator = storage.subscribe_patch(kSubscriber, root);

  auto msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  // first message is initial sync
  EXPECT_EQ(msg.getType(), SubscriberMessage::Type::chunk);
  // Should eventually recv heartbeat
  msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  EXPECT_EQ(msg.getType(), SubscriberMessage::Type::heartbeat);
}

TEST_P(SubscribableStorageTests, SubscribeDeltaUpdate) {
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

TEST_P(SubscribableStorageTests, SubscribeDeltaAddRemoveParent) {
  // add subscription for a path that doesn't exist yet, then add parent
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

TEST_P(SubscribableStorageTests, SubscribeEncodedPathSimple) {
  FLAGS_serveHeartbeats = true;
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

TEST_P(SubscribableStorageTests, SubscribeExtendedPathSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
  FLAGS_serveHeartbeats = true;

  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);

  auto path =
      ext_path_builder::raw(TestStructMembers::mapOfStringToI32::id::value)
          .regex("test1.*")
          .get();
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
      ::testing::ElementsAre(
          folly::to<std::string>(
              TestStructMembers::mapOfStringToI32::id::value),
          "test1"));

  auto deserialized = facebook::fboss::thrift_cow::
      deserialize<apache::thrift::type_class::integral, int>(
          OperProtocol::SIMPLE_JSON, *newVal->state()->contents());

  EXPECT_EQ(deserialized, 998);

  // Should eventually recv heartbeat
  taggedVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  EXPECT_EQ(taggedVal.size(), 0);
}

TEST_P(SubscribableStorageTests, SubscribeExtendedPathMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
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

TEST_P(SubscribableStorageTests, SubscribeExtendedDeltaSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
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

class SubscribableStorageTestsPathDelta
    : public Test,
      public WithParamInterface<std::tuple<bool, bool>> {
 public:
  void SetUp() override {
    const std::tuple<bool, bool> params = GetParam();
    isPath = get<0>(params);
    FLAGS_lazyPathStoreCreation = get<1>(params);
    auto testDyn = createTestDynamic();
    testStruct = facebook::thrift::from_dynamic<TestStruct>(
        testDyn, facebook::thrift::dynamic_format::JSON_1);
  }

 protected:
  thriftpath::RootThriftPath<TestStruct> root;
  TestStruct testStruct;
  bool isPath;
};

INSTANTIATE_TEST_SUITE_P(
    SubscribableStorageTests,
    SubscribableStorageTestsPathDelta,
    Combine(Bool(), Bool()));

CO_TEST_P(SubscribableStorageTestsPathDelta, UnregisterSubscriber) {
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

CO_TEST_P(SubscribableStorageTests, SubscribeExtendedDeltaUpdate) {
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

TEST_P(SubscribableStorageTests, SubscribeExtendedDeltaMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
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

TEST_P(SubscribableStorageTests, SetPatchWithPathSpec) {
  // test set/patch on different path spec
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
TEST_P(SubscribableStorageTests, SetPatchWithPathSpecOnTaggedState) {
  // test set/patch on different path spec
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

TEST_P(SubscribableStorageTests, PruneSubscriptionPathStores) {
  // add and remove paths, and verify PathStores are pruned
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
  if (FLAGS_lazyPathStoreCreation) {
    // with lazy PathStore creation, numPathStores should not change
    // for exact subscriptions when adding/deleting paths.
    EXPECT_EQ(maxNumPathStores, initialNumPathStores);
  } else {
    EXPECT_GT(maxNumPathStores, initialNumPathStores);
  }

  // now delete the added path
  storage.remove(root.structMap()[99]);
  EXPECT_EQ(storage.set(path, 3), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);

  // after path deletion, numPathStores should drop
  auto finalNumPathStores = storage.numPathStores();
  XLOG(DBG2) << "finalNumPathStores : " << finalNumPathStores;
  if (FLAGS_lazyPathStoreCreation) {
    EXPECT_EQ(finalNumPathStores, maxNumPathStores);
  } else {
    EXPECT_LT(finalNumPathStores, maxNumPathStores);
  }
  EXPECT_EQ(finalNumPathStores, initialNumPathStores);
}

TEST_P(SubscribableStorageTests, ApplyPatch) {
  using namespace facebook::fboss::thrift_cow;
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

TEST_P(SubscribableStorageTests, PatchInvalidDeltaPath) {
  auto storage = TestSubscribableStorage(testStruct);
  TestStructSimple newStruct;

  OperDelta delta;
  OperDeltaUnit unit;
  unit.path()->raw() = {"invalid", "path"};
  unit.newState() = facebook::fboss::thrift_cow::serialize<
      apache::thrift::type_class::structure>(OperProtocol::BINARY, newStruct);

  // should fail gracefully
  delta.changes() = {unit};
  EXPECT_EQ(storage.patch(delta), StorageError::INVALID_PATH);

  // partially valid path should still fail
  unit.path()->raw() = {"inlineStruct", "invalid", "path"};
  delta.changes() = {unit};
}

CO_TEST_P(SubscribableStorageTests, SubscribeExtendedPatchSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  int key = 0;
  auto generator = storage.subscribe_patch_extended(kSubscriber, {{key, path}});
  storage.start();

  EXPECT_EQ(storage.set(root.mapOfStringToI32()["test1"], 998), std::nullopt);
  auto msg = co_await folly::coro::timeout(
      consumeOne(generator), std::chrono::seconds(5));
  auto chunk = msg.get_chunk();

  EXPECT_EQ(chunk.patchGroups()->size(), 1); // single path -> single patchGroup
  EXPECT_EQ(chunk.patchGroups()->at(key).size(), 1);
  EXPECT_THAT(
      *chunk.patchGroups()->at(key).front().basePath(),
      ::testing::ElementsAre(
          "13", "test1")); // 13 is the id of mapOfStringToI32

  auto testStorage = CowStorage<TestStruct>(testStruct);
  EXPECT_EQ(
      testStorage.patch(std::move(chunk.patchGroups()->at(key).front())),
      std::nullopt);
  EXPECT_EQ(testStorage.root()->toThrift().mapOfStringToI32()["test1"], 998);
}

CO_TEST_P(SubscribableStorageTests, SubscribeExtendedPatchUpdate) {
  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  const auto& path =
      ext_path_builder::raw("stringToStruct").regex("test1.*").raw("max").get();
  auto generator = storage.subscribe_patch_extended(kSubscriber, {{0, path}});

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

CO_TEST_P(SubscribableStorageTests, SubscribeExtendedPatchMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = TestSubscribableStorage(testStruct);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  int key = 0;
  auto generator = storage.subscribe_patch_extended(kSubscriber, {{key, path}});
  storage.start();

  EXPECT_EQ(
      storage.set(root, createTestStructForExtendedTests()), std::nullopt);
  auto msg = co_await folly::coro::timeout(
      consumeOne(generator), std::chrono::seconds(5));
  auto chunk = msg.get_chunk();

  EXPECT_EQ(chunk.patchGroups()->size(), 1); // single path -> single patchGroup
  EXPECT_EQ(
      chunk.patchGroups()->at(key).size(),
      11); // 11 changes from createTestStructForExtendedTests

  std::map<std::string, int> expected = {
      {"test1", 1},
      {"test10", 10},
      {"test11", 11},
      {"test12", 12},
      {"test13", 13},
      {"test14", 14},
      {"test15", 15},
      {"test16", 16},
      {"test17", 17},
      {"test18", 18},
      {"test19", 19},
  };

  auto testStorage = CowStorage<TestStruct>(testStruct);

  for (auto& patch : chunk.patchGroups()->at(key)) {
    // this will be testXY
    auto lastPathElem = patch.basePath()->at(patch.basePath()->size() - 1);
    EXPECT_EQ(testStorage.patch(std::move(patch)), std::nullopt);
    EXPECT_EQ(
        testStorage.root()->toThrift().mapOfStringToI32()->at(lastPathElem),
        expected.at(lastPathElem));
  }
}
