// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss::thrift_cow {

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

  bool shouldShortCircuit() const {
    return static_cast<const Impl*>(this)->shouldShortCircuitImpl();
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

  bool shouldShortCircuitImpl() const {
    return false;
  }
  void onPushImpl() {}
  void onPopImpl() {}
};

} // namespace facebook::fboss::thrift_cow
