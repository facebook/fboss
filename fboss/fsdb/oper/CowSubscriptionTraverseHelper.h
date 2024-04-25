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
      return !lastElem.hasAncestorSubs && !hasDescendantSubs;
    } else if (visitorType == thrift_cow::VisitorType::RECURSE) {
      // when we are in a recurse visitor, we only need to keep
      // recursing if we have any children subscriptions
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
  }

  void onPopImpl(std::string&& popped, thrift_cow::ThriftTCType tc) {
    // TODO: for every patch subscription along path, call bubbleUpFromSubNode
    elementsAlongPath_.pop_back();
    if (patchBuilder_) {
      patchBuilder_->onPathPop(std::move(popped), tc);
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

 private:
  std::vector<CowSubscriptionTraverseHelperElem> elementsAlongPath_;
  std::optional<thrift_cow::PatchNodeBuilder>& patchBuilder_;
};

} // namespace facebook::fboss::fsdb
