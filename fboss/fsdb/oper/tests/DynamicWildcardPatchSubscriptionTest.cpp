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
    flagSaver_.emplace();
    FLAGS_dynamicWildcardPatchResolution = GetParam();
    testStruct_ = initializeTestStruct();
  }

  bool dynamicEnabled() const {
    return GetParam();
  }

  auto initStorage(const TestStruct& val) {
    return NaivePeriodicSubscribableCowStorage<TestStruct, false>(val);
  }

  auto createCowStorage(TestStruct val) {
    return CowStorage<
        TestStruct,
        thrift_cow::ThriftStructNode<
            TestStruct,
            thrift_cow::ThriftStructResolver<TestStruct, false>,
            false>>(val);
  }

  std::optional<gflags::FlagSaver> flagSaver_;
  TestStruct testStruct_;
  thriftpath::RootThriftPath<TestStruct> root_;
};

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

INSTANTIATE_TEST_SUITE_P(
    FlagOffAndOn,
    DynamicWildcardPatchSubscriptionTest,
    ::testing::Values(false, true),
    [](const ::testing::TestParamInfo<bool>& info) {
      return info.param ? "DynamicOn" : "DynamicOff";
    });

} // namespace facebook::fboss::fsdb
