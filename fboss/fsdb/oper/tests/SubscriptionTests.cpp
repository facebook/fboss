// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/fsdb/oper/Subscription.h>
#include <fboss/fsdb/oper/SubscriptionStore.h>

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Timeout.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest.h>

#include <unordered_map>

namespace facebook::fboss::fsdb::test {

constexpr int32_t kSubscriptionServeQueueSize = 100;

namespace {
template <typename Gen>
folly::coro::Task<typename Gen::value_type> consumeOne(Gen& generator) {
  auto item = co_await generator.next();
  auto&& value = *item;
  co_return std::move(value);
}
} // namespace

template <typename SubscriptionT>
class SubscriptionTests : public ::testing::Test {
 public:
  void SetUp() override {
    heartbeatThread_ = std::make_unique<folly::ScopedEventBaseThread>(
        "SubscriptionHeartbeats");
  }

  auto makeSubscription() {
    std::vector<std::string> path = {"test"};
    if constexpr (std::is_same_v<SubscriptionT, ExtendedPatchSubscription>) {
      return SubscriptionT::create(
          SubscriptionIdentifier("test-sub"),
          path,
          OperProtocol::BINARY,
          std::nullopt,
          heartbeatThread_->getEventBase(),
          std::chrono::milliseconds(100),
          kSubscriptionServeQueueSize);
    } else {
      return SubscriptionT::create(
          SubscriptionIdentifier("test-sub"),
          path.begin(),
          path.end(),
          OperProtocol::BINARY,
          std::nullopt,
          heartbeatThread_->getEventBase(),
          std::chrono::milliseconds(100),
          kSubscriptionServeQueueSize);
    }
  }

 private:
  std::shared_ptr<folly::ScopedEventBaseThread> heartbeatThread_;
};

using SimpleSubTypes = ::testing::
    Types<PathSubscription, DeltaSubscription, ExtendedPatchSubscription>;
TYPED_TEST_SUITE(SubscriptionTests, SimpleSubTypes);

TYPED_TEST(SubscriptionTests, verifyHeartbeat) {
  auto [gen, sub] = this->makeSubscription();
  // Should get a heartbeat chunk
  folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(gen), std::chrono::seconds{1}));
}

TEST(SubscriptionIdentifierTest, equalityComparesBothFields) {
  SubscriptionIdentifier base("subA", 5);
  EXPECT_EQ(base, SubscriptionIdentifier("subA", 5));
  EXPECT_FALSE(base == SubscriptionIdentifier("subB", 5)); // subscriberId
  EXPECT_FALSE(base == SubscriptionIdentifier("subA", 6)); // uid
}

TEST(SubscriptionIdentifierTest, hashConsistentWithEquality) {
  SubscriptionIdentifier::Hash hash;
  // uid set: equal ids hash equal
  EXPECT_EQ(
      hash(SubscriptionIdentifier("subA", 5)),
      hash(SubscriptionIdentifier("subA", 5)));
  // uid unset (0): hash derives from subscriberId, equal ids hash equal
  EXPECT_EQ(
      hash(SubscriptionIdentifier("subA")),
      hash(SubscriptionIdentifier("subA")));
}

TEST(SubscriptionIdentifierTest, usableAsUnorderedMapKey) {
  std::unordered_map<SubscriptionIdentifier, int, SubscriptionIdentifier::Hash>
      index;
  index.emplace(SubscriptionIdentifier("subA", 1), 1);
  index.emplace(SubscriptionIdentifier("subB"), 2); // uid 0
  EXPECT_EQ(index.at(SubscriptionIdentifier("subA", 1)), 1);
  EXPECT_EQ(index.at(SubscriptionIdentifier("subB")), 2);
  EXPECT_EQ(index.find(SubscriptionIdentifier("subA", 2)), index.end());
}

namespace {
ExtendedOperPath makeExtendedPath(const std::vector<std::string>& tokens) {
  ExtendedOperPath extPath;
  std::vector<OperPathElem> elems;
  elems.reserve(tokens.size());
  for (const auto& tok : tokens) {
    elems.emplace_back().set_raw(tok);
  }
  extPath.path() = std::move(elems);
  return extPath;
}
} // namespace

TEST(ExtendedSubscriptionTest, addPathsAppendsAndSkipsCollisions) {
  folly::ScopedEventBaseThread heartbeatThread("SubscriptionHeartbeats");

  ExtSubPathMap initialPaths;
  initialPaths[1] = makeExtendedPath({"a"});
  auto [gen, sub] = ExtendedPatchSubscription::create(
      SubscriptionIdentifier("test-sub"),
      initialPaths,
      OperProtocol::BINARY,
      std::nullopt,
      heartbeatThread.getEventBase(),
      std::chrono::milliseconds(100),
      kSubscriptionServeQueueSize);

  // Append two new keys; both are inserted.
  ExtSubPathMap newPaths;
  newPaths[2] = makeExtendedPath({"b"});
  newPaths[3] = makeExtendedPath({"c"});
  const std::vector<SubscriptionKey> expectedAdded{2, 3};
  EXPECT_EQ(sub->addPaths(std::move(newPaths)), expectedAdded);
  EXPECT_EQ(sub->size(), 3);

  // Colliding key (2) is skipped; only the new key (4) is inserted.
  ExtSubPathMap collidingPaths;
  collidingPaths[2] = makeExtendedPath({"b"});
  collidingPaths[4] = makeExtendedPath({"d"});
  const std::vector<SubscriptionKey> expectedAfterCollision{4};
  EXPECT_EQ(sub->addPaths(std::move(collidingPaths)), expectedAfterCollision);
  EXPECT_EQ(sub->size(), 4);
}

// Unregistering an extended subscription with pending added-path work must
// clear its extendedSubsWithAddedPaths_ entry -- that list holds a strong
// shared_ptr, so a leftover entry would leak the cancelled subscription.
TEST(ExtendedSubscriptionTest, unregisterClearsAddedPathsTracking) {
  folly::ScopedEventBaseThread heartbeatThread("SubscriptionHeartbeats");
  SubscriptionStore store;

  ExtSubPathMap initialPaths;
  initialPaths[1] = makeExtendedPath({"a"});
  auto [gen, sub] = ExtendedPatchSubscription::create(
      SubscriptionIdentifier("test-sub"),
      initialPaths,
      OperProtocol::BINARY,
      std::nullopt,
      heartbeatThread.getEventBase(),
      std::chrono::milliseconds(100),
      kSubscriptionServeQueueSize);

  std::shared_ptr<ExtendedSubscription> shared = std::move(sub);
  const auto id = shared->subscriptionId();
  store.registerExtendedSubscription(shared);
  ASSERT_EQ(store.extendedSubscriptions().size(), 1);
  const std::string name = store.extendedSubscriptions().begin()->first;

  // Simulate initial sync already done, so appended paths are recorded as
  // deferred work (extendedSubsWithAddedPaths_) rather than folded into it.
  store.initialSyncNeededExtended().erase(shared);

  ExtSubPathMap newPaths;
  newPaths[2] = makeExtendedPath({"b"});
  EXPECT_EQ(
      store.addPatchSubscriptionPaths(
          id, std::move(newPaths), std::nullopt /* publisherRoot */),
      std::nullopt);
  ASSERT_EQ(store.extendedSubsWithAddedPaths().size(), 1);

  // Drop the local strong ref so only the store holds it; weak_ptr then detects
  // whether the subscription is pinned alive after unregister.
  std::weak_ptr<ExtendedSubscription> weak = shared;
  shared.reset();

  store.unregisterExtendedSubscription(name);

  EXPECT_TRUE(store.extendedSubsWithAddedPaths().empty());
  // If unregister failed to erase the tracking entry, its strong shared_ptr
  // would keep the cancelled subscription alive here.
  EXPECT_TRUE(weak.expired());
}

} // namespace facebook::fboss::fsdb::test
