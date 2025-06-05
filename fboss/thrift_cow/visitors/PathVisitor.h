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
struct NodeType;
struct FieldsType;

namespace pv_detail {
using PathIter = typename std::vector<std::string>::const_iterator;
}

// BasePathVisitorOperator is a base class for "untyped" operators.
// This base class should be the preferred way to use visitors. Operations
// should subclass this and override the virtual methods but pass a pointer
// to the base class to avoid extra unique instantiations
typedef std::function<void(
    Serializable& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end)>
    SerializableVisitorFunc;

typedef std::function<void(
    const Serializable& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end)>
    ConstSerializableVisitorFunc;

struct BasePathVisitorOperator {
  explicit BasePathVisitorOperator(SerializableVisitorFunc&& f)
      : visitFunc_(f) {}

  explicit BasePathVisitorOperator(ConstSerializableVisitorFunc&& f)
      : constVisitFunc_(f) {}

  virtual ~BasePathVisitorOperator() = default;

  template <typename TC, typename Node>
  inline void
  visitTyped(Node& node, pv_detail::PathIter begin, pv_detail::PathIter end)
    requires(is_cow_type_v<Node>)
  {
    if constexpr (std::is_const_v<Node>) {
      cvisit(node, begin, end);
    } else {
      visit(node, begin, end);
    }
  }

  template <typename TC, typename Node>
  inline void
  visitTyped(Node& node, pv_detail::PathIter begin, pv_detail::PathIter end)
    requires(!is_cow_type_v<Node>)
  {
    // Node is not a Serializable, dispatch with wrapper
    SerializableWrapper<TC, Node> wrapper(node);
    if constexpr (std::is_const_v<Node>) {
      cvisit(wrapper, begin, end);
    } else {
      visit(wrapper, begin, end);
    }
  }

 protected:
  void visit(
      Serializable& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end) {
    if (constVisitFunc_) {
      constVisitFunc_(node, begin, end);
    }
    if (visitFunc_) {
      visitFunc_(node, begin, end);
    }
  }

  void cvisit(
      const Serializable& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end) {
    if (constVisitFunc_) {
      constVisitFunc_(node, begin, end);
    }
    if (visitFunc_) {
      throw std::runtime_error("const visitor called on non-const node");
    }
  }

 private:
  SerializableVisitorFunc visitFunc_{nullptr};
  ConstSerializableVisitorFunc constVisitFunc_{nullptr};
};

struct GetEncodedPathVisitorOperator : public BasePathVisitorOperator {
  explicit GetEncodedPathVisitorOperator(fsdb::OperProtocol protocol)
      : BasePathVisitorOperator(
            ConstSerializableVisitorFunc([this](
                                             const Serializable& node,
                                             pv_detail::PathIter,
                                             pv_detail::PathIter) {
              auto encoded = node.encode(protocol_);
              val = encoded;
            })),
        protocol_(protocol) {}

  std::optional<folly::fbstring> val{std::nullopt};

 private:
  const fsdb::OperProtocol protocol_;
};

struct SetEncodedPathVisitorOperator : public BasePathVisitorOperator {
  SetEncodedPathVisitorOperator(
      fsdb::OperProtocol protocol,
      const folly::fbstring& val)
      : BasePathVisitorOperator(SerializableVisitorFunc(
            [this](
                Serializable& node,
                pv_detail::PathIter,
                pv_detail::PathIter) { node.fromEncoded(protocol_, val_); })),
        protocol_(protocol),
        val_(val) {}

 private:
  const fsdb::OperProtocol protocol_;
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

inline std::string traverseResultString(ThriftTraverseResult result) {
  switch (result) {
    case ThriftTraverseResult::OK:
      return "OK";
    case ThriftTraverseResult::NON_EXISTENT_NODE:
      return "NON_EXISTENT_NODE";
    case ThriftTraverseResult::INVALID_ARRAY_INDEX:
      return "INVALID_ARRAY_INDEX";
    case ThriftTraverseResult::INVALID_MAP_KEY:
      return "INVALID_MAP_KEY";
    case ThriftTraverseResult::INVALID_STRUCT_MEMBER:
      return "INVALID_STRUCT_MEMBER";
    case ThriftTraverseResult::INVALID_VARIANT_MEMBER:
      return "INVALID_VARIANT_MEMBER";
    case ThriftTraverseResult::INCORRECT_VARIANT_MEMBER:
      return "INCORRECT_VARIANT_MEMBER";
    case ThriftTraverseResult::VISITOR_EXCEPTION:
      return "VISITOR_EXCEPTION";
    case ThriftTraverseResult::INVALID_SET_MEMBER:
      return "INVALID_SET_MEMBER";
    default:
      return "ThriftTraverseResult::unknown";
  }
}

/*
 * invokeVisitorFnHelper allows us to support two different visitor
 * signatures:
 *
 * 1. f(node)
 * 2. f(node, begin, end)
 *
 * This allows a visitor to use the remaining path tokens if needed.
 */

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

struct PathVisitOptions {
  static PathVisitOptions visitFull(
      bool skipOptionalOrImmutablePrimitiveNode = false) {
    return PathVisitOptions(
        PathVisitMode::FULL, skipOptionalOrImmutablePrimitiveNode);
  }

  static PathVisitOptions visitLeaf(
      bool skipOptionalOrImmutablePrimitiveNode = false) {
    return PathVisitOptions(
        PathVisitMode::LEAF, skipOptionalOrImmutablePrimitiveNode);
  }

  explicit PathVisitOptions(
      PathVisitMode mode,
      bool skipOptionalOrImmutablePrimitiveNode = false)
      : mode(mode),
        skipOptionalOrImmutablePrimitiveNode(
            skipOptionalOrImmutablePrimitiveNode) {}

  PathVisitMode mode;
  bool skipOptionalOrImmutablePrimitiveNode;
};

namespace pv_detail {
template <typename TC>
struct PathVisitorImpl;

using PathIter = typename std::vector<std::string>::const_iterator;

template <typename Op>
struct VisitImplParams {
 public:
  VisitImplParams(
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitOptions& options,
      Op& op)
      : begin(begin), end(end), options(options), op(op) {}

  pv_detail::PathIter begin;
  pv_detail::PathIter end;
  const PathVisitOptions& options;
  Op& op;
};

// Version of an operator that forwards operation to a lambda. This should be
// used as sparingly as possible, only when node types are needed, because
// every call with this templated operator is a unique instantiation of the
// entire template tree
template <typename Func>
struct LambdaPathVisitorOperator {
  explicit LambdaPathVisitorOperator(Func&& f) : f_(std::forward<Func>(f)) {}

  template <typename TC, typename Node>
  inline auto
  visitTyped(Node& node, pv_detail::PathIter begin, pv_detail::PathIter end)
    requires(is_cow_type_v<Node>)
  {
    return f_(node, begin, end);
  }

  template <typename TC, typename Node>
  inline auto
  visitTyped(Node& node, pv_detail::PathIter begin, pv_detail::PathIter end)
    requires(!is_cow_type_v<Node>)
  {
    // Node is not a Serializable, dispatch with wrapper
    SerializableWrapper<TC, Node> wrapper(node);
    return f_(wrapper, begin, end);
  }

 private:
  Func f_;
};

template <typename TC, typename Node, typename Op>
ThriftTraverseResult
visitNode(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
    // only enable for Node types
  requires(std::is_same_v<typename Node::CowType, NodeType>)
{
  if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
    try {
      params.op.template visitTyped<TC, Node>(node, cursor, params.end);
      if (cursor == params.end) {
        return ThriftTraverseResult::OK;
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
  }

  if constexpr (std::is_const_v<Node>) {
    return PathVisitorImpl<TC>::visit(*node.getFields(), params, cursor);
  } else {
    return PathVisitorImpl<TC>::visit(*node.writableFields(), params, cursor);
  }
}

/**
 * Set
 */
template <typename ValueTypeClass>
struct PathVisitorImpl<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;

  template <typename Node, typename Op>
  static inline ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, params, cursor);
  }

  template <typename Obj, typename Op>
  static inline ThriftTraverseResult
  visit(Obj& tObj, const VisitImplParams<Op>& params, PathIter cursor)
    requires(!is_cow_type_v<Obj> && !is_field_type_v<Obj>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Obj>(tObj, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    using ValueTType = typename Obj::value_type;

    // Get value
    auto token = *cursor++;

    if (auto value = tryParseKey<ValueTType, ValueTypeClass>(token)) {
      if (auto it = tObj.find(*value); it != tObj.end()) {
        // Recurse further
        return PathVisitorImpl<ValueTypeClass>::visit(*it, params, cursor);
      } else {
        return ThriftTraverseResult::NON_EXISTENT_NODE;
      }
    }
    // if we get here, we must have a malformed value
    return ThriftTraverseResult::INVALID_SET_MEMBER;
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult visit(
      Node& node /* node */,
      const VisitImplParams<Op>& params,
      PathIter cursor)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Node>(node, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    auto& tObj = node.ref();
    using ValueTType = typename Node::ThriftType::value_type;

    // Get value
    auto token = *cursor++;

    if (auto value = tryParseKey<ValueTType, ValueTypeClass>(token)) {
      if (auto it = tObj.find(*value); it != tObj.end()) {
        // Recurse further
        return PathVisitorImpl<ValueTypeClass>::visit(*it, params, cursor);
      } else {
        return ThriftTraverseResult::NON_EXISTENT_NODE;
      }
    }
    // if we get here, we must have a malformed value
    return ThriftTraverseResult::INVALID_SET_MEMBER;
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult
  visit(Fields& fields, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Fields types
    requires(
        is_field_type_v<Fields> &&
        std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using ValueTType = typename Fields::ValueTType;

    // Get value
    auto token = *cursor++;

    if (auto value = tryParseKey<ValueTType, ValueTypeClass>(token)) {
      if (auto it = fields.find(*value); it != fields.end()) {
        // Recurse further
        return PathVisitorImpl<ValueTypeClass>::visit(**it, params, cursor);
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
  static inline ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, params, cursor);
  }

  template <typename Obj, typename Op>
  static inline ThriftTraverseResult
  visit(Obj& tObj, const VisitImplParams<Op>& params, PathIter cursor)
    requires(!is_cow_type_v<Obj> && !is_field_type_v<Obj>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Obj>(tObj, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    // Parse and pop token. Also check for index bound
    auto index = folly::tryTo<size_t>(*cursor++);
    if (index.hasError() || index.value() >= tObj.size()) {
      return ThriftTraverseResult::INVALID_ARRAY_INDEX;
    }
    return PathVisitorImpl<ValueTypeClass>::visit(
        tObj.at(index.value()), params, cursor);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Node>(node, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    // get the value based on the key
    auto& tObj = node.ref();
    // Parse and pop token. Also check for index bound
    auto index = folly::tryTo<size_t>(*cursor++);
    if (index.hasError() || index.value() >= tObj.size()) {
      return ThriftTraverseResult::INVALID_ARRAY_INDEX;
    }
    return PathVisitorImpl<ValueTypeClass>::visit(
        tObj.at(index.value()), params, cursor);
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult
  visit(Fields& fields, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Fields types
    requires(
        is_field_type_v<Fields> &&
        std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    // Parse and pop token. Also check for index bound
    auto index = folly::tryTo<size_t>(*cursor++);
    if (index.hasError() || index.value() >= fields.size()) {
      return ThriftTraverseResult::INVALID_ARRAY_INDEX;
    }

    // Recurse at a given index
    if constexpr (std::is_const_v<Fields>) {
      const auto& next = *fields.ref(index.value());
      return PathVisitorImpl<ValueTypeClass>::visit(next, params, cursor);
    } else {
      return PathVisitorImpl<ValueTypeClass>::visit(
          *fields.ref(index.value()), params, cursor);
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
  static inline ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, params, cursor);
  }

  template <typename Obj, typename Op>
  static inline ThriftTraverseResult
  visit(Obj& tObj, const VisitImplParams<Op>& params, PathIter cursor)
    requires(!is_cow_type_v<Obj> && !is_field_type_v<Obj>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Obj>(tObj, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    // get the value based on the key
    using KeyT = typename folly::remove_cvref_t<decltype(tObj)>::key_type;
    // Get key
    auto token = *cursor++;
    auto key = folly::tryTo<KeyT>(token);
    if (!key.hasValue()) {
      return ThriftTraverseResult::INVALID_MAP_KEY;
    } else if (tObj.find(key.value()) == tObj.end()) {
      return ThriftTraverseResult::NON_EXISTENT_NODE;
    }
    return PathVisitorImpl<MappedTypeClass>::visit(
        tObj.at(*key), params, cursor);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Node>(node, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    // get the value based on the key
    auto& tObj = node.ref();
    using KeyT = typename folly::remove_cvref_t<decltype(tObj)>::key_type;
    // Get key
    auto token = *cursor++;
    auto key = folly::tryTo<KeyT>(token);
    if (!key.hasValue()) {
      return ThriftTraverseResult::INVALID_MAP_KEY;
    } else if (tObj.find(key.value()) == tObj.end()) {
      return ThriftTraverseResult::NON_EXISTENT_NODE;
    }
    return PathVisitorImpl<MappedTypeClass>::visit(
        tObj.at(*key), params, cursor);
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult
  visit(Fields& fields, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Fields types
    requires(
        is_field_type_v<Fields> &&
        std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using key_type = typename Fields::key_type;

    // Get key
    auto token = *cursor++;

    if (auto key = tryParseKey<key_type, KeyTypeClass>(token)) {
      if (fields.find(key.value()) != fields.end()) {
        // Recurse further
        if constexpr (std::is_const_v<Fields>) {
          const auto& next = *fields.ref(key.value());
          return PathVisitorImpl<MappedTypeClass>::visit(next, params, cursor);
        } else {
          return PathVisitorImpl<MappedTypeClass>::visit(
              *fields.ref(key.value()), params, cursor);
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
  static inline ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, params, cursor);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult
  visit(Node& /* node */, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for hybrid nodes
    XLOG(ERR) << "Unimplemented visitation for hybrid node: path: "
              << folly::join("/", params.begin, params.end)
              << " at: " << (cursor == params.end ? "(end)" : *cursor);
    return ThriftTraverseResult::VISITOR_EXCEPTION;
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult
  visit(Fields& fields, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Fields types
    requires(
        is_field_type_v<Fields> &&
        std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using MemberTypes = typename Fields::MemberTypes;

    auto result = ThriftTraverseResult::INVALID_VARIANT_MEMBER;

    // Get key
    auto key = *cursor++;

    visitMember<MemberTypes>(key, [&](auto tag) {
      using descriptor = typename decltype(fatal::tag_type(tag))::member;
      using name = typename descriptor::metadata::name;
      using tc = typename descriptor::metadata::type_class;

      if (folly::to_underlying(fields.type()) !=
          descriptor::metadata::id::value) {
        result = ThriftTraverseResult::INCORRECT_VARIANT_MEMBER;
        return;
      }

      auto& child = fields.template ref<name>();

      // ensure we propagate constness, since children will have type
      // const shared_ptr<T>, not shared_ptr<const T>.
      if constexpr (std::is_const_v<Fields>) {
        const auto& next = *child;
        result = PathVisitorImpl<tc>::visit(next, params, cursor);
      } else {
        result = PathVisitorImpl<tc>::visit(*child, params, cursor);
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
  static inline ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return pv_detail::visitNode<TC>(node, params, cursor);
  }

  template <typename Node, typename Op>
  static ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for HybridNode types
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Node>(node, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    auto& tObj = node.ref();
    using T = typename Node::ThriftType;
    // Get key
    auto key = *cursor++;
    // Perform linear search over all members for key
    ThriftTraverseResult result = ThriftTraverseResult::INVALID_STRUCT_MEMBER;
    using Members = typename apache::thrift::reflect_struct<T>::members;
    visitMember<Members>(key, [&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using tc = typename member::type_class;
      using getter = typename member::getter;
      const std::string fieldNameStr =
          fatal::to_instance<std::string, typename member::name>();
      if constexpr (
          member::optional::value == apache::thrift::optionality::optional) {
        if (!member::is_set(tObj)) {
          result = ThriftTraverseResult::NON_EXISTENT_NODE;
          return;
        }
      }
      // Recurse further
      auto& child = getter{}(tObj);
      result = PathVisitorImpl<tc>::visit(child, params, cursor);
    });

    return result;
  }

  template <typename Obj, typename Op>
  static inline ThriftTraverseResult
  visit(Obj& tObj, const VisitImplParams<Op>& params, PathIter cursor)
    requires(!is_cow_type_v<Obj> && !is_field_type_v<Obj>)
  {
    try {
      if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
        params.op.template visitTyped<TC, Obj>(tObj, cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while traversing path: "
                << folly::join("/", params.begin, params.end)
                << " at: " << (cursor == params.end ? "(end)" : *cursor)
                << ", exception: " << ex.what();
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
    // Get key
    auto key = *cursor++;
    // Perform linear search over all members for key
    ThriftTraverseResult result = ThriftTraverseResult::INVALID_STRUCT_MEMBER;
    using Members =
        typename apache::thrift::reflect_struct<std::remove_cv_t<Obj>>::members;
    visitMember<Members>(key, [&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using tc = typename member::type_class;
      using getter = typename member::getter;
      const std::string fieldNameStr =
          fatal::to_instance<std::string, typename member::name>();
      if constexpr (
          member::optional::value == apache::thrift::optionality::optional) {
        if (!member::is_set(tObj)) {
          result = ThriftTraverseResult::NON_EXISTENT_NODE;
          return;
        }
      }
      // Recurse further
      auto& child = getter{}(tObj);
      result = PathVisitorImpl<tc>::visit(child, params, cursor);
    });

    return result;
  }

  template <typename Fields, typename Op>
  static ThriftTraverseResult
  visit(Fields& fields, const VisitImplParams<Op>& params, PathIter cursor)
      // only enable for Fields types
    requires(
        is_field_type_v<Fields> &&
        std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using Members = typename Fields::Members;

    // Get key
    auto key = *cursor++;

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
        result = PathVisitorImpl<tc>::visit(next, params, cursor);
      } else {
        result = PathVisitorImpl<tc>::visit(*child, params, cursor);
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

  template <typename T>
  struct optional_field : std::false_type {};
  template <typename T>
  struct optional_field<apache::thrift::optional_field_ref<T>>
      : std::true_type {};

  template <typename Node, typename Op>
  static ThriftTraverseResult
  visit(Node& node, const VisitImplParams<Op>& params, PathIter cursor) {
    if constexpr (optional_field<folly::remove_cvref_t<Node>>::value) {
      if (params.options.skipOptionalOrImmutablePrimitiveNode) {
        return ThriftTraverseResult::OK;
      }
    }
    if constexpr (is_cow_type_v<Node>) {
      if constexpr (!std::is_same_v<typename Node::CowType, HybridNodeType>) {
        if (params.options.skipOptionalOrImmutablePrimitiveNode &&
            node.immutable) {
          return ThriftTraverseResult::OK;
        }
      }
    }
    if (params.options.mode == PathVisitMode::FULL || cursor == params.end) {
      try {
        // unfortunately its tough to get full const correctness for primitive
        // types since we don't enforce whether or not lambdas or operators
        // take a const param. Here we cast away the const and rely on
        // primitive node's functions throwing an exception if the node is
        // immutable.
        params.op.template visitTyped<
            std::remove_const_t<TC>,
            std::remove_const_t<Node>>(
            *const_cast<std::remove_const_t<Node>*>(&node), cursor, params.end);
        if (cursor == params.end) {
          return ThriftTraverseResult::OK;
        }
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Exception while traversing path: "
                  << folly::join("/", params.begin, params.end)
                  << " at: " << (cursor == params.end ? "(end)" : *cursor)
                  << ", exception: " << ex.what();
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
      const PathVisitOptions& options,
      pv_detail::LambdaPathVisitorOperator<Func>& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    pv_detail::VisitImplParams<pv_detail::LambdaPathVisitorOperator<Func>>
        params(begin, end, options, op);
    return pv_detail::PathVisitorImpl<TC>::visit(node, params, begin);
  }

  template <typename Node>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitOptions& options,
      BasePathVisitorOperator& op)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    pv_detail::VisitImplParams<BasePathVisitorOperator> params(
        begin, end, options, op);
    return pv_detail::PathVisitorImpl<TC>::visit(node, params, begin);
  }

  template <typename Fields, typename Func>
  inline static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitOptions& options,
      pv_detail::LambdaPathVisitorOperator<Func>& op)
      // only enable for Fields types
    requires(
        is_field_type_v<Fields> &&
        std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    pv_detail::VisitImplParams<pv_detail::LambdaPathVisitorOperator<Func>>
        params(begin, end, options, op);
    return pv_detail::PathVisitorImpl<TC>::visit(fields, params, begin);
  }

  template <typename Fields>
  inline static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitOptions& options,
      BasePathVisitorOperator& op)
      // only enable for Fields types
    requires(
        is_field_type_v<Fields> &&
        std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    pv_detail::VisitImplParams<BasePathVisitorOperator> params(
        begin, end, options, op);
    return pv_detail::PathVisitorImpl<TC>::visit(fields, params, begin);
  }
};

using RootPathVisitor = PathVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::thrift_cow
