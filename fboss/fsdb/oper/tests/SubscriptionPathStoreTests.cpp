// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/json/dynamic.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>

#include <fboss/fsdb/oper/Subscription.h>
#include <fboss/fsdb/oper/SubscriptionManager.h>
#include <fboss/fsdb/oper/SubscriptionPathStore.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"

namespace {
using namespace facebook::fboss::fsdb;

class TestSubscription : public Subscription {
 public:
  ~TestSubscription() override {
    stop();
  }

  PubSubType type() const override {
    return PubSubType::PATH;
  }

  void allPublishersGone(
      FsdbErrorCode /* disconnectReason */,
      const std::string& /* msg */) override {}

  bool isActive() const override {
    return true;
  }

  void flush(const SubscriptionMetadataServer&) override {}
  void serveHeartbeat() override {}

  explicit TestSubscription(std::vector<std::string> path)
      : Subscription(
            SubscriptionIdentifier("testSubcriber"),
            std::move(path),
            OperProtocol::BINARY,
            std::nullopt,
            nullptr,
            std::chrono::milliseconds(0)) {}
};

class TestDeltaSubscription : public Subscription {
 public:
  ~TestDeltaSubscription() override {
    stop();
  }

  PubSubType type() const override {
    return PubSubType::DELTA;
  }

  void allPublishersGone(
      FsdbErrorCode /* disconnectReason */,
      const std::string& /* msg */) override {}

  bool isActive() const override {
    return true;
  }

  void flush(const SubscriptionMetadataServer&) override {}
  void serveHeartbeat() override {}

  explicit TestDeltaSubscription(std::vector<std::string> path)
      : Subscription(
            SubscriptionIdentifier("testSubcriber"),
            std::move(path),
            OperProtocol::BINARY,
            std::nullopt,
            nullptr,
            std::chrono::milliseconds(0)) {}
};

class TestExtendedSubscription : public ExtendedSubscription {
 public:
  explicit TestExtendedSubscription(ExtSubPathMap paths)
      : ExtendedSubscription(
            SubscriptionIdentifier("testSubcriber"),
            std::move(paths),
            OperProtocol::BINARY,
            std::nullopt,
            nullptr,
            std::chrono::milliseconds(0)) {}

  ~TestExtendedSubscription() override {
    stop();
  }

  PubSubType type() const override {
    return PubSubType::PATH;
  }

  void allPublishersGone(
      FsdbErrorCode /* disconnectReason */,
      const std::string& /* msg */) override {}

  bool isActive() const override {
    return true;
  }

  void flush(const SubscriptionMetadataServer&) override {}
  void serveHeartbeat() override {}
  std::unique_ptr<Subscription> resolve(
      const SubscriptionKey& /* key */,
      const std::vector<std::string>& path) override {
    return std::make_unique<TestSubscription>(path);
  }
};

std::unique_ptr<TestSubscription> makeSubscription(
    std::vector<std::string> path) {
  return std::make_unique<TestSubscription>(std::move(path));
}

std::unique_ptr<TestDeltaSubscription> makeDeltaSubscription(
    std::vector<std::string> path) {
  return std::make_unique<TestDeltaSubscription>(std::move(path));
}

struct StubSubscriptionStore : public SubscriptionStore {
  void registerSubscription(std::unique_ptr<Subscription> sub) override {
    subs.emplace(subs.size(), std::move(sub));
  }

  std::unordered_map<int, std::unique_ptr<Subscription>> subs;
};

} // namespace

TEST(SubscriptionPathStoreTests, FindSimple) {
  using namespace facebook::fboss::fsdb;

  std::array<std::unique_ptr<TestSubscription>, 6> subs = {
      makeSubscription({"a"}),
      makeSubscription({"a", "b"}),
      makeSubscription({"a", "b", "c"}),
      makeSubscription({"a", "b", "c"}),
      makeSubscription({"a", "b", "c", "d"}),
      makeSubscription({"a", "b", "c", "d", "e"})};

  SubscriptionPathStoreTreeStats stats;
  SubscriptionPathStore store(&stats);
  for (const auto& sub : subs) {
    store.add(sub.get(), &stats);
  }

  std::vector<std::string> testPath = {"a", "b", "c"};
  auto targetOnly =
      store.find(testPath.begin(), testPath.end(), {LookupType::TARGET});
  auto targetAndParents = store.find(
      testPath.begin(),
      testPath.end(),
      {LookupType::TARGET, LookupType::PARENTS});
  auto targetAndChildren = store.find(
      testPath.begin(),
      testPath.end(),
      {LookupType::TARGET, LookupType::CHILDREN});
  auto parentsOnly =
      store.find(testPath.begin(), testPath.end(), {LookupType::PARENTS});
  auto childrenOnly =
      store.find(testPath.begin(), testPath.end(), {LookupType::CHILDREN});

  EXPECT_THAT(
      targetOnly,
      ::testing::UnorderedElementsAre(subs[2].get(), subs[3].get()));
  EXPECT_THAT(
      targetAndParents,
      ::testing::UnorderedElementsAre(
          subs[0].get(), subs[1].get(), subs[2].get(), subs[3].get()));
  EXPECT_THAT(
      targetAndChildren,
      ::testing::UnorderedElementsAre(
          subs[2].get(), subs[3].get(), subs[4].get(), subs[5].get()));
  EXPECT_THAT(
      parentsOnly,
      ::testing::UnorderedElementsAre(subs[0].get(), subs[1].get()));
  EXPECT_THAT(
      childrenOnly,
      ::testing::UnorderedElementsAre(subs[4].get(), subs[5].get()));
}

TEST(SubscriptionPathStoreTests, IncrementalResolveExtended) {
  using namespace facebook::fboss::fsdb;

  auto path1 = ext_path_builder::raw("a")
                   .raw("b")
                   .regex("test.*")
                   .raw("d")
                   .raw("e")
                   .raw("f")
                   .get();
  auto path2 = ext_path_builder::raw("a")
                   .raw("b")
                   .regex("test.*")
                   .raw("d")
                   .any()
                   .raw("f")
                   .get();
  auto path3 = ext_path_builder::raw("a")
                   .raw("b")
                   .regex("test.*")
                   .raw("d")
                   .raw("e")
                   .raw("f")
                   .raw("g")
                   .get();
  auto path4 = ext_path_builder::raw("a")
                   .raw("b")
                   .regex("test.*")
                   .raw("d")
                   .any()
                   .raw("f")
                   .regex("test.*")
                   .get();
  auto path5 = ext_path_builder::raw("a").raw("b").get();

  auto extSub1 = std::make_shared<TestExtendedSubscription>(
      ExtSubPathMap{{0, std::move(path1)}});
  auto extSub2 = std::make_shared<TestExtendedSubscription>(
      ExtSubPathMap{{0, std::move(path2)}});
  auto extSub3 = std::make_shared<TestExtendedSubscription>(
      ExtSubPathMap{{0, std::move(path3)}, {1, std::move(path4)}});
  auto extSub4 = std::make_shared<TestExtendedSubscription>(
      ExtSubPathMap{{0, std::move(path5)}});

  SubscriptionPathStoreTreeStats stats;
  SubscriptionPathStore pathStore(&stats);
  StubSubscriptionStore store;

  // seed incremental resolutions of all the subscriptions
  std::vector<std::string> emptyPathSoFar;
  pathStore.incrementallyResolve(store, extSub1, 0, emptyPathSoFar);
  pathStore.incrementallyResolve(store, extSub2, 0, emptyPathSoFar);
  pathStore.incrementallyResolve(store, extSub3, 0, emptyPathSoFar);
  pathStore.incrementallyResolve(store, extSub3, 1, emptyPathSoFar);

  // First subs should only pathStore partially resolved subpaths, no fully
  // resolved paths yet
  EXPECT_EQ(store.subs.size(), 0);

  pathStore.incrementallyResolve(store, extSub4, 0, emptyPathSoFar);

  // Since extSub4 only has raw tokens, we should get a fully resolved
  // subscription added to the store.
  EXPECT_EQ(store.subs.size(), 1);

  // incremental resolve with non-existent pathNum will throw
  EXPECT_THROW(
      pathStore.incrementallyResolve(store, extSub3, 2, emptyPathSoFar),
      std::out_of_range);

  pathStore.debugPrint();

  // vector of pairs of paths to add and the number of resolved paths
  // we expect afterwards. Note we already have 1 resolved
  std::vector<std::pair<std::vector<std::string>, int>> pathsToAddAndTest = {
      {{"a", "b"}, 1},
      {{"a", "b", "c"}, 1},
      {{"a", "b", "test4"}, 3},
      {{"a", "b", "test5"}, 5},
      {{"a", "b", "test5", "d"}, 5},
      {{"a", "b", "test5", "d", "e"}, 6},
      {{"a", "b", "test5", "d", "e", "f"}, 6},
      {{"a", "b", "test5", "d", "e", "f", "g"}, 6},
      {{"a", "b", "test5", "d", "e", "f", "test34"}, 7},
      {{"a", "b", "test5", "d", "e", "f", "test34"}, 7},
  };

  for (int i = 0; i < pathsToAddAndTest.size(); ++i) {
    const auto& [path, numResolvedAfter] = pathsToAddAndTest[i];
    pathStore.processAddedPath(store, path.begin(), path.begin(), path.end());
    XLOG(INFO) << "SubscriptionPathStore state after adding path " << i;
    pathStore.debugPrint();
    ASSERT_EQ(store.subs.size(), numResolvedAfter)
        << "Unexpected # of resolved subs after adding path " << i;
  }
}

TEST(SubscriptionPathStoreTests, TestRecursiveCounts) {
  using namespace facebook::fboss::fsdb;

  std::array<std::unique_ptr<Subscription>, 9> subs = {
      makeSubscription({"a"}),
      makeSubscription({"a", "b"}),
      makeSubscription({"a", "b", "c"}),
      makeSubscription({"a", "b", "c"}),
      makeDeltaSubscription({"a", "b", "c"}),
      makeSubscription({"a", "b", "c", "d"}),
      makeDeltaSubscription({"a", "b", "c", "d"}),
      makeSubscription({"a", "b", "c", "d", "e"}),
      makeDeltaSubscription({"a", "b", "c", "d", "e"})};

  SubscriptionPathStoreTreeStats stats;
  SubscriptionPathStore store(&stats);
  for (const auto& sub : subs) {
    store.add(sub.get(), &stats);
  }

  EXPECT_EQ(store.numSubs(), 0);
  EXPECT_EQ(store.numChildSubs(), 9);
  EXPECT_EQ(store.numSubsRecursive(), 9);

  std::vector<std::string> fullPath = {"a", "b", "c", "d", "e"};

  auto child = store.findStore(fullPath.begin(), fullPath.begin() + 1);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 1);
  EXPECT_EQ(child->numChildSubs(), 8);
  EXPECT_EQ(child->numSubsRecursive(), 9);

  child = store.findStore(fullPath.begin(), fullPath.begin() + 2);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 1);
  EXPECT_EQ(child->numChildSubs(), 7);
  EXPECT_EQ(child->numSubsRecursive(), 8);

  child = store.findStore(fullPath.begin(), fullPath.begin() + 3);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 3);
  EXPECT_EQ(child->numChildSubs(), 4);
  EXPECT_EQ(child->numSubsRecursive(), 7);

  child = store.findStore(fullPath.begin(), fullPath.begin() + 4);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 2);
  EXPECT_EQ(child->numChildSubs(), 2);
  EXPECT_EQ(child->numSubsRecursive(), 4);

  child = store.findStore(fullPath.begin(), fullPath.end());
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 2);
  EXPECT_EQ(child->numChildSubs(), 0);
  EXPECT_EQ(child->numSubsRecursive(), 2);

  // now remove a few of the subscriptions
  store.remove(subs[8].get());
  store.remove(subs[6].get());
  store.remove(subs[5].get());
  store.remove(subs[2].get());
  store.remove(subs[0].get());

  EXPECT_EQ(store.numSubs(), 0);
  EXPECT_EQ(store.numChildSubs(), 4);
  EXPECT_EQ(store.numSubsRecursive(), 4);

  child = store.findStore(fullPath.begin(), fullPath.begin() + 1);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 0);
  EXPECT_EQ(child->numChildSubs(), 4);
  EXPECT_EQ(child->numSubsRecursive(), 4);

  child = store.findStore(fullPath.begin(), fullPath.begin() + 2);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 1);
  EXPECT_EQ(child->numChildSubs(), 3);
  EXPECT_EQ(child->numSubsRecursive(), 4);

  child = store.findStore(fullPath.begin(), fullPath.begin() + 3);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 2);
  EXPECT_EQ(child->numChildSubs(), 1);
  EXPECT_EQ(child->numSubsRecursive(), 3);

  child = store.findStore(fullPath.begin(), fullPath.begin() + 4);
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 0);
  EXPECT_EQ(child->numChildSubs(), 1);
  EXPECT_EQ(child->numSubsRecursive(), 1);

  child = store.findStore(fullPath.begin(), fullPath.end());
  ASSERT_TRUE(child);
  EXPECT_EQ(child->numSubs(), 1);
  EXPECT_EQ(child->numChildSubs(), 0);
  EXPECT_EQ(child->numSubsRecursive(), 1);
}
