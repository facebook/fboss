// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/WildcardPatchCandidateTracker.h"

#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest.h>

namespace facebook::fboss::fsdb {
namespace {

OperPathElem raw(std::string token) {
  OperPathElem elem;
  elem.set_raw(std::move(token));
  return elem;
}

OperPathElem any() {
  OperPathElem elem;
  elem.set_any(true);
  return elem;
}

OperPathElem regex(std::string r) {
  OperPathElem elem;
  elem.set_regex(std::move(r));
  return elem;
}

ExtendedPatchSubscription::CompiledRegexPath compileRegexes(
    const std::vector<OperPathElem>& pattern) {
  ExtendedPatchSubscription::CompiledRegexPath compiled;
  compiled.reserve(pattern.size());
  for (const auto& elem : pattern) {
    compiled.push_back(
        elem.regex() ? std::make_shared<const re2::RE2>(*elem.regex())
                     : nullptr);
  }
  return compiled;
}

class TrackerTestSubscription {
 public:
  TrackerTestSubscription() {
    ExtSubPathMap paths;
    ExtendedOperPath path;
    path.path()->emplace_back().set_regex(".*");
    paths[0] = std::move(path);
    auto [generator, subscription] = ExtendedPatchSubscription::create(
        SubscriptionIdentifier("tracker-test"),
        std::move(paths),
        OperProtocol::BINARY,
        std::nullopt,
        heartbeatThread_.getEventBase(),
        std::chrono::milliseconds(100),
        10);
    subscription_ = std::move(subscription);
  }

  ExtendedPatchSubscription* get() const {
    return subscription_.get();
  }

 private:
  folly::ScopedEventBaseThread heartbeatThread_{"SubscriptionHeartbeats"};
  std::unique_ptr<ExtendedPatchSubscription> subscription_;
};

class WildcardPatchCandidateTrackerTest : public ::testing::Test {
 protected:
  void addTestCandidate(
      WildcardPatchCandidateTracker& tracker,
      SubscriptionKey key,
      const std::vector<OperPathElem>& pattern,
      const ExtendedPatchSubscription::CompiledRegexPath& compiledRegexes) {
    tracker.addSeedCandidate(
        testSubscription_.get(), key, &pattern, &compiledRegexes);
  }

 private:
  TrackerTestSubscription testSubscription_;
};

// Collect the keys of fully-matched candidates at the current node.
std::vector<SubscriptionKey> matchedKeys(
    const WildcardPatchCandidateTracker& tracker) {
  std::vector<SubscriptionKey> keys;
  for (const auto& cand : tracker.matchedCandidates()) {
    keys.push_back(cand.key);
  }
  return keys;
}

} // namespace

TEST_F(WildcardPatchCandidateTrackerTest, emptyTracker) {
  WildcardPatchCandidateTracker tracker;
  EXPECT_FALSE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());
}

TEST_F(WildcardPatchCandidateTrackerTest, singlePatternFullMatch) {
  // pattern: a/*/c
  std::vector<OperPathElem> pattern{raw("a"), any(), raw("c")};
  auto compiledRegexes = compileRegexes(pattern);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 7, pattern, compiledRegexes);
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.push("a");
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.push("anything"); // matches the wildcard
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.push("c");
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_EQ(matchedKeys(tracker), (std::vector<SubscriptionKey>{7}));
}

TEST_F(WildcardPatchCandidateTrackerTest, noMatchPrunes) {
  std::vector<OperPathElem> pattern{raw("a"), raw("b")};
  auto compiledRegexes = compileRegexes(pattern);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 1, pattern, compiledRegexes);

  tracker.push("x"); // does not match raw "a"
  EXPECT_FALSE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());
}

TEST_F(WildcardPatchCandidateTrackerTest, multiPatternTracking) {
  // Two patterns sharing the "a" prefix:
  //   key 1: a/*/c
  //   key 2: a/b/*
  std::vector<OperPathElem> p1{raw("a"), any(), raw("c")};
  std::vector<OperPathElem> p2{raw("a"), raw("b"), any()};
  auto c1 = compileRegexes(p1);
  auto c2 = compileRegexes(p2);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 1, p1, c1);
  addTestCandidate(tracker, 2, p2, c2);

  tracker.push("a");
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.push("b"); // matches p1's wildcard and p2's raw "b"
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.push("c"); // p1: raw c matches; p2: wildcard matches -> both matched
  auto keys = matchedKeys(tracker);
  std::sort(keys.begin(), keys.end());
  EXPECT_EQ(keys, (std::vector<SubscriptionKey>{1, 2}));
}

TEST_F(WildcardPatchCandidateTrackerTest, divergentPathsOnlyMatchRelevant) {
  // key 1: a/b/c   key 2: a/x/y
  std::vector<OperPathElem> p1{raw("a"), raw("b"), raw("c")};
  std::vector<OperPathElem> p2{raw("a"), raw("x"), raw("y")};
  auto c1 = compileRegexes(p1);
  auto c2 = compileRegexes(p2);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 1, p1, c1);
  addTestCandidate(tracker, 2, p2, c2);

  tracker.push("a");
  tracker.push("b"); // only p1 survives
  EXPECT_TRUE(tracker.hasActiveCandidates());
  tracker.push("c");
  EXPECT_EQ(matchedKeys(tracker), (std::vector<SubscriptionKey>{1}));
}

TEST_F(WildcardPatchCandidateTrackerTest, regexMatching) {
  // ports/eth\d+/state
  std::vector<OperPathElem> pattern{
      raw("ports"), regex("eth[0-9]+"), raw("state")};
  auto compiledRegexes = compileRegexes(pattern);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 5, pattern, compiledRegexes);

  tracker.push("ports");
  tracker.push("eth42");
  tracker.push("state");
  EXPECT_EQ(matchedKeys(tracker), (std::vector<SubscriptionKey>{5}));

  // A token that doesn't match the regex prunes the candidate.
  WildcardPatchCandidateTracker tracker2;
  addTestCandidate(tracker2, 5, pattern, compiledRegexes);
  tracker2.push("ports");
  tracker2.push("lo0");
  EXPECT_FALSE(tracker2.hasActiveCandidates());
}

TEST_F(WildcardPatchCandidateTrackerTest, pushPopSymmetry) {
  std::vector<OperPathElem> pattern{raw("a"), raw("b")};
  auto compiledRegexes = compileRegexes(pattern);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 1, pattern, compiledRegexes);

  tracker.push("a");
  tracker.push("b");
  EXPECT_EQ(matchedKeys(tracker), (std::vector<SubscriptionKey>{1}));

  tracker.pop(); // back at "a"
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.pop(); // back at root
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  // Re-descend a different branch: no match.
  tracker.push("z");
  EXPECT_FALSE(tracker.hasActiveCandidates());
  tracker.pop();
  EXPECT_TRUE(tracker.hasActiveCandidates());
}

TEST_F(WildcardPatchCandidateTrackerTest, matchedAncestorKeepsSubtreeActive) {
  // pattern a/b matched at depth 2; descending further must stay active so
  // patch building can reach leaves, but must NOT re-report a match.
  std::vector<OperPathElem> pattern{raw("a"), raw("b")};
  auto compiledRegexes = compileRegexes(pattern);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 1, pattern, compiledRegexes);

  tracker.push("a");
  tracker.push("b");
  EXPECT_EQ(matchedKeys(tracker), (std::vector<SubscriptionKey>{1}));

  tracker.push("child"); // below the matched node
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.push("grandchild");
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());

  tracker.pop();
  tracker.pop();
  // Back at the matched node, the match is reported again.
  EXPECT_EQ(matchedKeys(tracker), (std::vector<SubscriptionKey>{1}));
}

TEST_F(WildcardPatchCandidateTrackerTest, zeroLengthPatternMatchesAtRoot) {
  std::vector<OperPathElem> pattern{};
  auto compiledRegexes = compileRegexes(pattern);
  WildcardPatchCandidateTracker tracker;
  addTestCandidate(tracker, 9, pattern, compiledRegexes);
  EXPECT_EQ(matchedKeys(tracker), (std::vector<SubscriptionKey>{9}));
  EXPECT_TRUE(tracker.hasActiveCandidates());

  tracker.push("anything");
  // Subtree stays active (matched ancestor) but no new match reported.
  EXPECT_TRUE(tracker.hasActiveCandidates());
  EXPECT_TRUE(tracker.matchedCandidates().empty());
}

} // namespace facebook::fboss::fsdb
