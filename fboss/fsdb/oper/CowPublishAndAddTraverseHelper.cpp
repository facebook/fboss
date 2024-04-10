// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/CowPublishAndAddTraverseHelper.h"

namespace facebook::fboss::fsdb {

CowPublishAndAddTraverseHelper::CowPublishAndAddTraverseHelper(
    SubscriptionPathStore* root,
    SubscriptionManagerBase* manager)
    : manager_(manager) {
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
  auto* child = lastPathStore->getOrCreateChild(newTok);

  // this assumes currPath has size > 0, which we know because we
  // would have added at least one elem in TraverseHelper::push().
  lastPathStore->processAddedPath(
      *manager_, currPath.begin(), currPath.end() - 1, currPath.end());

  pathStores_.emplace_back(child);
}

void CowPublishAndAddTraverseHelper::onPopImpl(
    std::string&& /* popped */,
    thrift_cow::ThriftTCType /* tc */) {
  pathStores_.pop_back();
}

} // namespace facebook::fboss::fsdb
