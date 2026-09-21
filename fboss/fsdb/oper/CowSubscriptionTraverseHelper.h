/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fboss/fsdb/oper/SubscriptionPathStore.h>
#include <fboss/fsdb/oper/WildcardPatchCandidateTracker.h>
#include <fboss/thrift_cow/visitors/PatchBuilder.h>
#include <fboss/thrift_cow/visitors/TraverseHelper.h>

namespace facebook::fboss::fsdb {

struct CowSubscriptionTraverseHelperElem {
  CowSubscriptionTraverseHelperElem(
      const SubscriptionPathStore* lookup,
      bool hasAncestorSubs)
      : lookup(lookup), hasAncestorSubs(hasAncestorSubs) {}

  const SubscriptionPathStore* lookup{nullptr};
  const bool hasAncestorSubs{false};
};

struct CowSubscriptionTraverseHelper
    : thrift_cow::TraverseHelper<CowSubscriptionTraverseHelper> {
  using Base = thrift_cow::TraverseHelper<CowSubscriptionTraverseHelper>;

  using Base::path;
  using Base::shouldShortCircuit;

  CowSubscriptionTraverseHelper(
      const SubscriptionPathStore* root,
      std::optional<thrift_cow::PatchNodeBuilder>& patchBuilder)
      : patchBuilder_(patchBuilder) {
    bool hasRootSubs = root->numSubs() > 0;
    elementsAlongPath_.emplace_back(root, hasRootSubs);
  }

  bool shouldShortCircuitImpl(thrift_cow::VisitorType visitorType) const {
    const auto& lastElem = elementsAlongPath_.back();

    auto* lookup = lastElem.lookup;
    bool hasDescendantSubs = lookup && lookup->numSubsRecursive();
    if (visitorType == thrift_cow::VisitorType::DELTA) {
      // when in the delta phase, we need to keep recursing if we have
      // any ancestor subscriptions active, as we need to fill out delta
      // or confirm that a path subscription should be served.
      //
      // Dynamic wildcard PATCH subscriptions are not present in lookup_, so
      // we must also keep recursing while any wildcard candidate still
      // prefix-matches this subtree (or a matched ancestor needs the subtree
      // walked for patch building).
      if (wildcardTracker_ && wildcardTracker_->hasActiveCandidates()) {
        return false;
      }
      return !lastElem.hasAncestorSubs && !hasDescendantSubs;
    } else if (visitorType == thrift_cow::VisitorType::RECURSE) {
      // RecurseVisitor builds patches below nodes selected by DeltaVisitor.
      // Dynamic wildcard subscriptions are absent from lookup_, so their
      // active candidates must keep this phase alive too.
      if (wildcardTracker_ && wildcardTracker_->hasActiveCandidates()) {
        return false;
      }
      return !hasDescendantSubs;
    } else {
      throw std::runtime_error("Unexpected visitor type");
    }
  }

  void onPushImpl(thrift_cow::ThriftTCType tc) {
    const auto& newTok = path().back();
    const auto& lastElem = elementsAlongPath_.back();
    auto* child = (lastElem.lookup) ? lastElem.lookup->child(newTok) : nullptr;
    bool hasAncestorSubs =
        lastElem.hasAncestorSubs || (child && child->numSubs());
    elementsAlongPath_.emplace_back(child, hasAncestorSubs);
    if (patchBuilder_) {
      patchBuilder_->onPathPush(newTok, tc);
    }
    if (wildcardTracker_) {
      wildcardTracker_->push(newTok);
    }
  }

  void onPopImpl(std::string&& popped, thrift_cow::ThriftTCType tc) {
    // TODO: for every patch subscription along path, call bubbleUpFromSubNode
    elementsAlongPath_.pop_back();
    if (patchBuilder_) {
      patchBuilder_->onPathPop(std::move(popped), tc);
    }
    if (wildcardTracker_) {
      wildcardTracker_->pop();
    }
  }

  const SubscriptionPathStore* currentStore() const {
    const auto& lastElem = elementsAlongPath_.back();
    return lastElem.lookup;
  }

  const std::vector<CowSubscriptionTraverseHelperElem>& elementsAlongPath()
      const {
    return elementsAlongPath_;
  }

  std::optional<thrift_cow::PatchNodeBuilder>& patchBuilder() const {
    return patchBuilder_;
  }

  // Attach a wildcard PATCH candidate tracker. Must be seeded and set before
  // traversal begins (its base level corresponds to the root node, matching
  // the root element pushed in the constructor). Ownership stays with the
  // caller; the tracker must outlive this helper.
  void setWildcardTracker(WildcardPatchCandidateTracker* tracker) {
    wildcardTracker_ = tracker;
  }

  WildcardPatchCandidateTracker* wildcardTracker() const {
    return wildcardTracker_;
  }

 private:
  std::vector<CowSubscriptionTraverseHelperElem> elementsAlongPath_;
  std::optional<thrift_cow::PatchNodeBuilder>& patchBuilder_;
  WildcardPatchCandidateTracker* wildcardTracker_{nullptr};
};

} // namespace facebook::fboss::fsdb
