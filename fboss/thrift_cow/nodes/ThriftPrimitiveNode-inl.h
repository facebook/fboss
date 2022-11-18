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

// TODO: replace with our own ThriftTraverseResult
#include <folly/dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/folly_dynamic.h>
#include <functional>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/visitors/PathVisitor.h"

namespace facebook::fboss::thrift_cow {

template <typename TypeClass, typename TType, bool Immutable>
class ThriftPrimitiveNode {
 public:
  using Self = ThriftPrimitiveNode<TypeClass, TType, Immutable>;
  using CowType = FieldsType;
  using TC = TypeClass;
  using ThriftType = TType;
  using PathIter = typename std::vector<std::string>::const_iterator;

  static constexpr bool immutable = Immutable;

  ThriftPrimitiveNode() = default;
  explicit ThriftPrimitiveNode(ThriftType obj) : obj_(std::move(obj)) {}

  template <typename T = Self>
  auto set(const ThriftType& obj) -> std::enable_if_t<!T::immutable, void> {
    obj_ = obj;
  }

  template <typename T = Self>
  auto set(const ThriftType&) const -> std::enable_if_t<T::immutable, void> {
    throwImmutableException();
  }

  void operator=(const ThriftType& obj) {
    if constexpr (!Immutable) {
      set(obj);
    } else {
      throwImmutableException();
    }
  }

  void operator==(const Self& other) {
    return obj_ == other.obj_;
  }

  void operator!=(const Self& other) {
    return obj_ != other.obj_;
  }

  void operator<(const Self& other) {
    return obj_ < other.obj_;
  }

  ThriftType toThrift() const {
    return obj_;
  }

  template <typename T = Self>
  auto fromThrift(const ThriftType& obj)
      -> std::enable_if_t<!T::immutable, void> {
    set(obj);
  }

  template <typename T = Self>
  auto fromThrift(const ThriftType&) const
      -> std::enable_if_t<T::immutable, void> {
    throwImmutableException();
  }

#ifdef ENABLE_DYNAMIC_APIS

  folly::dynamic toFollyDynamic() const {
    folly::dynamic out;
    apache::thrift::to_dynamic<TypeClass>(
        out, toThrift(), apache::thrift::dynamic_format::JSON_1);
    return out;
  }

  template <typename T = Self>
  auto fromFollyDynamic(const folly::dynamic& value)
      -> std::enable_if_t<!T::immutable, void> {
    ThriftType thrift;
    apache::thrift::from_dynamic<TypeClass>(
        thrift, value, apache::thrift::dynamic_format::JSON_1);
    fromThrift(thrift);
  }

  template <typename T = Self>
  auto fromFollyDynamic(const folly::dynamic&) const
      -> std::enable_if_t<T::immutable, void> {
    throwImmutableException();
  }
#endif

  folly::fbstring encode(fsdb::OperProtocol proto) const {
    return serialize<TypeClass>(proto, toThrift());
  }

  template <typename T = Self>
  auto fromEncoded(fsdb::OperProtocol proto, const folly::fbstring& encoded)
      -> std::enable_if_t<!T::immutable, void> {
    fromThrift(deserialize<TC, TType>(proto, encoded));
  }

  template <typename T = Self>
  auto fromEncoded(
      fsdb::OperProtocol /*proto*/,
      const folly::fbstring& /*encoded*/) const
      -> std::enable_if_t<T::immutable, void> {
    throwImmutableException();
  }

  template <typename T = Self>
  auto remove(const std::string& token)
      -> std::enable_if_t<!T::immutable, bool> {
    throw std::runtime_error(folly::to<std::string>(
        "Cannot remove a child from a primitive node: ", token));
  }

  template <typename T = Self>
  auto remove(const std::string& token) const
      -> std::enable_if_t<T::immutable, bool> {
    throw std::runtime_error(folly::to<std::string>(
        "Cannot remove a child from a primitive node: ", token));
  }

  template <typename T = Self>
  auto modify(const std::string&) -> std::enable_if_t<!T::immutable, void> {}

  template <typename T = Self>
  auto modify(const std::string&) const
      -> std::enable_if_t<T::immutable, void> {
    throwImmutableException();
  }

  ThriftType& ref() {
    return obj_;
  }

  const ThriftType& ref() const {
    return obj_;
  }

  const ThriftType& cref() const {
    return obj_;
  }

  std::size_t hash() const {
    return std::hash<ThriftType>()(obj_);
  }

  /*
   * Visitors by string path
   */

  template <typename Func>
  inline ThriftTraverseResult
  visitPath(PathIter begin, PathIter end, Func&& f) {
    return PathVisitor<TypeClass>::visit(
        *this, begin, end, PathVisitMode::LEAF, std::forward<Func>(f));
  }

  template <typename Func>
  inline ThriftTraverseResult visitPath(PathIter begin, PathIter end, Func&& f)
      const {
    return PathVisitor<TypeClass>::visit(
        *this, begin, end, PathVisitMode::LEAF, std::forward<Func>(f));
  }

  template <typename Func>
  inline ThriftTraverseResult cvisitPath(PathIter begin, PathIter end, Func&& f)
      const {
    return PathVisitor<TypeClass>::visit(
        *this, begin, end, PathVisitMode::LEAF, std::forward<Func>(f));
  }

 private:
  [[noreturn]] void throwImmutableException() const {
    throw std::runtime_error("Cannot mutate an immutable primitive node");
  }

  ThriftType obj_{};
};

template <typename TC, typename TType>
using ImmutableThriftPrimitiveNode = ThriftPrimitiveNode<TC, TType, true>;

template <typename PrimitiveNode>
bool operator==(
    const PrimitiveNode& node,
    const typename PrimitiveNode::ThriftType& data) {
  return node.cref() == data;
}

template <typename PrimitiveNode>
bool operator==(
    const typename PrimitiveNode::ThriftType& data,
    const PrimitiveNode& node) {
  return node.cref() == data;
}

template <typename TC, typename TType, bool Immutable>
bool operator==(
    const ThriftPrimitiveNode<TC, TType, Immutable>& nodeA,
    const ThriftPrimitiveNode<TC, TType, Immutable>& nodeB) {
  return nodeA.cref() == nodeB.cref();
}

template <typename TC, typename TType, bool Immutable>
bool operator==(
    const std::optional<ThriftPrimitiveNode<TC, TType, Immutable>>& nodeA,
    const std::optional<ThriftPrimitiveNode<TC, TType, Immutable>>& nodeB) {
  if (nodeA.has_value() && nodeB.has_value()) {
    return *nodeA == *nodeB;
  } else if (nodeA.has_value() || nodeB.has_value()) {
    return false;
  }
  return true;
}

template <typename PrimitiveNode>
bool operator!=(
    const PrimitiveNode& node,
    const typename PrimitiveNode::ThriftType& data) {
  return node.cref() != data;
}

template <typename PrimitiveNode>
bool operator!=(
    const typename PrimitiveNode::ThriftType& data,
    const PrimitiveNode& node) {
  return node.cref() != data;
}

template <typename TC, typename TType, bool Immutable>
bool operator!=(
    const ThriftPrimitiveNode<TC, TType, Immutable>& nodeA,
    const ThriftPrimitiveNode<TC, TType, Immutable>& nodeB) {
  return nodeA.cref() != nodeB.cref();
}

template <typename TC, typename TType, bool Immutable>
bool operator!=(
    const std::optional<ThriftPrimitiveNode<TC, TType, Immutable>>& nodeA,
    const std::optional<ThriftPrimitiveNode<TC, TType, Immutable>>& nodeB) {
  return !(nodeA == nodeB);
}

template <typename TC, typename TType, bool Immutable>
bool operator<(
    const ThriftPrimitiveNode<TC, TType, Immutable>& nodeA,
    const ThriftPrimitiveNode<TC, TType, Immutable>& nodeB) {
  return nodeA.cref() < nodeB.cref();
}

template <typename TC, typename TType, bool Immutable>
bool operator<(
    const std::optional<ThriftPrimitiveNode<TC, TType, Immutable>>& nodeA,
    const std::optional<ThriftPrimitiveNode<TC, TType, Immutable>>& nodeB) {
  return *nodeA < *nodeB;
}

} // namespace facebook::fboss::thrift_cow

namespace std {

template <typename TC, typename TType, bool Immutable>
struct hash<
    facebook::fboss::thrift_cow::ThriftPrimitiveNode<TC, TType, Immutable>> {
  using Key =
      facebook::fboss::thrift_cow::ThriftPrimitiveNode<TC, TType, Immutable>;

  std::size_t operator()(const Key& key) {
    return key.hash();
  }
};

} // namespace std
