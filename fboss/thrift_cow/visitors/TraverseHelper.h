// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

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
  void push(std::string&& tok) {
    currentPath_.emplace_back(std::move(tok));
    onPush();
  }

  void pop() {
    currentPath_.pop_back();
    onPop();
  }

  bool shouldShortCircuit(VisitorType visitorType) const {
    return static_cast<const Impl*>(this)->shouldShortCircuitImpl(visitorType);
  }

  const std::vector<std::string>& path() const {
    // TODO: const ref?
    return currentPath_;
  }

 private:
  void onPush() {
    return static_cast<Impl*>(this)->onPushImpl();
  }
  void onPop() {
    return static_cast<Impl*>(this)->onPopImpl();
  }

  std::vector<std::string> currentPath_;
};

struct SimpleTraverseHelper : TraverseHelper<SimpleTraverseHelper> {
  using Base = TraverseHelper<SimpleTraverseHelper>;

  using Base::shouldShortCircuit;

  bool shouldShortCircuitImpl(VisitorType visitorType) const {
    return false;
  }
  void onPushImpl() {}
  void onPopImpl() {}
};

} // namespace facebook::fboss::thrift_cow
