// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

// Integration tests for dynamic resolution of wildcard PATCH extended
// subscriptions (FLAGS_dynamicWildcardPatchResolution). Each test is
// parameterized over the flag value; the subscriber-visible output must be
// identical whether the flag is OFF (eager resolved-object expansion) or ON
// (dynamic per-cycle matching), which is what gives us flag OFF/ON parity.

#include <gtest/gtest.h>
#include <chrono>
#include <map>
#include <optional>

#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h>
#include <fboss/fsdb/oper/SubscriptionPathStore.h>
#include <fboss/lib/CommonUtils.h>
#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/coro/Timeout.h>
#include <gflags/gflags.h>

#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/oper/tests/TestHelpers.h"
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2/thriftpath_test_types.h"

namespace facebook::fboss::fsdb {
namespace {

using namespace std::chrono_literals;

constexpr auto kSubscriber = "dynamicWildcardSubscriber";
// Field id of TestStruct.mapOfStringToI32 (used as the first basePath element
// once paths are converted to id form).
constexpr auto kMapOfStringToI32Id = "13";

template <typename value_type>
folly::coro::Task<value_type> consumeOne(
    SubscriptionStreamReader<value_type>& reader) {
  auto& generator = reader.generator_;
  auto item = co_await generator.next();
  auto&& value = *item;
  co_return std::move(value);
}

// Consume the next SubscriberMessage, returning std::nullopt on timeout.
template <typename Reader>
folly::coro::Task<std::optional<SubscriberMessage>> nextMessage(
    Reader& reader,
    std::chrono::milliseconds timeout) {
  auto res = co_await folly::coro::co_awaitTry(
      folly::coro::timeout(consumeOne(reader), timeout));
  if (res.hasException()) {
    co_return std::nullopt;
  }
  co_return std::move(res.value().val);
}

// Consume the next chunk message, skipping heartbeats, returning nullopt if no
// chunk arrives within the timeout.
template <typename Reader>
std::optional<SubscriberChunk> nextChunk(
    Reader& reader,
    std::chrono::milliseconds timeout) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now());
    auto msg = folly::coro::blockingWait(
        nextMessage(reader, std::max(remaining, 1ms)));
    if (!msg.has_value()) {
      break;
    }
    if (msg->getType() == SubscriberMessage::Type::chunk) {
      return msg->move_chunk();
    }
    // otherwise a heartbeat: keep waiting
  }
  return std::nullopt;
}

// Assert that no chunk arrives within the window (heartbeats are tolerated).
template <typename Reader>
void expectNoChunk(Reader& reader, std::chrono::milliseconds window) {
  auto chunk = nextChunk(reader, window);
  EXPECT_FALSE(chunk.has_value())
      << "Unexpected chunk emitted for a non-matching change";
}

class DynamicWildcardPatchSubscriptionTest
    : public ::testing::TestWithParam<bool> {
 protected:
  void SetUp() override {
    FLAGS_dynamicWildcardPatchResolution = GetParam();
    testStruct_ = initializeTestStruct();
  }

  bool dynamicEnabled() const {
    return GetParam();
  }

  auto initStorage(const TestStruct& val) {
    return NaivePeriodicSubscribableCowStorage<TestStruct, false>(val);
  }

  // google::FlagSaver
  gflags::FlagSaver flagSaver_;
  TestStruct testStruct_;
  thriftpath::RootThriftPath<TestStruct> root_;
};

// Target store for applying served patches, kept free so both fixtures can use
// it without respelling the CowStorage instantiation.
auto createCowStorage(TestStruct val) {
  return CowStorage<
      TestStruct,
      thrift_cow::ThriftStructNode<
          TestStruct,
          thrift_cow::ThriftStructResolver<TestStruct, false>,
          false>>(val);
}

// regex "test1.*" matches test1 and test10..test19 (11 keys); the value of each
// matching key equals the integer suffix.
std::map<std::string, int> expectedTest1Matches() {
  std::map<std::string, int> expected;
  expected["test1"] = 1;
  for (int i = 10; i <= 19; ++i) {
    expected[fmt::format("test{}", i)] = i;
  }
  return expected;
}

class DynamicWildcardPatchSubscriptionStandaloneTest : public ::testing::Test {
 protected:
  // google::FlagSaver
  gflags::FlagSaver flagSaver_;
};

} // namespace

// Initial sync: all matching concrete paths already present at subscribe time
// must be emitted as one full-state patch each, regardless of flag.
TEST_P(DynamicWildcardPatchSubscriptionTest, InitialSync) {
  auto storage = initStorage(createTestStructForExtendedTests());
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  constexpr int key = 0;
  auto generator = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{key, path}});
  storage.start();

  auto chunk = nextChunk(generator, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->size(), 1);
  auto& patches = chunk->patchGroups()->at(key);

  const auto expected = expectedTest1Matches();
  EXPECT_EQ(patches.size(), expected.size());

  // Apply each patch to a fresh store and confirm both the basePath and the
  // decoded full-state value match the source data.
  auto testStorage = createCowStorage(initializeTestStruct());
  for (auto& patch : patches) {
    ASSERT_GE(patch.basePath()->size(), 2);
    EXPECT_EQ(patch.basePath()->front(), kMapOfStringToI32Id);
    auto matchedKey = patch.basePath()->back();
    EXPECT_TRUE(expected.contains(matchedKey)) << matchedKey;
    EXPECT_EQ(testStorage.patch(std::move(patch)), std::nullopt);
    EXPECT_EQ(
        testStorage.root()->toThrift().mapOfStringToI32()->at(matchedKey),
        expected.at(matchedKey));
  }
}

// Incremental updates: a newly set matching leaf, then a brand-new matching key
// (which exercises the delta walk visiting added nodes without any lookup_ node
// for dynamic mode), then a non-matching change which must emit nothing.
TEST_P(DynamicWildcardPatchSubscriptionTest, IncrementalUpdate) {
  testStruct_.mapOfStringToI32()["test1"] = 1;
  auto storage = initStorage(testStruct_);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  constexpr int key = 0;
  auto generator = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{key, path}});
  storage.start();

  auto testStorage = createCowStorage(initializeTestStruct());
  auto initialChunk = nextChunk(generator, 5s);
  ASSERT_TRUE(initialChunk.has_value());
  for (auto& patch : initialChunk->patchGroups()->at(key)) {
    ASSERT_EQ(testStorage.patch(std::move(patch)), std::nullopt);
  }

  // 1) Update an already-synced matching leaf.
  EXPECT_EQ(storage.set(root_.mapOfStringToI32()["test1"], 998), std::nullopt);
  {
    auto chunk = nextChunk(generator, 5s);
    ASSERT_TRUE(chunk.has_value());
    ASSERT_EQ(chunk->patchGroups()->size(), 1);
    auto& patches = chunk->patchGroups()->at(key);
    ASSERT_EQ(patches.size(), 1);
    EXPECT_EQ(patches.front().basePath()->back(), "test1");
    EXPECT_EQ(testStorage.patch(std::move(patches.front())), std::nullopt);
    EXPECT_EQ(
        testStorage.root()->toThrift().mapOfStringToI32()->at("test1"), 998);
  }

  // 2) Add a brand-new matching key.
  EXPECT_EQ(storage.set(root_.mapOfStringToI32()["test15"], 515), std::nullopt);
  {
    auto chunk = nextChunk(generator, 5s);
    ASSERT_TRUE(chunk.has_value());
    auto& patches = chunk->patchGroups()->at(key);
    ASSERT_EQ(patches.size(), 1);
    EXPECT_EQ(patches.front().basePath()->back(), "test15");
    EXPECT_EQ(testStorage.patch(std::move(patches.front())), std::nullopt);
    EXPECT_EQ(
        testStorage.root()->toThrift().mapOfStringToI32()->at("test15"), 515);
  }

  // 3) A non-matching change emits nothing.
  EXPECT_EQ(storage.set(root_.mapOfStringToI32()["test2"], 222), std::nullopt);
  expectNoChunk(generator, 1s);
}

// With the flag ON, no resolved PatchSubscription objects are created for the
// matching concrete paths; with it OFF, one per matching key is created. This
// is the core memory/efficiency win and guards against expansion.
TEST_P(DynamicWildcardPatchSubscriptionTest, NoResolvedObjectExpansion) {
  auto storage = initStorage(createTestStructForExtendedTests());
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto generator = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{0, path}});
  storage.start();

  // Wait until the initial sync chunk has been served.
  auto chunk = nextChunk(generator, 5s);
  ASSERT_TRUE(chunk.has_value());

  const auto numMatches = expectedTest1Matches().size();
  if (dynamicEnabled()) {
    // No resolved child subscriptions registered.
    WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), 0));
  } else {
    WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), numMatches));
  }
}

TEST_P(
    DynamicWildcardPatchSubscriptionTest,
    LateSubscriptionUsesCurrentStateOnce) {
  testStruct_.mapOfStringToI32()["test1"] = 1;
  SynchronousServeStorage<TestStruct> storage(testStruct_);
  storage.setConvertToIDPaths(true);

  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId("lateSubscriber")), {{0, path}});
  ASSERT_EQ(storage.set(root_.mapOfStringToI32()["test1"], 999), std::nullopt);
  storage.serveOnce();

  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->at(0).size(), 1);
  auto target = createCowStorage(initializeTestStruct());
  ASSERT_EQ(
      target.patch(std::move(chunk->patchGroups()->at(0).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test1"), 999);
  storage.serveOnce();
  expectNoChunk(reader, 100ms);
}

TEST_P(
    DynamicWildcardPatchSubscriptionTest,
    PendingAddedPathGetsOneCurrentStatePatch) {
  auto data = createTestStructForExtendedTests();
  SynchronousServeStorage<TestStruct> storage(data);
  storage.setConvertToIDPaths(true);
  auto initialPath =
      ext_path_builder::raw("mapOfStringToI32").regex("test1").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{1, initialPath}});
  storage.serveOnce();
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());

  auto addedPath =
      ext_path_builder::raw("mapOfStringToI32").regex("test2").get();
  ASSERT_EQ(
      storage.add_extended_patch_subscription_path(
          SubscriptionIdentifier(SubscriberId(kSubscriber)),
          2,
          std::move(addedPath)),
      std::nullopt);
  ASSERT_EQ(storage.set(root_.mapOfStringToI32()["test2"], 202), std::nullopt);
  storage.serveOnce();

  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->size(), 1);
  ASSERT_EQ(chunk->patchGroups()->at(2).size(), 1);
  auto target = createCowStorage(initializeTestStruct());
  ASSERT_EQ(
      target.patch(std::move(chunk->patchGroups()->at(2).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test2"), 202);

  ASSERT_EQ(storage.set(root_.mapOfStringToI32()["test2"], 203), std::nullopt);
  storage.serveOnce();
  auto update = nextChunk(reader, 5s);
  ASSERT_TRUE(update.has_value());
  ASSERT_EQ(update->patchGroups()->at(2).size(), 1);
  ASSERT_EQ(
      target.patch(std::move(update->patchGroups()->at(2).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test2"), 203);
  storage.serveOnce();
  expectNoChunk(reader, 100ms);
}

TEST_F(
    DynamicWildcardPatchSubscriptionStandaloneTest,
    RejectsFirstWildcardAppend) {
  // NOLINTNEXTLINE(facebook-gtest-flag-mutation-without-saver)
  FLAGS_dynamicWildcardPatchResolution = true;
  auto data = createTestStructForExtendedTests();
  SynchronousServeStorage<TestStruct> storage(data);
  storage.setConvertToIDPaths(true);
  auto rawPath = ext_path_builder::raw("mapOfStringToI32").raw("test1").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{1, rawPath}});
  storage.serveOnce();
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());

  auto wildcardPath =
      ext_path_builder::raw("mapOfStringToI32").regex("test2").get();
  EXPECT_EQ(
      storage.add_extended_patch_subscription_path(
          SubscriptionIdentifier(SubscriberId(kSubscriber)),
          2,
          std::move(wildcardPath)),
      FsdbErrorCode::INVALID_REQUEST);

  thriftpath::RootThriftPath<TestStruct> root;
  ASSERT_EQ(storage.set(root.mapOfStringToI32()["test1"], 101), std::nullopt);
  storage.serveOnce();
  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->at(1).size(), 1);
  auto target = createCowStorage(initializeTestStruct());
  ASSERT_EQ(
      target.patch(std::move(chunk->patchGroups()->at(1).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test1"), 101);
}

TEST_F(
    DynamicWildcardPatchSubscriptionStandaloneTest,
    AllowsFirstWildcardAppendWithFlagOff) {
  // NOLINTNEXTLINE(facebook-gtest-flag-mutation-without-saver)
  FLAGS_dynamicWildcardPatchResolution = false;
  auto data = createTestStructForExtendedTests();
  SynchronousServeStorage<TestStruct> storage(data);
  storage.setConvertToIDPaths(true);
  auto rawPath = ext_path_builder::raw("mapOfStringToI32").raw("test1").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{1, rawPath}});
  storage.serveOnce();
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());

  auto wildcardPath =
      ext_path_builder::raw("mapOfStringToI32").regex("test2").get();
  EXPECT_EQ(
      storage.add_extended_patch_subscription_path(
          SubscriptionIdentifier(SubscriberId(kSubscriber)),
          2,
          std::move(wildcardPath)),
      std::nullopt);
  storage.serveOnce();
  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->at(2).size(), 1);
}

TEST_P(DynamicWildcardPatchSubscriptionTest, MatchingKeyDeletion) {
  testStruct_.mapOfStringToI32()["test1"] = 1;
  auto storage = initStorage(testStruct_);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{0, path}});
  storage.start();

  auto target = createCowStorage(testStruct_);
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());
  // SubscribableStorage::remove() returns void, so there is no status to check.
  storage.remove(root_.mapOfStringToI32()["test1"]);
  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->at(0).size(), 1);
  auto patch = std::move(chunk->patchGroups()->at(0).front());
  EXPECT_EQ(
      patch.basePath(),
      std::vector<std::string>({kMapOfStringToI32Id, "test1"}));
  EXPECT_EQ(patch.patch()->getType(), thrift_cow::PatchNode::Type::del);
  ASSERT_EQ(target.patch(std::move(patch)), std::nullopt);
  EXPECT_FALSE(target.root()->toThrift().mapOfStringToI32()->contains("test1"));
}

TEST_P(DynamicWildcardPatchSubscriptionTest, WholesaleStructMapReplacement) {
  TestStructSimple original;
  original.min() = 0;
  original.max() = 10;
  testStruct_.stringToStruct()["test0"] = original;
  auto storage = initStorage(testStruct_);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("stringToStruct").regex("test.*").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{0, path}});
  storage.start();

  auto target = createCowStorage(testStruct_);
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());
  TestStructSimple first;
  first.min() = 1;
  first.max() = 11;
  TestStructSimple second;
  second.min() = 2;
  second.max() = 22;
  std::map<std::string, TestStructSimple> replacement{
      {"test1", first}, {"test2", second}};
  ASSERT_EQ(storage.set(root_.stringToStruct(), replacement), std::nullopt);
  auto added = nextChunk(reader, 5s);
  ASSERT_TRUE(added.has_value());
  for (auto& patch : added->patchGroups()->at(0)) {
    ASSERT_EQ(target.patch(std::move(patch)), std::nullopt);
  }
  EXPECT_EQ(*target.root()->toThrift().stringToStruct(), replacement);

  ASSERT_EQ(
      storage.set(
          root_.stringToStruct(), std::map<std::string, TestStructSimple>{}),
      std::nullopt);
  auto removed = nextChunk(reader, 5s);
  ASSERT_TRUE(removed.has_value());
  for (auto& patch : removed->patchGroups()->at(0)) {
    ASSERT_EQ(target.patch(std::move(patch)), std::nullopt);
  }
  EXPECT_TRUE(target.root()->toThrift().stringToStruct()->empty());
}

TEST_P(DynamicWildcardPatchSubscriptionTest, LiveAddedPathGetsCurrentState) {
  auto storage = initStorage(createTestStructForExtendedTests());
  storage.setConvertToIDPaths(true);
  auto initialPath =
      ext_path_builder::raw("mapOfStringToI32").regex("test1").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{1, initialPath}});
  storage.start();
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());

  auto addedPath =
      ext_path_builder::raw("mapOfStringToI32").regex("test2").get();
  ASSERT_EQ(
      storage.add_extended_patch_subscription_path(
          SubscriptionIdentifier(SubscriberId(kSubscriber)),
          2,
          std::move(addedPath)),
      std::nullopt);
  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->size(), 1);
  ASSERT_EQ(chunk->patchGroups()->at(2).size(), 1);
  EXPECT_EQ(chunk->patchGroups()->at(2).front().basePath()->back(), "test2");
  auto target = createCowStorage(initializeTestStruct());
  ASSERT_EQ(
      target.patch(std::move(chunk->patchGroups()->at(2).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test2"), 2);
}

TEST_P(DynamicWildcardPatchSubscriptionTest, RawOnlyPatchRemainsEager) {
  auto data = createTestStructForExtendedTests();
  auto storage = initStorage(data);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").raw("test1").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{0, path}});
  storage.start();
  auto target = createCowStorage(initializeTestStruct());
  auto initial = nextChunk(reader, 5s);
  ASSERT_TRUE(initial.has_value());
  ASSERT_EQ(initial->patchGroups()->at(0).size(), 1);
  ASSERT_EQ(
      target.patch(std::move(initial->patchGroups()->at(0).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test1"), 1);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(storage.numSubscriptions(), 1));

  ASSERT_EQ(storage.set(root_.mapOfStringToI32()["test1"], 101), std::nullopt);
  auto update = nextChunk(reader, 5s);
  ASSERT_TRUE(update.has_value());
  ASSERT_EQ(update->patchGroups()->at(0).size(), 1);
  ASSERT_EQ(
      target.patch(std::move(update->patchGroups()->at(0).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test1"), 101);
}

TEST_P(DynamicWildcardPatchSubscriptionTest, MultiKeySubscription) {
  auto storage = initStorage(createTestStructForExtendedTests());
  storage.setConvertToIDPaths(true);
  auto test1 = ext_path_builder::raw("mapOfStringToI32").regex("test1").get();
  auto test2 = ext_path_builder::raw("mapOfStringToI32").regex("test2").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)),
      {{11, test1}, {22, test2}});
  storage.start();

  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->size(), 2);
  ASSERT_EQ(chunk->patchGroups()->at(11).size(), 1);
  ASSERT_EQ(chunk->patchGroups()->at(22).size(), 1);
  EXPECT_EQ(chunk->patchGroups()->at(11).front().basePath()->back(), "test1");
  EXPECT_EQ(chunk->patchGroups()->at(22).front().basePath()->back(), "test2");

  auto replacement = *createTestStructForExtendedTests().mapOfStringToI32();
  replacement["test1"] = 111;
  replacement["test2"] = 222;
  ASSERT_EQ(storage.set(root_.mapOfStringToI32(), replacement), std::nullopt);
  auto update = nextChunk(reader, 5s);
  ASSERT_TRUE(update.has_value());
  ASSERT_EQ(update->patchGroups()->at(11).size(), 1);
  ASSERT_EQ(update->patchGroups()->at(22).size(), 1);
  auto target = createCowStorage(initializeTestStruct());
  ASSERT_EQ(
      target.patch(std::move(update->patchGroups()->at(11).front())),
      std::nullopt);
  ASSERT_EQ(
      target.patch(std::move(update->patchGroups()->at(22).front())),
      std::nullopt);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test1"), 111);
  EXPECT_EQ(target.root()->toThrift().mapOfStringToI32()->at("test2"), 222);
}

TEST_P(DynamicWildcardPatchSubscriptionTest, ConcurrentSubscribers) {
  testStruct_.mapOfStringToI32()["test1"] = 1;
  auto storage = initStorage(testStruct_);
  storage.setConvertToIDPaths(true);
  auto path = ext_path_builder::raw("mapOfStringToI32").regex("test1").get();
  auto readerA = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId("subscriberA")), {{0, path}});
  auto readerB = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId("subscriberB")), {{0, path}});
  storage.start();
  ASSERT_TRUE(nextChunk(readerA, 5s).has_value());
  ASSERT_TRUE(nextChunk(readerB, 5s).has_value());

  ASSERT_EQ(storage.set(root_.mapOfStringToI32()["test1"], 777), std::nullopt);
  auto chunkA = nextChunk(readerA, 5s);
  auto chunkB = nextChunk(readerB, 5s);
  ASSERT_TRUE(chunkA.has_value());
  ASSERT_TRUE(chunkB.has_value());
  ASSERT_EQ(chunkA->patchGroups()->at(0).size(), 1);
  ASSERT_EQ(chunkB->patchGroups()->at(0).size(), 1);
  auto targetA = createCowStorage(initializeTestStruct());
  auto targetB = createCowStorage(initializeTestStruct());
  ASSERT_EQ(
      targetA.patch(std::move(chunkA->patchGroups()->at(0).front())),
      std::nullopt);
  ASSERT_EQ(
      targetB.patch(std::move(chunkB->patchGroups()->at(0).front())),
      std::nullopt);
  EXPECT_EQ(targetA.root()->toThrift().mapOfStringToI32()->at("test1"), 777);
  EXPECT_EQ(targetB.root()->toThrift().mapOfStringToI32()->at("test1"), 777);
}

// Raw-only PATCH subscriptions must stay on the eager path, so enabling the
// flag must not change how many lookup_ path stores get created for them. Both
// flag states run in one body so the counts can be compared directly.
TEST_F(
    DynamicWildcardPatchSubscriptionStandaloneTest,
    RawOnlyPathStoresUnaffectedByFlag) {
  auto countsWithFlag = [](bool dynamicResolution) {
    // NOLINTNEXTLINE(facebook-gtest-flag-mutation-without-saver)
    FLAGS_dynamicWildcardPatchResolution = dynamicResolution;
    auto data = createTestStructForExtendedTests();
    SynchronousServeStorage<TestStruct> storage(data);
    storage.setConvertToIDPaths(true);
    auto path = ext_path_builder::raw("mapOfStringToI32").raw("test1").get();
    auto reader = storage.subscribe_patch_extended(
        SubscriptionIdentifier(SubscriberId(kSubscriber)), {{0, path}});
    storage.serveOnce();
    EXPECT_TRUE(nextChunk(reader, 5s).has_value());
    return std::make_pair(storage.numPathStores(), storage.numSubscriptions());
  };

  const auto eager = countsWithFlag(false);
  const auto dynamic = countsWithFlag(true);
  EXPECT_EQ(dynamic, eager);
  // Confirms the counts above are not two copies of a degenerate "nothing was
  // created" result: the raw path really did expand into the trie and resolve a
  // child.
  EXPECT_GT(eager.first, 2);
  EXPECT_EQ(eager.second, 1);
}

// Wildcard at a non-leaf position (mapOfStructs/<regex>/m), where the matched
// node is a nested container rather than a scalar leaf. This is the suite's
// only end-to-end coverage of an intermediate wildcard through the real
// DeltaVisitor/RecurseVisitor/PatchBuilder stack.
TEST_P(DynamicWildcardPatchSubscriptionTest, NonLeafWildcardMatchesNestedNode) {
  OtherStruct first;
  first.m() = {{"a", 1}};
  testStruct_.mapOfStructs()["key1"] = first;
  auto storage = initStorage(testStruct_);
  storage.setConvertToIDPaths(true);
  auto path =
      ext_path_builder::raw("mapOfStructs").regex("key.*").raw("m").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{0, path}});
  storage.start();

  auto target = createCowStorage(testStruct_);
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());

  // Mutate a leaf underneath the matched nested map.
  ASSERT_EQ(
      storage.set(root_.mapOfStructs()["key1"].m()["a"], 42), std::nullopt);
  auto updated = nextChunk(reader, 5s);
  ASSERT_TRUE(updated.has_value());
  ASSERT_EQ(updated->patchGroups()->at(0).size(), 1);
  ASSERT_EQ(
      target.patch(std::move(updated->patchGroups()->at(0).front())),
      std::nullopt);
  EXPECT_EQ(
      target.root()->toThrift().mapOfStructs()->at("key1").m()->at("a"), 42);

  // A brand-new matching outer key materializes the nested subtree in one
  // update, which is the wholly-added-node path.
  OtherStruct second;
  second.m() = {{"b", 7}};
  ASSERT_EQ(storage.set(root_.mapOfStructs()["key2"], second), std::nullopt);
  auto added = nextChunk(reader, 5s);
  ASSERT_TRUE(added.has_value());
  ASSERT_EQ(added->patchGroups()->at(0).size(), 1);
  ASSERT_EQ(
      target.patch(std::move(added->patchGroups()->at(0).front())),
      std::nullopt);
  EXPECT_EQ(
      target.root()->toThrift().mapOfStructs()->at("key2").m()->at("b"), 7);

  // A change outside the pattern must not produce a chunk.
  ASSERT_EQ(storage.set(root_.mapOfStringToI32()["other"], 5), std::nullopt);
  expectNoChunk(reader, 500ms);
}

// The first-wildcard rejection is gated on !hasWildcardPath(), so appending a
// second wildcard to a subscription that is already dynamic must be accepted.
TEST_F(
    DynamicWildcardPatchSubscriptionStandaloneTest,
    AllowsSecondWildcardAppend) {
  // NOLINTNEXTLINE(facebook-gtest-flag-mutation-without-saver)
  FLAGS_dynamicWildcardPatchResolution = true;
  auto data = createTestStructForExtendedTests();
  SynchronousServeStorage<TestStruct> storage(data);
  storage.setConvertToIDPaths(true);
  auto firstWildcard =
      ext_path_builder::raw("mapOfStringToI32").regex("test1").get();
  auto reader = storage.subscribe_patch_extended(
      SubscriptionIdentifier(SubscriberId(kSubscriber)), {{1, firstWildcard}});
  storage.serveOnce();
  ASSERT_TRUE(nextChunk(reader, 5s).has_value());

  auto secondWildcard =
      ext_path_builder::raw("mapOfStringToI32").regex("test2").get();
  ASSERT_EQ(
      storage.add_extended_patch_subscription_path(
          SubscriptionIdentifier(SubscriberId(kSubscriber)),
          2,
          std::move(secondWildcard)),
      std::nullopt);
  storage.serveOnce();

  // The appended key receives its current state, confirming it was admitted
  // rather than merely not rejected.
  auto chunk = nextChunk(reader, 5s);
  ASSERT_TRUE(chunk.has_value());
  ASSERT_EQ(chunk->patchGroups()->at(2).size(), 1);
  EXPECT_EQ(chunk->patchGroups()->at(2).front().basePath()->back(), "test2");
}

// Asserts flag OFF/ON parity on the served wire format. The parameterized tests
// only assert applied state, and CowStorage accepts several encodings of the
// same logical change, so they would pass even if the dynamic path emitted a
// different PatchNode variant, basePath, compression decision, or patch count.
TEST_F(DynamicWildcardPatchSubscriptionStandaloneTest, ChunkParityAcrossFlag) {
  // Every step mutates at least one matching key, so every serve cycle is
  // expected to produce a chunk. nextChunk() signals a timeout by letting
  // folly::coro::timeout throw into the generator, which leaves it in an
  // EXCEPTION_WRAPPER state and makes further use of it a fatal check failure,
  // so waiting on a chunk that never arrives would abort the process rather
  // than fail an assertion. The last step therefore combines a non-matching
  // write with a matching deletion, and the resulting chunk must carry only the
  // deletion.
  auto runScenario = [](bool dynamicResolution) {
    // NOLINTNEXTLINE(facebook-gtest-flag-mutation-without-saver)
    FLAGS_dynamicWildcardPatchResolution = dynamicResolution;
    thriftpath::RootThriftPath<TestStruct> root;
    SynchronousServeStorage<TestStruct> storage(
        createTestStructForExtendedTests());
    storage.setConvertToIDPaths(true);
    auto path =
        ext_path_builder::raw("mapOfStringToI32").regex("test1.*").get();
    auto reader = storage.subscribe_patch_extended(
        SubscriptionIdentifier(SubscriberId(kSubscriber)), {{0, path}});

    std::vector<std::optional<SubscriberChunk>> chunks;
    // lastServedAt is stamped per serve, so it cannot match across two runs.
    // lastConfirmedAt and lastPublishedAt come from publisher metadata, which
    // no publisher registers here. Clearing all three leaves the comparison
    // covering basePath, patch node type and contents, protocol,
    // streamRevision, and the number and order of patches per key.
    auto stripServeTimestamps = [](SubscriberChunk& chunk) {
      for (auto& [_key, patches] : *chunk.patchGroups()) {
        for (auto& patch : patches) {
          patch.metadata()->lastConfirmedAt().reset();
          patch.metadata()->lastPublishedAt().reset();
          patch.metadata()->lastServedAt().reset();
        }
      }
    };
    auto serveAndCollect = [&]() {
      if (!chunks.empty() && !chunks.back().has_value()) {
        // An earlier step missed its chunk, which poisoned the generator. Stop
        // rather than crash; the comparison below reports the divergence.
        return;
      }
      storage.serveOnce();
      auto chunk = nextChunk(reader, 5s);
      if (chunk.has_value()) {
        stripServeTimestamps(*chunk);
      }
      chunks.push_back(std::move(chunk));
    };

    // Initial sync over every pre-existing matching key.
    serveAndCollect();
    // Incremental update to an already-matching key. These use EXPECT rather
    // than ASSERT because ASSERT_* returns void, which this lambda cannot do.
    EXPECT_EQ(
        storage.set(root.mapOfStringToI32()["test1"], 1001), std::nullopt);
    serveAndCollect();
    // Brand-new matching key: a fully added node, which in dynamic mode has no
    // lookup_ node for the delta walk to find.
    EXPECT_EQ(
        storage.set(root.mapOfStringToI32()["test199"], 199), std::nullopt);
    serveAndCollect();
    // A non-matching write and a matching deletion in the same cycle. remove()
    // returns void, so only the write has a status to check.
    EXPECT_EQ(storage.set(root.mapOfStringToI32()["other"], 7), std::nullopt);
    storage.remove(root.mapOfStringToI32()["test1"]);
    serveAndCollect();
    return chunks;
  };

  const auto eager = runScenario(false);
  const auto dynamic = runScenario(true);

  // Every step is expected to serve a chunk in both modes.
  ASSERT_EQ(eager.size(), 4u);
  for (size_t i = 0; i < eager.size(); ++i) {
    ASSERT_TRUE(eager[i].has_value())
        << "eager step " << i << " served nothing";
  }
  ASSERT_EQ(dynamic.size(), eager.size());
  for (size_t i = 0; i < eager.size(); ++i) {
    EXPECT_EQ(eager[i], dynamic[i])
        << "served chunk " << i << " differs between eager and dynamic modes";
  }
}

INSTANTIATE_TEST_SUITE_P(
    FlagOffAndOn,
    DynamicWildcardPatchSubscriptionTest,
    ::testing::Values(false, true),
    [](const ::testing::TestParamInfo<bool>& info) {
      return info.param ? "DynamicOn" : "DynamicOff";
    });

} // namespace facebook::fboss::fsdb
