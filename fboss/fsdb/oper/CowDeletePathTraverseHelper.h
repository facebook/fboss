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
#include <optional>

namespace facebook::fboss::fsdb {

struct CowDeletePathTraverseHelper
    : thrift_cow::TraverseHelper<CowDeletePathTraverseHelper> {
  using Base = thrift_cow::TraverseHelper<CowDeletePathTraverseHelper>;

  using Base::path;
  using Base::shouldShortCircuit;

  CowDeletePathTraverseHelper(SubscriptionPathStore* root) : root_(root) {
    pathStores_.emplace_back(root);
  }

  bool shouldShortCircuitImpl(thrift_cow::VisitorType visitorType) const {
    return false;
  }

  void onPushImpl(thrift_cow::ThriftTCType /* tc */) {
    const auto& currPath = path();
    auto lastTok = currPath.back();

    auto* lastPathStore = pathStores_.back();
    // Even if PathStore is not added for currPath (or its parent), traversal
    // will continue. So ensure that sizeof(pathStores_) == pathlen.
    if (lastPathStore) {
      auto* child = lastPathStore->child(lastTok);
      if (child) {
        pathStores_.emplace_back(child);
        return;
      }
    }
    pathStores_.emplace_back(nullptr);
  }

  void onPopImpl(
      std::string&& /* popped */,
      thrift_cow::ThriftTCType /* tc */) {
    pathStores_.pop_back();
  }

  SubscriptionPathStore* currentStore() const {
    return pathStores_.back();
  }

  SubscriptionPathStore* parentStore() const {
    if (pathStores_.size() < 2) {
      return nullptr;
    } else {
      return pathStores_[pathStores_.size() - 2];
    }
  }

 private:
  std::vector<SubscriptionPathStore*> pathStores_;
  SubscriptionPathStore* root_{nullptr};
};

} // namespace facebook::fboss::fsdb
