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
#include <fboss/thrift_cow/visitors/TraverseHelper.h>

namespace facebook::fboss::fsdb {

template <typename Manager>
struct CowPublishAndAddTraverseHelper
    : thrift_cow::TraverseHelper<CowPublishAndAddTraverseHelper<Manager>> {
  using Base =
      thrift_cow::TraverseHelper<CowPublishAndAddTraverseHelper<Manager>>;

  using Base::path;
  using Base::shouldShortCircuit;

  CowPublishAndAddTraverseHelper(SubscriptionPathStore* root, Manager* manager)
      : manager_(manager) {
    pathStores_.emplace_back(root);
  }

  bool shouldShortCircuitImpl(thrift_cow::VisitorType visitorType) const {
    return false;
  }

  void onPushImpl(thrift_cow::ThriftTCType /* tc */) {
    const auto& currPath = path();
    const auto& newTok = currPath.back();
    auto* lastPathStore = pathStores_.back();
    auto* child = lastPathStore->getOrCreateChild(newTok);

    // this assumes currPath has size > 0, which we know because we
    // would have added at least one elem in TraverseHelper::push().
    lastPathStore->processAddedPath(
        *manager_, currPath.begin(), currPath.end() - 1, currPath.end());

    pathStores_.emplace_back(child);
  }

  void onPopImpl(
      std::string&& /* popped */,
      thrift_cow::ThriftTCType /* tc */) {
    pathStores_.pop_back();
  }

 private:
  std::vector<SubscriptionPathStore*> pathStores_;
  Manager* manager_{nullptr};
};

} // namespace facebook::fboss::fsdb
