// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/PatchApplier.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::thrift_cow {

void PatchTraverser::push(int tok) {
  push(folly::to<std::string>(tok));
}

void PatchTraverser::push(std::string tok) {
  curPath_.push_back(std::move(tok));
}

PatchApplyResult PatchTraverser::traverseResult(
    const PatchApplyResult& result) {
  if (result != PatchApplyResult::OK) {
    curResult_ = result;
  }
  if (curResult_ != PatchApplyResult::OK && XLOG_IS_ON(DBG4)) {
    XLOG(DBG4) << "Error " << apache::thrift::util::enumNameSafe(result)
               << " at path " << folly::join("/", curPath_);
  }
  return curResult_;
}
void PatchTraverser::pop() {
  curPath_.pop_back();
}

} // namespace facebook::fboss::thrift_cow
