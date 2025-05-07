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
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/hybrid_storage_test.h" // @manual=//fboss/fsdb/tests:hybrid_storage_test-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/hybrid_storage_test_types.h"

using folly::dynamic;
using namespace testing;

namespace {

using namespace facebook::fboss::fsdb;

// TODO: templatize test cases
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

constexpr auto kSubscriber = "testSubscriber";

template <typename Gen>
folly::coro::Task<typename Gen::value_type> consumeOne(Gen& generator) {
  auto item = co_await generator.next();
  auto&& value = *item;
  co_return std::move(value);
}

} // namespace

template <bool EnableHybridStorage, bool LazyPathStoreCreation>
struct TestParams {
  static constexpr auto hybridStorage = EnableHybridStorage;
  static constexpr auto lazyPathStoreCreation = LazyPathStoreCreation;
};

using SubscribableStorageTestTypes = ::testing::Types<
    TestParams<false, false>,
    TestParams<false, true>,
    TestParams<true, true>>;

template <typename TestParams>
class SubscribableStorageTests : public Test {
 public:
  void SetUp() override {
    FLAGS_lazyPathStoreCreation = TestParams::lazyPathStoreCreation;
    auto testDyn = createTestDynamic();
    testStruct = facebook::thrift::from_dynamic<TestStruct>(
        testDyn, facebook::thrift::dynamic_format::JSON_1);
  }

  auto initStorage(auto val) {
    auto constexpr isHybridStorage = TestParams::hybridStorage;
    using RootType = std::remove_cvref_t<decltype(val)>;
    return NaivePeriodicSubscribableCowStorage<RootType, isHybridStorage>(val);
  }

  auto createCowStorage(auto val) {
    auto constexpr isHybridStorage = TestParams::hybridStorage;
    using RootType = std::remove_cvref_t<decltype(val)>;
    return CowStorage<
        RootType,
        facebook::fboss::thrift_cow::ThriftStructNode<
            RootType,
            facebook::fboss::thrift_cow::
                ThriftStructResolver<RootType, isHybridStorage>,
            isHybridStorage>>(val);
  }

 protected:
  thriftpath::RootThriftPath<TestStruct> root;
  TestStruct testStruct;
};

TYPED_TEST_SUITE(SubscribableStorageTests, SubscribableStorageTestTypes);

TYPED_TEST(SubscribableStorageTests, GetThrift) {
  auto storage = this->initStorage(this->testStruct);
  EXPECT_EQ(storage.get(this->root.tx()).value(), true);
  EXPECT_EQ(storage.get(this->root.rx()).value(), false);
  EXPECT_EQ(
      storage.get(this->root.member()).value(),
      this->testStruct.member().value());
  EXPECT_EQ(
      storage.get(this->root.structMap()[3]).value(),
      this->testStruct.structMap()->at(3));
  EXPECT_EQ(storage.get(this->root).value(), this->testStruct);
}

TYPED_TEST(SubscribableStorageTests, SubscribeUnsubscribe) {
  auto storage = this->initStorage(this->testStruct);
  auto txPath = this->root.tx();
  storage.start();
  {
    auto generator = storage.subscribe(
        std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
        std::move(txPath));
    WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), 1));
  }
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), 0));
}

TYPED_TEST(SubscribableStorageTests, SubscribeOne) {
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);

  auto txPath = this->root.tx();
  storage.start();
  auto generator = storage.subscribe(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      std::move(txPath));
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.newVal, true);
  EXPECT_EQ(deltaVal.oldVal, std::nullopt);
  EXPECT_EQ(storage.set(this->root.tx(), false), std::nullopt);

  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.oldVal, true);
  EXPECT_EQ(deltaVal.newVal, false);
}

TYPED_TEST(SubscribableStorageTests, SubscribePathAddRemoveParent) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = this->initStorage(this->testStruct);
  auto path = this->root.structMap()[99].min();
  auto generator = storage.subscribe(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      std::move(path));
  storage.start();

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.oldVal, std::nullopt);
  EXPECT_EQ(deltaVal.newVal, 999);

  // now delete the parent and verify we see the deletion delta too
  storage.remove(this->root.structMap()[99]);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.oldVal, 999);
  EXPECT_EQ(deltaVal.newVal, std::nullopt);
}

TYPED_TEST(SubscribableStorageTests, SubscribeDelta) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);

  auto generator = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      OperProtocol::SIMPLE_JSON);
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
  EXPECT_EQ(storage.set(this->root.tx(), false), std::nullopt);

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

TYPED_TEST(SubscribableStorageTests, SubscribePatch) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;

  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);

  auto generator = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), this->root);
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
  EXPECT_EQ(deserialized, this->testStruct);

  // Make changes, we should see that come in as a patch now
  EXPECT_EQ(storage.set(this->root.tx(), false), std::nullopt);
  msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  patchGroups = *msg.get_chunk().patchGroups();
  EXPECT_EQ(patchGroups.size(), 1);
  patches = patchGroups.begin()->second;
  EXPECT_EQ(patches.size(), 1);
  patch = patches.front();
  using ReflectedMembers = apache::thrift::reflect_struct<TestStruct>::member;
  auto newVal = patch.patch()
                    ->struct_node_ref()
                    ->children()
                    ->at(ReflectedMembers::tx::id::value)
                    .val_ref();
  auto deserializedVal = facebook::fboss::thrift_cow::
      deserializeBuf<apache::thrift::type_class::integral, bool>(
          OperProtocol::COMPACT, std::move(*newVal));
  EXPECT_FALSE(deserializedVal);
}

TYPED_TEST(SubscribableStorageTests, SubscribePatchUpdate) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;

  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  const auto& path = this->root.stringToStruct()["test"].max();
  auto generator = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), path);

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

TYPED_TEST(SubscribableStorageTests, SubscribePatchMulti) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;

  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  const auto& path1 = this->root.stringToStruct()["test1"].max();
  const auto& path2 = this->root.stringToStruct()["test2"].max();
  RawOperPath p1, p2;
  // validate both tokens and idTokens work
  p1.path() = path1.tokens();
  p2.path() = path2.idTokens();
  auto generator = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
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
      storage.get(this->root.stringToStruct()).value();
  // update both structs now, should recv both patches
  stringToStruct["test1"].max() = 100;
  stringToStruct["test2"].max() = 200;
  EXPECT_EQ(
      storage.set(this->root.stringToStruct(), stringToStruct), std::nullopt);
  msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  patchGroups = *msg.get_chunk().patchGroups();

  EXPECT_EQ(patchGroups.size(), 2);
  for (auto& [key, patchGroup] : patchGroups) {
    EXPECT_EQ(patchGroup.size(), 1); // only one patch per raw path
    auto& firstPatch = patchGroup.front();
    EXPECT_EQ(firstPatch.basePath()[1], fmt::format("test{}", key));
    auto rootPatchForGroup = firstPatch.patch()->move_val();
    deserialized = facebook::fboss::thrift_cow::
        deserializeBuf<apache::thrift::type_class::integral, int32_t>(
            *firstPatch.protocol(), std::move(rootPatchForGroup));
    // above we set values such that it's sub key * 100 for easier testing
    EXPECT_EQ(deserialized, key * 100);
  }
}

TYPED_TEST(SubscribableStorageTests, SubscribePatchHeartbeat) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  auto generator = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), this->root);

  auto msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  // first message is initial sync
  EXPECT_EQ(msg.getType(), SubscriberMessage::Type::chunk);
  // Should eventually recv heartbeat
  msg = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  EXPECT_EQ(msg.getType(), SubscriberMessage::Type::heartbeat);
}

TYPED_TEST(SubscribableStorageTests, SubscribeHeartbeatConfigured) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  SubscriptionStorageParams params1(std::chrono::seconds(2));
  SubscriptionStorageParams params2(std::chrono::seconds(10));

  auto generator1 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      params1);
  auto generator2 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      params2);

  auto msg1 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(20)));
  // first message is initial sync
  EXPECT_EQ(msg1.getType(), SubscriberMessage::Type::chunk);
  // Should recv heartbeat every 2 seconds. Allow leeway incase previous
  // heartbeat was just sent
  msg1 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(3)));
  EXPECT_EQ(msg1.getType(), SubscriberMessage::Type::heartbeat);

  // First sync post subscription setup
  auto msg2 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator2), std::chrono::seconds(5)));

  // Heartbeat configured for 10 seconds, so we should not see one within 8
  try {
    msg2 = folly::coro::blockingWait(
        folly::coro::timeout(consumeOne(generator2), std::chrono::seconds(3)));
  } catch (const std::exception& ex) {
    EXPECT_EQ(typeid(ex), typeid(folly::FutureTimeout));
  }
}

TYPED_TEST(SubscribableStorageTests, SubscribeHeartbeatNotReceived) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  SubscriptionStorageParams params(std::chrono::seconds(30));

  auto generator1 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      params);
  // Keep default subscription hearbeat interval of 5 seconds
  auto generator2 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), this->root);

  auto msg1 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(20)));
  // first message is initial sync
  EXPECT_EQ(msg1.getType(), SubscriberMessage::Type::chunk);
  // Should not receive any heartbeat
  try {
    msg1 = folly::coro::blockingWait(
        folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(20)));
  } catch (const std::exception& ex) {
    EXPECT_EQ(typeid(ex), typeid(folly::FutureTimeout));
  }
  // First sync post subscription setup
  auto msg2 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator2), std::chrono::seconds(5)));

  // Should recv heartbeat every 5 seconds(default interval). Allow leeway
  // incase previous heartbeat was just sent
  msg2 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator2), std::chrono::seconds(6)));
  EXPECT_EQ(msg2.getType(), SubscriberMessage::Type::heartbeat);
}

TYPED_TEST(SubscribableStorageTests, SubscribeDeltaUpdate) {
  auto storage = this->initStorage(this->testStruct);
  storage.start();

  const auto& path = this->root.stringToStruct()["test"].max();
  auto generator = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      path,
      OperProtocol::SIMPLE_JSON);

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

TYPED_TEST(SubscribableStorageTests, SubscribeDeltaAddRemoveParent) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = this->initStorage(this->testStruct);
  auto path = this->root.structMap()[99].min();
  auto generator = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      std::move(path),
      OperProtocol::SIMPLE_JSON);
  storage.start();

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);
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
  storage.remove(this->root.structMap()[99]);
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

TYPED_TEST(SubscribableStorageTests, SubscribeEncodedPathSimple) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);

  const auto& path = this->root.stringToStruct()["test"].max();
  auto generator = storage.subscribe_encoded(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      path,
      OperProtocol::SIMPLE_JSON);
  storage.start();

  EXPECT_EQ(
      storage.set(this->root.stringToStruct()["test"].max(), 123),
      std::nullopt);
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

CO_TYPED_TEST(SubscribableStorageTests, SubscribeExtendedDeltaUpdate) {
  auto storage = this->initStorage(this->testStruct);
  storage.start();

  const auto& path =
      ext_path_builder::raw("stringToStruct").regex("test1.*").raw("max").get();
  auto generator = storage.subscribe_delta_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {path},
      OperProtocol::SIMPLE_JSON);

  const auto& setPath = this->root.stringToStruct()["test1"].max();
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

TYPED_TEST(SubscribableStorageTests, SetPatchWithPathSpec) {
  // test set/patch on different path spec
  auto storage = this->initStorage(this->testStruct);
  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;

  // verify set API works as expected
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);
  storage.publishCurrentState();

  auto struct99 = storage.get(this->root.structMap()[99]);
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
    storage.publishCurrentState();

    // confirm the new value didn't get in
    // instead it wiped entire content previously set
    auto updatedMap = storage.get(this->root.structMap());
    EXPECT_EQ(updatedMap->size(), 0);
  }

  // test correct patching logic
  {
    // confirm the structMap is empty at beginning
    auto currentMap = storage.get(this->root.structMap());
    EXPECT_EQ(currentMap->size(), 0);

    // update min,max values
    newStruct.min() = 99;
    newStruct.max() = 101;

    // create oper delta
    OperDelta delta;
    delta.protocol() = OperProtocol::BINARY;

    // bring unit
    OperDeltaUnit unit;
    auto correctPath = this->root.structMap()[99].tokens();
    unit.path()->raw() = std::move(correctPath);
    unit.newState() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(OperProtocol::BINARY, newStruct);

    // add unit to delta
    delta.changes()->push_back(std::move(unit));

    // patch which suppose to succeed
    // oper delta has the type of root->structMap->struct{min, max}
    // the patch is applied on root->structMap[99], which has the same type
    EXPECT_EQ(storage.patch(delta), std::nullopt);
    storage.publishCurrentState();

    // confirm the new value has been stamped to root->structMap[99]
    auto updatedMapEntry = storage.get(this->root.structMap()[99]);
    EXPECT_EQ(*updatedMapEntry->min(), *newStruct.min());
    EXPECT_EQ(*updatedMapEntry->max(), *newStruct.max());
  }
}

// Similar test to SetPatchWithPathSpec except we are testing patching
// TaggedOperState
TYPED_TEST(SubscribableStorageTests, SetPatchWithPathSpecOnTaggedState) {
  // test set/patch on different path spec
  auto storage = this->initStorage(this->testStruct);
  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;

  // verify set API works as expected
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);
  storage.publishCurrentState();

  auto struct99 = storage.get(this->root.structMap()[99]);
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
    storage.publishCurrentState();

    // confirm the new value didn't get in
    // instead it wiped entire content previously set
    auto updatedMap = storage.get(this->root.structMap());
    EXPECT_EQ(updatedMap->size(), 0);
  }

  // test correct patching logic
  {
    // confirm the structMap is empty at beginning
    auto currentMap = storage.get(this->root.structMap());
    EXPECT_EQ(currentMap->size(), 0);

    // update min,max values
    newStruct.min() = 99;
    newStruct.max() = 101;

    // create tagged oper delta
    TaggedOperState operState;
    operState.state()->protocol() = OperProtocol::BINARY;

    auto correctPath = this->root.structMap()[99].tokens();
    operState.path()->path() = std::move(correctPath);
    operState.state()->contents() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(OperProtocol::BINARY, newStruct);

    // patch which suppose to succeed
    // tagged oper delta has the type of root->structMap->struct{min, max}
    // the patch is applied on root->structMap[99], which has the same type
    EXPECT_EQ(storage.patch(operState), std::nullopt);
    storage.publishCurrentState();

    // confirm the new value has been stamped to root->structMap[99]
    auto updatedMapEntry = storage.get(this->root.structMap()[99]);
    EXPECT_EQ(*updatedMapEntry->min(), *newStruct.min());
    EXPECT_EQ(*updatedMapEntry->max(), *newStruct.max());
  }
}

TYPED_TEST(SubscribableStorageTests, PruneSubscriptionPathStores) {
  // add and remove paths, and verify PathStores are pruned
  auto storage = this->initStorage(this->testStruct);
  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(this->root.structMap()[3], newStruct), std::nullopt);
  storage.start();

  // create subscriber
  const auto& path = this->root.stringToStruct()["test"].max();
  auto generator = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      path,
      OperProtocol::SIMPLE_JSON);

  // add path and wait for it to be served (publishAndAddPaths)
  EXPECT_EQ(storage.set(path, 1), std::nullopt);
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);

  auto initialNumPathStores = storage.numPathStoresRecursive_Expensive();
  XLOG(DBG2) << "initialNumPathStores: " << initialNumPathStores;

  // add another path and check PathStores count
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);
  EXPECT_EQ(storage.set(path, 2), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);

  auto maxNumPathStores = storage.numPathStoresRecursive_Expensive();
  XLOG(DBG2) << "maxNumPathStores: " << maxNumPathStores;
  if (FLAGS_lazyPathStoreCreation) {
    // with lazy PathStore creation, numPathStores should not change
    // for exact subscriptions when adding/deleting paths.
    EXPECT_EQ(maxNumPathStores, initialNumPathStores);
  } else {
    EXPECT_GT(maxNumPathStores, initialNumPathStores);
  }

  // now delete the added path
  storage.remove(this->root.structMap()[99]);
  EXPECT_EQ(storage.set(path, 3), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.changes()->size(), 1);

  // after path deletion, numPathStores should drop
  auto finalNumPathStores = storage.numPathStoresRecursive_Expensive();
  XLOG(DBG2) << "finalNumPathStores : " << finalNumPathStores;
  if (FLAGS_lazyPathStoreCreation) {
    EXPECT_EQ(finalNumPathStores, maxNumPathStores);
  } else {
    EXPECT_LT(finalNumPathStores, maxNumPathStores);
  }
  EXPECT_EQ(finalNumPathStores, initialNumPathStores);

  // stronger check: allocation stats and numPathStores must match
  auto numAllocatedPathStore = storage.numPathStores();
  EXPECT_EQ(numAllocatedPathStore, finalNumPathStores);
}

TYPED_TEST(SubscribableStorageTests, ApplyPatch) {
  using namespace facebook::fboss::thrift_cow;
  auto storage = this->initStorage(this->testStruct);

  TestStructSimple curStruct = *this->testStruct.member();
  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;

  // make some nodes so we can use PatchBuilder
  auto nodeA = std::make_shared<ThriftStructNode<TestStructSimple>>(curStruct);
  auto nodeB = std::make_shared<ThriftStructNode<TestStructSimple>>(
      std::move(newStruct));

  using ReflectedMembers = apache::thrift::reflect_struct<TestStruct>::member;
  std::vector<std::string> path = {
      folly::to<std::string>(ReflectedMembers::member::id::value)};
  auto patch = PatchBuilder::build(nodeA, nodeB, path);

  auto memberStruct = storage.get(this->root.member());
  EXPECT_EQ(*memberStruct->min(), 10);
  EXPECT_EQ(*memberStruct->max(), 20);

  auto ret = storage.patch(std::move(patch));
  EXPECT_EQ(ret.has_value(), false);
  storage.publishCurrentState();

  memberStruct = storage.get(this->root.member());
  EXPECT_EQ(*memberStruct->min(), 999);
  EXPECT_EQ(*memberStruct->max(), 1001);
}

TYPED_TEST(SubscribableStorageTests, PatchInvalidDeltaPath) {
  auto storage = this->initStorage(this->testStruct);
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

CO_TYPED_TEST(SubscribableStorageTests, SubscribeExtendedPatchUpdate) {
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  const auto& path =
      ext_path_builder::raw("stringToStruct").regex("test1.*").raw("max").get();
  auto generator = storage.subscribe_patch_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {{0, path}});

  const auto& setPath = this->root.stringToStruct()["test1"].max();
  EXPECT_EQ(storage.set(setPath, 1), std::nullopt);
  auto ret = co_await co_awaitTry(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_FALSE(ret.hasException());

  // update value
  EXPECT_EQ(storage.set(setPath, 10), std::nullopt);
  ret = co_await co_awaitTry(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_FALSE(ret.hasException());

  // remove value
  storage.remove(this->root.stringToStruct()["test1"]);
  ret = co_await co_awaitTry(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_FALSE(ret.hasException());
  SubscriberChunk subChunk = *ret->chunk_ref();
  EXPECT_EQ(subChunk.patchGroups()->size(), 1);
  EXPECT_EQ(subChunk.patchGroups()->at(0).size(), 1);
  auto patch = subChunk.patchGroups()->at(0).front();
  EXPECT_EQ(patch.basePath()->size(), 3);
  EXPECT_EQ(
      patch.patch()->getType(),
      facebook::fboss::thrift_cow::PatchNode::Type::del);
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
  bool isPath{false};
};

INSTANTIATE_TEST_SUITE_P(
    SubscribableStorageSubTypeTests,
    SubscribableStorageTestsPathDelta,
    Combine(Bool(), Bool()));

using TestSubscribableStorage = NaivePeriodicSubscribableCowStorage<TestStruct>;

CO_TEST_P(SubscribableStorageTestsPathDelta, UnregisterSubscriber) {
  auto storage = TestSubscribableStorage(testStruct);
  storage.start();

  const auto path =
      ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  EXPECT_EQ(storage.set(root.mapOfStringToI32()["test1"], 1), std::nullopt);

  auto subId = SubscriptionIdentifier(SubscriberId(kSubscriber));
  auto subscribeOneAndUnregister = [&](bool isPath) -> folly::coro::Task<void> {
    if (isPath) {
      auto generator = storage.subscribe_encoded_extended(
          std::move(subId), {path}, OperProtocol::SIMPLE_JSON);
      auto ret = co_await co_awaitTry(
          folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
      EXPECT_FALSE(ret.hasException());
    } else {
      auto generator = storage.subscribe_delta_extended(
          std::move(subId), {path}, OperProtocol::SIMPLE_JSON);
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

  auto subId = SubscriptionIdentifier(SubscriberId(kSubscriber));
  auto subscribeOneAndUnregister = [&](bool isPath) -> folly::coro::Task<void> {
    if (isPath) {
      auto generator = storage.subscribe_encoded_extended(
          std::move(subId), {path}, OperProtocol::SIMPLE_JSON);
      co_await folly::coro::sleep(
          std::chrono::milliseconds(folly::Random::rand32(0, 5000)));
    } else {
      auto generator = storage.subscribe_delta_extended(
          std::move(subId), {path}, OperProtocol::SIMPLE_JSON);
      co_await folly::coro::sleep(
          std::chrono::milliseconds(folly::Random::rand32(0, 5000)));
    }
  };

  auto subscribeManyAndUnregister = [&](bool isPath,
                                        int count) -> folly::coro::Task<void> {
    std::vector<folly::coro::Task<void>> tasks;
    tasks.reserve(count);
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
