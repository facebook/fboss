// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_cow/visitors/ThriftTCType.h>

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <string>
#include <vector>

namespace facebook::fboss::thrift_cow {

enum class VisitorType {
  /* It can be useful to know the visitor type when making decisions
   * in a traverse helper as some use cases compose visitors together
   * and the behavior may change depending on which visitor is
   * running.
   */
  DELTA,

  RECURSE
};

template <typename Impl>
class TraverseHelper {
 public:
  void push(std::string&& tok, ThriftTCType tc) {
    currentPath_.emplace_back(std::move(tok));
    if (XLOG_IS_ON(DBG6)) {
      XLOG(DBG6) << "Traversing up to path " << folly::join("/", currentPath_);
    }
    onPush(tc);
  }

  void pop(ThriftTCType tc) {
    std::string tok = std::move(currentPath_.back());
    currentPath_.pop_back();
    if (XLOG_IS_ON(DBG6)) {
      XLOG(DBG6) << "Traversing down to path "
                 << folly::join("/", currentPath_);
    }
    onPop(std::move(tok), tc);
  }

  bool shouldShortCircuit(VisitorType visitorType) const {
    return static_cast<const Impl*>(this)->shouldShortCircuitImpl(visitorType);
  }

  const std::vector<std::string>& path() const {
    // TODO: const ref?
    return currentPath_;
  }

 private:
  void onPush(ThriftTCType tc) {
    return static_cast<Impl*>(this)->onPushImpl(tc);
  }
  void onPop(std::string&& popped, ThriftTCType tc) {
    return static_cast<Impl*>(this)->onPopImpl(
        std::forward<std::string>(popped), tc);
  }

  std::vector<std::string> currentPath_;
};

struct SimpleTraverseHelper : TraverseHelper<SimpleTraverseHelper> {
  using Base = TraverseHelper<SimpleTraverseHelper>;

  using Base::shouldShortCircuit;

  bool shouldShortCircuitImpl(VisitorType visitorType) const {
    return false;
  }
  void onPushImpl(ThriftTCType tc) {}
  void onPopImpl(std::string&& popped, ThriftTCType tc) {}
};

} // namespace facebook::fboss::thrift_cow
