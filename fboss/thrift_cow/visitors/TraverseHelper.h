// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <thrift/lib/cpp2/TypeClass.h>
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

// Simplified enum representing main thrift type classes we care about We
// convert to this in template params whenever possible to reduce total number
// of types generated. Thrift TC containers (map, list, set) are templated on
// key and val types so they will generate more types if used
enum class ThriftSimpleTC {
  PRIMITIVE,
  STRUCTURE,
  VARIANT,
  MAP,
  SET,
  LIST,
};

template <typename TC>
struct TCtoSimpleTC;

template <>
struct TCtoSimpleTC<apache::thrift::type_class::structure> {
  static constexpr auto type = ThriftSimpleTC::STRUCTURE;
};

template <>
struct TCtoSimpleTC<apache::thrift::type_class::variant> {
  static constexpr auto type = ThriftSimpleTC::VARIANT;
};

template <typename ValTC>
struct TCtoSimpleTC<apache::thrift::type_class::set<ValTC>> {
  static constexpr auto type = ThriftSimpleTC::SET;
};

template <typename KeyTC, typename ValTC>
struct TCtoSimpleTC<apache::thrift::type_class::map<KeyTC, ValTC>> {
  static constexpr auto type = ThriftSimpleTC::MAP;
};

template <typename ValTC>
struct TCtoSimpleTC<apache::thrift::type_class::list<ValTC>> {
  static constexpr auto type = ThriftSimpleTC::LIST;
};

template <typename TC>
struct TCtoSimpleTC {
  static constexpr auto type = ThriftSimpleTC::PRIMITIVE;
};

template <typename TC>
static constexpr auto SimpleTC = TCtoSimpleTC<TC>::type;

template <typename Impl>
class TraverseHelper {
 public:
  template <ThriftSimpleTC SimpleTC>
  void push(std::string&& tok) {
    currentPath_.emplace_back(std::move(tok));
    onPush<SimpleTC>();
  }

  template <ThriftSimpleTC SimpleTC>
  void pop() {
    std::string&& tok = std::move(currentPath_.back());
    currentPath_.pop_back();
    onPop<SimpleTC>(std::forward<std::string>(tok));
  }

  bool shouldShortCircuit(VisitorType visitorType) const {
    return static_cast<const Impl*>(this)->shouldShortCircuitImpl(visitorType);
  }

  const std::vector<std::string>& path() const {
    // TODO: const ref?
    return currentPath_;
  }

 private:
  template <ThriftSimpleTC SimpleTC>
  void onPush() {
    return static_cast<Impl*>(this)->template onPushImpl<SimpleTC>();
  }
  template <ThriftSimpleTC SimpleTC>
  void onPop(std::string&& popped) {
    return static_cast<Impl*>(this)->template onPopImpl<SimpleTC>(
        std::forward<std::string>(popped));
  }

  std::vector<std::string> currentPath_;
};

struct SimpleTraverseHelper : TraverseHelper<SimpleTraverseHelper> {
  using Base = TraverseHelper<SimpleTraverseHelper>;

  using Base::shouldShortCircuit;

  bool shouldShortCircuitImpl(VisitorType visitorType) const {
    return false;
  }
  template <ThriftSimpleTC SimpleTC>
  void onPushImpl() {}
  template <ThriftSimpleTC SimpleTC>
  void onPopImpl(std::string&& popped) {}
};

} // namespace facebook::fboss::thrift_cow
