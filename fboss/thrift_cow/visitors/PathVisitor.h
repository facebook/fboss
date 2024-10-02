// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <type_traits>
#include <utility>
#include "folly/Conv.h"
#include "folly/logging/xlog.h"

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/nodes/NodeUtils.h>
#include <fboss/thrift_cow/nodes/Serializer.h>
#include <fboss/thrift_cow/visitors/VisitorUtils.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

struct HybridNodeType;

namespace pv_detail {
using PathIter = typename std::vector<std::string>::const_iterator;
}

// Base class for "untyped" operators. This base class should be the prefered
// way to use visitors. Operations should subclass this and override the virtual
// methods but pass a pointer to the base class to avoid extra unique
// instantiations
class BasePathVisitorOperator {
 public:
  virtual ~BasePathVisitorOperator() = default;

  template <typename Node>
  inline void
  visitTyped(Node& node, pv_detail::PathIter begin, pv_detail::PathIter end) {
    if constexpr (std::is_const_v<Node>) {
      cvisit(node, begin, end);
      cvisit(node);
    } else {
      visit(node, begin, end);
      visit(node);
    }
  }

 protected:
  virtual void visit(
      Serializable& /* node */,
      pv_detail::PathIter /* begin */,
      pv_detail::PathIter /* end */) {}

  virtual void visit(Serializable& node) {}

  virtual void cvisit(
      const Serializable& /* node */,
      pv_detail::PathIter /* begin */,
      pv_detail::PathIter /* end */) {}

  virtual void cvisit(const Serializable& node) {}
};

struct GetEncodedPathVisitorOperator : public BasePathVisitorOperator {
  explicit GetEncodedPathVisitorOperator(fsdb::OperProtocol protocol)
      : protocol_(protocol) {}

  std::optional<folly::fbstring> val{std::nullopt};

 protected:
  void cvisit(const Serializable& node) override {
    val = node.encode(protocol_);
  }

  void visit(Serializable& node) override {
    val = node.encode(protocol_);
  }

 private:
  fsdb::OperProtocol protocol_;
};

struct SetEncodedPathVisitorOperator : public BasePathVisitorOperator {
  SetEncodedPathVisitorOperator(
      fsdb::OperProtocol protocol,
      const folly::fbstring& val)
      : protocol_(protocol), val_(val) {}

 protected:
  void visit(facebook::fboss::thrift_cow::Serializable& node) override {
    node.fromEncoded(protocol_, val_);
  }

 private:
  fsdb::OperProtocol protocol_;
  const folly::fbstring& val_{};
};

/*
 * This visitor takes a path object and a thrift type and is able to
 * traverse the object and then run the visitor function against the
 * final type.
 */

// TODO: enable more details
enum class ThriftTraverseResult {
  OK,
  NON_EXISTENT_NODE,
  INVALID_ARRAY_INDEX,
  INVALID_MAP_KEY,
  INVALID_STRUCT_MEMBER,
  INVALID_VARIANT_MEMBER,
  INCORRECT_VARIANT_MEMBER,
  VISITOR_EXCEPTION,
  INVALID_SET_MEMBER,
};

/*
 * invokeVisitorFnHelper allows us to support two different visitor signatures:
 *
 * 1. f(node)
 * 2. f(node, begin, end)
 *
 * This allows a visitor to use the remaining path tokens if needed.
 */

struct NodeType;
struct FieldsType;

enum class PathVisitMode {
  /*
   * In this mode, we visit every node along a given path.
   */
  FULL,

  /*
   * In this mode, we only visit the end of the path.
   */
  LEAF
};

namespace pv_detail {

template <typename TC>
struct PathVisitorImpl;

using PathIter = typename std::vector<std::string>::const_iterator;

// Version of an operator that forwards operation to a lambda. This should be
// used as sparingly as possible, only when node types are needed, because
// every call with this templated operator is a unique instantiation of the
// entire template tree
template <typename Func>
struct LambdaPathVisitorOperator {
  explicit LambdaPathVisitorOperator(Func&& f) : f_(std::forward<Func>(f)) {}

  template <typename Node>
  inline auto
  visitTyped(Node& node, pv_detail::PathIter begin, pv_detail::PathIter end)
      -> std::invoke_result_t<
          Func,
          Node&,
          pv_detail::PathIter,
          pv_detail::PathIter> {
    return f_(node, begin, end);
  }

  template <typename Node>
  inline auto visitTyped(
      Node& node,
      pv_detail::PathIter /* begin */,
      pv_detail::PathIter /* end */) -> std::invoke_result_t<Func, Node&> {
    return f_(node);
  }

 private:
  Func f_;
};

template <typename TC, typename Node, typename Op>
ThriftTraverseResult visitNode(
    Node& node,
    PathIter begin,
    PathIter end,
    const PathVisitMode& mode,
    Op& op)
    // only enable for Node types
  requires(std::is_same_v<typename Node::CowType, NodeType>)
{
  if (mode == PathVisitMode::FULL || begin == end) {
    try {
      op.visitTyped(node, begin, end);
      if (begin == end) {
        return ThriftTraverseResult::OK;
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
  }

  if constexpr (std::is_const_v<Node>) {
    return PathVisitorImpl<TC>::visit(*node.getFields(), begin, end, mode, op);
  } else {
    return PathVisitorImpl<TC>::visit(
        *node.writableFields(), begin, end, mode, op);
  }
}

/**
 * Set
 */
template <typename ValueTypeClass>
struct PathVisitorImpl<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;

  template <typename Node, typename Op>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, begin, end, mode, op);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for hybrid nodes
    return ThriftTraverseResult::VISITOR_EXCEPTION;
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using ValueTType = typename Fields::ValueTType;

    // Get value
    auto token = *begin++;

    if (auto value = tryParseKey<ValueTType, ValueTypeClass>(token)) {
      if (auto it = fields.find(*value); it != fields.end()) {
        // Recurse further
        return PathVisitorImpl<ValueTypeClass>::visit(
            **it, begin, end, mode, op);
      } else {
        return ThriftTraverseResult::NON_EXISTENT_NODE;
      }
    }

    // if we get here, we must have a malformed value
    return ThriftTraverseResult::INVALID_SET_MEMBER;
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct PathVisitorImpl<apache::thrift::type_class::list<ValueTypeClass>> {
  using TC = apache::thrift::type_class::list<ValueTypeClass>;

  template <typename Node, typename Op>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, begin, end, mode, op);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for hybrid nodes
    return ThriftTraverseResult::VISITOR_EXCEPTION;
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    // Parse and pop token. Also check for index bound
    auto index = folly::tryTo<size_t>(*begin++);
    if (index.hasError() || index.value() >= fields.size()) {
      return ThriftTraverseResult::INVALID_ARRAY_INDEX;
    }

    // Recurse at a given index
    if constexpr (std::is_const_v<Fields>) {
      const auto& next = *fields.ref(index.value());
      return PathVisitorImpl<ValueTypeClass>::visit(next, begin, end, mode, op);
    } else {
      return PathVisitorImpl<ValueTypeClass>::visit(
          *fields.ref(index.value()), begin, end, mode, op);
    }
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct PathVisitorImpl<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;

  template <typename Node, typename Op>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, begin, end, mode, op);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    try {
      if (begin == end) {
        op.visitTyped(node, begin, end);
      } else {
        // TODO: handle traversing thrift objects in hybrid nodes
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
      return ThriftTraverseResult::OK;
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using key_type = typename Fields::key_type;

    // Get key
    auto token = *begin++;

    if (auto key = tryParseKey<key_type, KeyTypeClass>(token)) {
      if (fields.find(key.value()) != fields.end()) {
        // Recurse further
        if constexpr (std::is_const_v<Fields>) {
          const auto& next = *fields.ref(key.value());
          return PathVisitorImpl<MappedTypeClass>::visit(
              next, begin, end, mode, op);
        } else {
          return PathVisitorImpl<MappedTypeClass>::visit(
              *fields.ref(key.value()), begin, end, mode, op);
        }
      } else {
        return ThriftTraverseResult::NON_EXISTENT_NODE;
      }
    }

    return ThriftTraverseResult::INVALID_MAP_KEY;
  }
};

/**
 * Variant
 */
template <>
struct PathVisitorImpl<apache::thrift::type_class::variant> {
  using TC = apache::thrift::type_class::variant;

  template <typename Node, typename Op>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, begin, end, mode, op);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for hybrid nodes
    return ThriftTraverseResult::VISITOR_EXCEPTION;
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using MemberTypes = typename Fields::MemberTypes;

    auto result = ThriftTraverseResult::INVALID_VARIANT_MEMBER;

    // Get key
    auto key = *begin++;

    visitMember<MemberTypes>(key, [&](auto tag) {
      using descriptor = typename decltype(fatal::tag_type(tag))::member;
      using name = typename descriptor::metadata::name;
      using tc = typename descriptor::metadata::type_class;

      if (fields.type() != descriptor::metadata::id::value) {
        result = ThriftTraverseResult::INCORRECT_VARIANT_MEMBER;
        return;
      }

      auto& child = fields.template ref<name>();

      // ensure we propagate constness, since children will have type
      // const shared_ptr<T>, not shared_ptr<const T>.
      if constexpr (std::is_const_v<Fields>) {
        const auto& next = *child;
        result = PathVisitorImpl<tc>::visit(next, begin, end, mode, op);
      } else {
        result = PathVisitorImpl<tc>::visit(*child, begin, end, mode, op);
      }
    });

    return result;
  }
};

/**
 * Structure
 */
template <>
struct PathVisitorImpl<apache::thrift::type_class::structure> {
  using TC = apache::thrift::type_class::structure;

  template <typename Node, typename Op>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, begin, end, mode, op);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for hybrid nodes
    return ThriftTraverseResult::VISITOR_EXCEPTION;
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using Members = typename Fields::Members;

    // Get key
    auto key = *begin++;

    ThriftTraverseResult result = ThriftTraverseResult::INVALID_STRUCT_MEMBER;

    visitMember<Members>(key, [&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using name = typename member::name;
      using tc = typename member::type_class;

      // Recurse further
      auto& child = fields.template ref<name>();

      if (!child) {
        // child is unset, cannot traverse through missing optional child
        result = ThriftTraverseResult::NON_EXISTENT_NODE;
        return;
      }

      // ensure we propagate constness, since children will have type
      // const shared_ptr<T>, not shared_ptr<const T>.
      if constexpr (std::is_const_v<Fields>) {
        const auto& next = *child;
        result = PathVisitorImpl<tc>::visit(next, begin, end, mode, op);
      } else {
        result = PathVisitorImpl<tc>::visit(*child, begin, end, mode, op);
      }
    });

    return result;
  }
};

/**
 * Primitives - fallback specialization
 * - string / binary
 * - floating_point
 * - integral
 * - enumeration
 */
template <typename TC>
struct PathVisitorImpl {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Node, typename Op>
  static ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Op& op) {
    if (mode == PathVisitMode::FULL || begin == end) {
      try {
        // unfortunately its tough to get full const correctness for primitive
        // types since we don't enforce whether or not lambdas or operators take
        // a const param. Here we cast away the const and rely on primitive
        // node's functions throwing an exception if the node is immutable.
        op.visitTyped(
            *const_cast<std::remove_const_t<Node>*>(&node), begin, end);
        if (begin == end) {
          return ThriftTraverseResult::OK;
        }
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Exception while traversing path: " << ex.what();
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }
    return ThriftTraverseResult::NON_EXISTENT_NODE;
  }
};

} // namespace pv_detail

// Helper for creating LambdaPathVisitorOperator. As above, should be used
// sparingly, only when node types are required
template <typename Func>
inline pv_detail::LambdaPathVisitorOperator<Func> pvlambda(Func&& f) {
  return pv_detail::LambdaPathVisitorOperator<Func>(std::forward<Func>(f));
}

template <typename TC>
struct PathVisitor {
  template <typename Node, typename Func>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      pv_detail::LambdaPathVisitorOperator<Func>& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::PathVisitorImpl<TC>::visit(node, begin, end, mode, op);
  }

  template <typename Node>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      BasePathVisitorOperator& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::PathVisitorImpl<TC>::visit(node, begin, end, mode, op);
  }

  template <typename Fields, typename Func>
  inline static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      pv_detail::LambdaPathVisitorOperator<Func>& op)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    return pv_detail::PathVisitorImpl<TC>::visit(fields, begin, end, mode, op);
  }

  template <typename Fields>
  inline static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      BasePathVisitorOperator& op)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    return pv_detail::PathVisitorImpl<TC>::visit(fields, begin, end, mode, op);
  }
};

using RootPathVisitor = PathVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::thrift_cow
