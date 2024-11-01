// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/CowPublishAndAddTraverseHelper.h"

namespace facebook::fboss::fsdb {

CowPublishAndAddTraverseHelper::CowPublishAndAddTraverseHelper(
    SubscriptionPathStore* root,
    SubscriptionStore* store)
    : store_(store) {
  pathStores_.emplace_back(root);
}

bool CowPublishAndAddTraverseHelper::shouldShortCircuitImpl(
    thrift_cow::VisitorType visitorType) const {
  return false;
}

void CowPublishAndAddTraverseHelper::onPushImpl(
    thrift_cow::ThriftTCType /* tc */) {
  const auto& currPath = path();
  const auto& newTok = currPath.back();
  auto* lastPathStore = pathStores_.back();

  SubscriptionPathStore* child{nullptr};
  if (lastPathStore) {
    if (FLAGS_lazyPathStoreCreation) {
      child = lastPathStore->child(newTok);
    } else {
      child =
          lastPathStore->getOrCreateChild(newTok, store_->getPathStoreStats());
    }
    // this assumes currPath has size > 0, which we know because we
    // would have added at least one elem in TraverseHelper::push().
    lastPathStore->processAddedPath(
        *store_, currPath.begin(), currPath.end() - 1, currPath.end());
  }
  // on push, always add the child to the pathStores_ even if lastPathStore
  // is null, so that pathStores_.size() is always equal to pathlen()
  pathStores_.emplace_back(child);
}

void CowPublishAndAddTraverseHelper::onPopImpl(
    std::string&& /* popped */,
    thrift_cow::ThriftTCType /* tc */) {
  pathStores_.pop_back();
}

} // namespace facebook::fboss::fsdb
