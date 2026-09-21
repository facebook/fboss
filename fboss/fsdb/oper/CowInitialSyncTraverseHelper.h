// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/oper/SubscriptionPathStore.h"
#include "fboss/thrift_cow/visitors/TraverseHelper.h"

namespace facebook::fboss::fsdb {

struct CowInitialSyncTraverseHelper
    : thrift_cow::TraverseHelper<CowInitialSyncTraverseHelper> {
  using Base = thrift_cow::TraverseHelper<CowInitialSyncTraverseHelper>;

  using Base::path;
  using Base::shouldShortCircuit;

  CowInitialSyncTraverseHelper(SubscriptionPathStore* root)
      : elementsAlongPath_{root} {}

  bool shouldShortCircuitImpl(thrift_cow::VisitorType visitorType) const {
    const auto* lookup = elementsAlongPath_.back();
    return !lookup || lookup->numSubsRecursive() == 0;
  }

  void onPushImpl(thrift_cow::ThriftTCType tc) {
    const auto& newTok = path().back();
    const auto* lookup = elementsAlongPath_.back();
    CHECK(lookup);
    auto* child = lookup->child(newTok);
    elementsAlongPath_.push_back(child);
  }

  void onPopImpl(std::string&& popped, thrift_cow::ThriftTCType tc) {
    elementsAlongPath_.pop_back();
  }

  SubscriptionPathStore* currentStore() {
    return elementsAlongPath_.back();
  }

 private:
  std::vector<SubscriptionPathStore*> elementsAlongPath_;
};

} // namespace facebook::fboss::fsdb
