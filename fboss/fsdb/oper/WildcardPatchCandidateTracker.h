// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/logging/xlog.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "fboss/fsdb/oper/Subscription.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"
#include "fboss/thrift_cow/visitors/ExtendedPathMatcher.h"

namespace facebook::fboss::fsdb {

// A single candidate wildcard PATCH subscription being tracked as the delta
// walk descends. `elemIdx` is how many leading pattern elements have already
// been matched by the concrete path tokens seen so far. A candidate is fully
// matched when `elemIdx == pattern->size()`.
struct WildcardPatchCandidate {
  ExtendedPatchSubscription* sub{nullptr};
  SubscriptionKey key{0};
  size_t elemIdx{0};
  const std::vector<OperPathElem>* pattern{nullptr};
  const ExtendedPatchSubscription::CompiledRegexPath* compiledRegexes{nullptr};
};

// Tracks, along a single root->node traversal path, which registered wildcard
// PATCH extended subscriptions prefix-match the concrete path currently being
// visited. It is the dynamic analogue of the resolved-subscription path-store
// lookup: instead of pre-expanding every matching concrete path into a
// PatchSubscription object in `lookup_`, candidates are advanced token-by-token
// during the serve-cycle delta walk.
//
// Usage mirrors the traverse helper push/pop lifecycle:
//   tracker.seed(...);              // before traversal
//   tracker.push(token);            // on each onPush
//   ... tracker.matchedCandidates() // serve fully-matched candidates here
//   ... tracker.hasActiveCandidates() // drives short-circuit decision
//   tracker.pop();                  // on each onPop
//
// A fully-matched candidate keeps the entire subtree below the match "active"
// (hasActiveCandidates() stays true) so that CHILDREN_FIRST patch building can
// visit and compress the leaves beneath the subscription point, exactly as the
// resolved-subscription path keeps traversing via its sticky `hasAncestorSubs`.
class WildcardPatchCandidateTracker {
 public:
  using ExtendedSubscriptionMap =
      std::unordered_map<std::string, std::shared_ptr<ExtendedSubscription>>;

  WildcardPatchCandidateTracker() {
    // Base level corresponds to the root (empty concrete path).
    levels_.emplace_back();
  }

  // Add a single seed candidate at the root level. Must be called before any
  // push(). This is the insertion primitive seed() is built on; it is public so
  // unit tests can drive the state machine directly.
  void addSeedCandidate(
      ExtendedPatchSubscription* sub,
      SubscriptionKey key,
      const std::vector<OperPathElem>* pattern,
      const ExtendedPatchSubscription::CompiledRegexPath* compiledRegexes) {
    CHECK_EQ(levels_.size(), 1)
        << "seed candidates must be added before traversal begins";
    CHECK(sub);
    CHECK(pattern);
    CHECK(compiledRegexes);
    // Parity with the pattern, and a compiled regex for every regex element,
    // are established where the cache is built, in
    // ExtendedPatchSubscription::compileRegexes. This is an invariant check
    // rather than input validation on the per-serve-cycle path.
    CHECK_EQ(pattern->size(), compiledRegexes->size());
    WildcardPatchCandidate cand{sub, key, 0, pattern, compiledRegexes};
    auto& base = levels_.front();
    if (cand.elemIdx == pattern->size()) {
      // Zero-length pattern: matched at the root.
      base.matched.push_back(cand);
    } else {
      base.inProgress.push_back(cand);
    }
  }

  // Seed candidates from the store's extended subscriptions. The caller must
  // hold the SubscriptionStore write lock until traversal and tracker lifetime
  // complete because candidates borrow subscription-owned path/cache storage.
  // Only subscriptions shouldDynamicallyResolve() admits are seeded, i.e.
  // wildcard-bearing PATCH subscriptions, and only once they are live and past
  // their own initial sync. Returns the number of (key, path) candidates
  // seeded.
  size_t seed(
      const ExtendedSubscriptionMap& extendedSubs,
      const SubscriptionMetadataServer& metadataServer) {
    size_t seeded = 0;
    for (const auto& [_name, sub] : extendedSubs) {
      if (!sub || !shouldDynamicallyResolve(*sub) || sub->shouldPrune() ||
          sub->getInitialSyncCompletedAt() == 0 ||
          !metadataServer.ready(sub->publisherTreeRoot())) {
        continue;
      }
      // shouldDynamicallyResolve() already established PATCH type.
      auto* patchSub = static_cast<ExtendedPatchSubscription*>(sub.get());
      for (const auto& [key, extPath] : patchSub->paths()) {
        if (patchSub->isInitialSyncPending(key)) {
          continue;
        }
        addSeedCandidate(
            patchSub,
            key,
            &extPath.path().value(),
            &patchSub->compiledRegexesAt(key));
        ++seeded;
      }
    }
    return seeded;
  }

  // Advance to the child node identified by `token`.
  void push(const std::string& token) {
    const auto& cur = levels_.back();
    LevelState next;
    // Sticky: once an ancestor (or this node) fully matched, the whole subtree
    // stays active so patch building can reach the leaves.
    next.hasMatchedAncestor = cur.hasMatchedAncestor || !cur.matched.empty();
    for (const auto& cand : cur.inProgress) {
      if (thrift_cow::matchesStrToken(
              token,
              cand.pattern->at(cand.elemIdx),
              cand.compiledRegexes->at(cand.elemIdx).get())) {
        WildcardPatchCandidate advanced{
            cand.sub,
            cand.key,
            cand.elemIdx + 1,
            cand.pattern,
            cand.compiledRegexes};
        if (advanced.elemIdx == advanced.pattern->size()) {
          next.matched.push_back(advanced);
        } else {
          next.inProgress.push_back(advanced);
        }
      }
    }
    levels_.emplace_back(std::move(next));
  }

  void pop() {
    CHECK_GT(levels_.size(), 1) << "pop() without a matching push()";
    levels_.pop_back();
  }

  // True if traversal should continue below the current node, because either an
  // in-progress candidate could still match deeper, a candidate matched exactly
  // here, or a matched ancestor requires the subtree to be walked.
  bool hasActiveCandidates() const {
    const auto& cur = levels_.back();
    return !cur.inProgress.empty() || !cur.matched.empty() ||
        cur.hasMatchedAncestor;
  }

  // Candidates whose pattern is fully matched at exactly the current node.
  // These should be served a patch built from the current traversal node.
  const std::vector<WildcardPatchCandidate>& matchedCandidates() const {
    return levels_.back().matched;
  }

 private:
  struct LevelState {
    std::vector<WildcardPatchCandidate> inProgress;
    std::vector<WildcardPatchCandidate> matched;
    bool hasMatchedAncestor{false};
  };

  std::vector<LevelState> levels_;
};

} // namespace facebook::fboss::fsdb
