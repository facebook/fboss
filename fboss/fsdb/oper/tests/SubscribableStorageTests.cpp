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
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/oper/tests/TestHelpers.h"
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

using folly::dynamic;
using namespace testing;

namespace {

using namespace facebook::fboss::fsdb;

// TODO: templatize test cases
using TestStructMembers = apache::thrift::reflect_struct<TestStruct>::member;

constexpr auto kSubscriber = "testSubscriber";

template <typename value_type>
folly::coro::Task<value_type> consumeOne(
    SubscriptionStreamReader<value_type>& reader) {
  auto& generator = reader.generator_;
  auto item = co_await generator.next();
  auto&& value = *item;
  co_return std::move(value);
}

template <typename Gen>
folly::coro::Task<typename Gen::value_type> consumeOne(Gen& generator) {
  auto item = co_await generator.next();
  auto&& value = *item;
  co_return std::move(value);
}

} // namespace

template <bool EnableHybridStorage>
struct TestParams {
  static constexpr auto hybridStorage = EnableHybridStorage;
};

using SubscribableStorageTestTypes =
    ::testing::Types<TestParams<false>, TestParams<true>>;

template <typename TestParams>
class SubscribableStorageTests : public Test {
 public:
  void SetUp() override {
    testStruct = initializeTestStruct();
  }

  auto initStorage(auto& val) {
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

  constexpr bool isHybridStorage() {
    return TestParams::hybridStorage;
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
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
    return;
  }

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

  auto streamReader = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);
  storage.start();
  // First sync post subscription setup
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  EXPECT_EQ(deltaVal.val.changes()->size(), 1);
  auto first = deltaVal.val.changes()->at(0);
  // Synced entire tree from root
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({})));

  // Make changes, we should see that come in as delta now
  EXPECT_EQ(storage.set(this->root.tx(), false), std::nullopt);

  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));

  EXPECT_EQ(deltaVal.val.changes()->size(), 1);
  first = deltaVal.val.changes()->at(0);
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({"tx"})));

  // Should eventually recv heartbeat
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  EXPECT_EQ(deltaVal.val.changes()->size(), 0);
}

TYPED_TEST(SubscribableStorageTests, SubscribeHybridDelta) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);

  auto streamReader = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);
  storage.start();

  // First sync post subscription setup
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  auto deltaVal = std::move(element.val);
  EXPECT_EQ(deltaVal.changes()->size(), 1);
  auto first = deltaVal.changes()->at(0);
  // Synced entire tree from root
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({})));

  // Make change under hybrid node and verify delta
  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);

  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  deltaVal = std::move(element.val);

  EXPECT_EQ(deltaVal.changes()->size(), 1);
  EXPECT_TRUE(deltaVal.metadata());
  first = deltaVal.changes()->at(0);

  // verify base path
  EXPECT_EQ(first.path()->raw()->size(), 2);
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({"structMap", "99"})));

  EXPECT_FALSE(first.oldState());

  // verify received newState...
  TestStructSimple deserialized = facebook::fboss::thrift_cow::
      deserialize<apache::thrift::type_class::structure, TestStructSimple>(
          OperProtocol::SIMPLE_JSON, *first.newState());
  EXPECT_EQ(deserialized.min(), 999);

  // now delete the parent and verify we see the deletion delta too
  storage.remove(this->root.structMap()[99]);
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  deltaVal = std::move(element.val);

  EXPECT_EQ(deltaVal.changes()->size(), 1);
  EXPECT_TRUE(deltaVal.metadata());
  first = deltaVal.changes()->at(0);

  // verify base path
  EXPECT_EQ(first.path()->raw()->size(), 2);
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({"structMap", "99"})));

  EXPECT_FALSE(first.newState());
  deserialized = facebook::fboss::thrift_cow::
      deserialize<apache::thrift::type_class::structure, TestStructSimple>(
          OperProtocol::SIMPLE_JSON, *first.oldState());
  EXPECT_EQ(deserialized.min(), 999);

  // Should eventually recv heartbeat
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  deltaVal = std::move(element.val);
  EXPECT_EQ(deltaVal.changes()->size(), 0);
}

TYPED_TEST(SubscribableStorageTests, SubscribePatch) {
  using namespace facebook::fboss::fsdb;
  using namespace facebook::fboss::thrift_cow;

  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);

  auto streamReader = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), this->root);
  auto generator = std::move(streamReader.generator_);
  storage.start();

  // Initial sync post subscription setup
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  auto msg = std::move(element.val);
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
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  msg = std::move(element.val);
  patchGroups = *msg.get_chunk().patchGroups();
  EXPECT_EQ(patchGroups.size(), 1);
  patches = patchGroups.begin()->second;
  EXPECT_EQ(patches.size(), 1);
  patch = patches.front();
  using LocalTestStructMembers =
      apache::thrift::reflect_struct<TestStruct>::member;
  auto newVal = patch.patch()
                    ->struct_node_ref()
                    ->children()
                    ->at(LocalTestStructMembers::tx::id::value)
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
  auto streamReader = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), path);
  auto generator = std::move(streamReader.generator_);

  // set and check
  EXPECT_EQ(storage.set(path, 1), std::nullopt);
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  auto msg = std::move(element.val);
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
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  msg = std::move(element.val);
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
  auto streamReader = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {
          {1, std::move(p1)},
          {2, std::move(p2)},
      });
  auto generator = std::move(streamReader.generator_);

  // set and check, should only recv one patch on the path that exists
  EXPECT_EQ(storage.set(path1, 123), std::nullopt);
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  auto msg = std::move(element.val);
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
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  msg = std::move(element.val);
  patchGroups = *msg.get_chunk().patchGroups();

  EXPECT_EQ(patchGroups.size(), 2);
  for (auto& [key, patchGroup] : patchGroups) {
    EXPECT_EQ(patchGroup.size(), 1); // only one patch per raw path
    auto& currentPatch = patchGroup.front();
    EXPECT_EQ(currentPatch.basePath()[1], fmt::format("test{}", key));
    auto currentRootPatch = currentPatch.patch()->move_val();
    deserialized = facebook::fboss::thrift_cow::
        deserializeBuf<apache::thrift::type_class::integral, int32_t>(
            *currentPatch.protocol(), std::move(currentRootPatch));
    // above we set values such that it's sub key * 100 for easier testing
    EXPECT_EQ(deserialized, key * 100);
  }
}

TYPED_TEST(SubscribableStorageTests, SubscribePatchHeartbeat) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  auto streamReader = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), this->root);
  auto generator = std::move(streamReader.generator_);

  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  auto msg = std::move(element.val);
  // first message is initial sync
  EXPECT_EQ(msg.getType(), SubscriberMessage::Type::chunk);
  // Should eventually recv heartbeat
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(20)));
  msg = std::move(element.val);
  EXPECT_EQ(msg.getType(), SubscriberMessage::Type::heartbeat);
}

TYPED_TEST(SubscribableStorageTests, SubscribeHeartbeatConfigured) {
  FLAGS_serveHeartbeats = true;
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  SubscriptionStorageParams params1(std::chrono::seconds(2));
  SubscriptionStorageParams params2(std::chrono::seconds(10));

  auto streamReader1 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      params1);
  auto generator1 = std::move(streamReader1.generator_);
  auto streamReader2 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), this->root);
  auto generator2 = std::move(streamReader2.generator_);

  auto element1 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(20)));
  auto msg1 = std::move(element1.val);
  // first message is initial sync
  EXPECT_EQ(msg1.getType(), SubscriberMessage::Type::chunk);
  // Should recv heartbeat every 2 seconds. Allow leeway incase previous
  // heartbeat was just sent
  element1 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(3)));
  msg1 = std::move(element1.val);
  EXPECT_EQ(msg1.getType(), SubscriberMessage::Type::heartbeat);

  // First sync post subscription setup
  auto element2 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator2), std::chrono::seconds(5)));
  auto msg2 = std::move(element2.val);

  // Heartbeat configured for 10 seconds, so we should not see one within 8
  try {
    element2 = folly::coro::blockingWait(
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

  auto streamReader1 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      this->root,
      params);
  auto generator1 = std::move(streamReader1.generator_);
  // Keep default subscription hearbeat interval of 5 seconds
  auto streamReader2 = storage.subscribe_patch(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))), this->root);
  auto generator2 = std::move(streamReader2.generator_);

  auto element1 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(20)));
  auto msg1 = std::move(element1.val);
  // first message is initial sync
  EXPECT_EQ(msg1.getType(), SubscriberMessage::Type::chunk);
  // Should not receive any heartbeat
  try {
    element1 = folly::coro::blockingWait(
        folly::coro::timeout(consumeOne(generator1), std::chrono::seconds(20)));
  } catch (const std::exception& ex) {
    EXPECT_EQ(typeid(ex), typeid(folly::FutureTimeout));
  }
  // First sync post subscription setup
  auto element2 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator2), std::chrono::seconds(5)));
  auto msg2 = std::move(element2.val);

  // Should recv heartbeat every 5 seconds(default interval). Allow leeway
  // incase previous heartbeat was just sent
  element2 = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator2), std::chrono::seconds(6)));
  msg2 = std::move(element2.val);
  EXPECT_EQ(msg2.getType(), SubscriberMessage::Type::heartbeat);
}

TYPED_TEST(SubscribableStorageTests, SubscribeDeltaUpdate) {
  auto storage = this->initStorage(this->testStruct);
  storage.start();

  const auto& path = this->root.stringToStruct()["test"].max();
  auto streamReader = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      path,
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);

  // set value and subscribe
  EXPECT_EQ(storage.set(path, 1), std::nullopt);
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  auto deltaVal = std::move(element.val);
  EXPECT_EQ(deltaVal.changes()->size(), 1);
  auto first = deltaVal.changes()->at(0);
  EXPECT_THAT(
      *first.path()->raw(),
      ::testing::ContainerEq(std::vector<std::string>({})));
  EXPECT_FALSE(first.oldState());
  EXPECT_TRUE(first.newState());

  // update value
  EXPECT_EQ(storage.set(path, 10), std::nullopt);
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  deltaVal = std::move(element.val);
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
  auto streamReader = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      std::move(path),
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
    return;
  }

  storage.start();

  TestStructSimple newStruct;
  newStruct.min() = 999;
  newStruct.max() = 1001;
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  auto deltaVal = std::move(element.val);

  EXPECT_EQ(deltaVal.changes()->size(), 1);
  EXPECT_TRUE(deltaVal.metadata());
  auto first = deltaVal.changes()->at(0);

  // in this case, the delta has the relative path, which should be empty
  EXPECT_EQ(first.path()->raw()->size(), 0);

  EXPECT_FALSE(first.oldState());

  EXPECT_EQ(folly::to<int>(*first.newState()), 999);

  // now delete the parent and verify we see the deletion delta too
  storage.remove(this->root.structMap()[99]);
  element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  deltaVal = std::move(element.val);

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

TYPED_TEST(SubscribableStorageTests, SubscribeExtendedPathSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
  FLAGS_serveHeartbeats = true;

  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);

  auto path =
      ext_path_builder::raw(TestStructMembers::mapOfStringToI32::id::value)
          .regex("test1.*")
          .get();
  auto generator = storage.subscribe_encoded_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {path},
      OperProtocol::SIMPLE_JSON);
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
    return;
  }

  storage.start();

  EXPECT_EQ(
      storage.set(this->root.mapOfStringToI32()["test1"], 998), std::nullopt);
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

TYPED_TEST(SubscribableStorageTests, SubscribeExtendedPathMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = this->initStorage(this->testStruct);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto generator = storage.subscribe_encoded_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {path},
      OperProtocol::SIMPLE_JSON);
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
    return;
  }

  storage.start();

  EXPECT_EQ(
      storage.set(this->root, createTestStructForExtendedTests()),
      std::nullopt);
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

TYPED_TEST(SubscribableStorageTests, SubscribeExtendedDeltaSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = this->initStorage(this->testStruct);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto streamReader = storage.subscribe_delta_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {path},
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
    return;
  }

  storage.start();

  EXPECT_EQ(
      storage.set(this->root.mapOfStringToI32()["test1"], 998), std::nullopt);
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  auto streamedVal = std::move(element.val);

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

CO_TYPED_TEST(SubscribableStorageTests, SubscribeExtendedDeltaUpdate) {
  auto storage = this->initStorage(this->testStruct);
  storage.start();

  const auto& path =
      ext_path_builder::raw("stringToStruct").regex("test1.*").raw("max").get();
  auto streamReader = storage.subscribe_delta_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {path},
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
  } else {
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
}

TYPED_TEST(SubscribableStorageTests, SubscribeExtendedDeltaMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = this->initStorage(this->testStruct);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto streamReader = storage.subscribe_delta_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {path},
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
    return;
  }

  storage.start();

  EXPECT_EQ(
      storage.set(this->root, createTestStructForExtendedTests()),
      std::nullopt);
  auto element = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(5)));
  auto streamedVal = std::move(element.val);

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
  auto streamReader = storage.subscribe_delta(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      path,
      OperProtocol::SIMPLE_JSON);
  auto generator = std::move(streamReader.generator_);

  // add path and wait for it to be served (publishAndAddPaths)
  EXPECT_EQ(storage.set(path, 1), std::nullopt);
  auto deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.val.changes()->size(), 1);

  auto initialNumPathStores = storage.numPathStoresRecursive_Expensive();
  XLOG(DBG2) << "initialNumPathStores: " << initialNumPathStores;

  // add another path and check PathStores count
  EXPECT_EQ(storage.set(this->root.structMap()[99], newStruct), std::nullopt);
  EXPECT_EQ(storage.set(path, 2), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.val.changes()->size(), 1);

  auto maxNumPathStores = storage.numPathStoresRecursive_Expensive();
  // with lazy PathStore creation, numPathStores should not change
  // for exact subscriptions when adding/deleting paths.
  EXPECT_EQ(maxNumPathStores, initialNumPathStores);

  // now delete the added path
  storage.remove(this->root.structMap()[99]);
  EXPECT_EQ(storage.set(path, 3), std::nullopt);
  deltaVal = folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
  EXPECT_EQ(deltaVal.val.changes()->size(), 1);

  // after path deletion, numPathStores should drop
  auto finalNumPathStores = storage.numPathStoresRecursive_Expensive();
  XLOG(DBG2) << "finalNumPathStores : " << finalNumPathStores;
  EXPECT_EQ(finalNumPathStores, maxNumPathStores);
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

  using LocalTestStructMembers =
      apache::thrift::reflect_struct<TestStruct>::member;
  std::vector<std::string> path = {
      folly::to<std::string>(LocalTestStructMembers::member::id::value)};
  auto patch = PatchBuilder::build(nodeA, nodeB, std::move(path));

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
  EXPECT_EQ(
      storage.patch(delta).value().code(), StorageError::Code::INVALID_PATH);

  // partially valid path should still fail
  unit.path()->raw() = {"inlineStruct", "invalid", "path"};
  delta.changes() = {unit};
}

CO_TYPED_TEST(SubscribableStorageTests, SubscribeExtendedPatchSimple) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  int key = 0;
  auto generator = storage.subscribe_patch_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {{key, path}});
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
  } else {
    storage.start();

    EXPECT_EQ(
        storage.set(this->root.mapOfStringToI32()["test1"], 998), std::nullopt);
    auto element = co_await folly::coro::timeout(
        consumeOne(generator), std::chrono::seconds(5));
    auto msg = std::move(element.val);
    auto chunk = msg.get_chunk();

    EXPECT_EQ(
        chunk.patchGroups()->size(), 1); // single path -> single patchGroup
    EXPECT_EQ(chunk.patchGroups()->at(key).size(), 1);
    EXPECT_THAT(
        *chunk.patchGroups()->at(key).front().basePath(),
        ::testing::ElementsAre(
            "13", "test1")); // 13 is the id of mapOfStringToI32

    auto testStorage = this->createCowStorage(this->testStruct);
    EXPECT_EQ(
        testStorage.patch(std::move(chunk.patchGroups()->at(key).front())),
        std::nullopt);
    EXPECT_EQ(testStorage.root()->toThrift().mapOfStringToI32()["test1"], 998);
  }
}

CO_TYPED_TEST(SubscribableStorageTests, SubscribeExtendedPatchUpdate) {
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  storage.start();

  auto tgtStorage = this->createCowStorage(this->testStruct);

  const auto& path =
      ext_path_builder::raw("stringToStruct").regex("test1.*").raw("max").get();
  auto generator = storage.subscribe_patch_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {{0, path}});
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
  } else {
    const auto& setPath = this->root.stringToStruct()["test1"].max();
    EXPECT_EQ(storage.set(setPath, 1), std::nullopt);
    auto ret = co_await co_awaitTry(
        folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
    EXPECT_FALSE(ret.hasException());

    for (auto& patch : ret.value().val.chunk_ref()->patchGroups()->at(0)) {
      EXPECT_EQ(tgtStorage.patch(std::move(patch)), std::nullopt);
    }

    // update value
    EXPECT_EQ(storage.set(setPath, 10), std::nullopt);
    ret = co_await co_awaitTry(
        folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
    EXPECT_FALSE(ret.hasException());

    for (auto& patch : ret.value().val.chunk_ref()->patchGroups()->at(0)) {
      EXPECT_EQ(tgtStorage.patch(std::move(patch)), std::nullopt);
    }

    // remove value
    storage.remove(this->root.stringToStruct()["test1"]);
    ret = co_await co_awaitTry(
        folly::coro::timeout(consumeOne(generator), std::chrono::seconds(1)));
    EXPECT_FALSE(ret.hasException());
    SubscriberChunk subChunk = *ret.value().val.chunk_ref();
    EXPECT_EQ(subChunk.patchGroups()->size(), 1);
    EXPECT_EQ(subChunk.patchGroups()->at(0).size(), 1);
    auto patch = subChunk.patchGroups()->at(0).front();
    EXPECT_EQ(patch.basePath()->size(), 3);
    EXPECT_EQ(
        patch.patch()->getType(),
        facebook::fboss::thrift_cow::PatchNode::Type::del);
    for (auto& subPatch : ret.value().val.chunk_ref()->patchGroups()->at(0)) {
      EXPECT_EQ(tgtStorage.patch(std::move(subPatch)), std::nullopt);
    }
  }
}

CO_TYPED_TEST(SubscribableStorageTests, SubscribeExtendedPatchMultipleChanges) {
  // add subscription for a path that doesn't exist yet, then add parent
  auto storage = this->initStorage(this->testStruct);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  int key = 0;
  auto generator = storage.subscribe_patch_extended(
      std::move(SubscriptionIdentifier(SubscriberId(kSubscriber))),
      {{key, path}});
  if (this->isHybridStorage()) {
    // extended subscription under HybridNode is unsupported
  } else {
    storage.start();

    EXPECT_EQ(
        storage.set(this->root, createTestStructForExtendedTests()),
        std::nullopt);
    auto element = co_await folly::coro::timeout(
        consumeOne(generator), std::chrono::seconds(5));
    auto msg = std::move(element.val);
    auto chunk = msg.get_chunk();

    EXPECT_EQ(
        chunk.patchGroups()->size(), 1); // single path -> single patchGroup
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

    auto testStorage = this->createCowStorage(this->testStruct);

    for (auto& patch : chunk.patchGroups()->at(key)) {
      // this will be testXY
      auto lastPathElem = patch.basePath()->at(patch.basePath()->size() - 1);
      EXPECT_EQ(testStorage.patch(std::move(patch)), std::nullopt);
      EXPECT_EQ(
          testStorage.root()->toThrift().mapOfStringToI32()->at(lastPathElem),
          expected.at(lastPathElem));
    }
  }
}

class SubscribableStorageTestsPathDelta
    : public Test,
      public WithParamInterface<std::tuple<bool>> {
 public:
  void SetUp() override {
    const std::tuple<bool> params = GetParam();
    isPath = get<0>(params);
    testStruct = initializeTestStruct();
  }

 protected:
  thriftpath::RootThriftPath<TestStruct> root;
  TestStruct testStruct;
  bool isPath{false};
};

INSTANTIATE_TEST_SUITE_P(
    SubscribableStorageSubTypeTests,
    SubscribableStorageTestsPathDelta,
    Combine(Bool()));

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
      auto streamReader = storage.subscribe_delta_extended(
          std::move(subId), {path}, OperProtocol::SIMPLE_JSON);
      auto generator = std::move(streamReader.generator_);
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
      auto streamReader = storage.subscribe_delta_extended(
          std::move(subId), {path}, OperProtocol::SIMPLE_JSON);
      auto generator = std::move(streamReader.generator_);
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
  backgroundScope.add(co_withExecutor(
      folly::getGlobalCPUExecutor(), subscribeManyAndUnregister(isPath, 50)));

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
