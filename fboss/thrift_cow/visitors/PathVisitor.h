// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <type_traits>
#include "folly/Conv.h"

#include <fboss/thrift_cow/visitors/VisitorUtils.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

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

template <typename TC>
struct PathVisitor;

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

using PathIter = typename std::vector<std::string>::const_iterator;

template <typename Node, typename Func>
auto invokeVisitorFnHelper(Node& node, PathIter begin, PathIter end, Func&& f)
    -> std::invoke_result_t<Func, Node&, PathIter, PathIter> {
  return f(node, begin, end);
}

template <typename Node, typename Func>
auto invokeVisitorFnHelper(
    Node& node,
    PathIter /*begin*/,
    PathIter /*end*/,
    Func&& f) -> std::invoke_result_t<Func, Node&> {
  return f(node);
}

template <
    typename TC,
    typename Node,
    typename Func,
    // only enable for Node types
    std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
        true>
ThriftTraverseResult visitNode(
    Node& node,
    PathIter begin,
    PathIter end,
    const PathVisitMode& mode,
    Func&& f) {
  if (mode == PathVisitMode::FULL || begin == end) {
    try {
      invokeVisitorFnHelper(node, begin, end, std::forward<Func>(f));
      if (begin == end) {
        return ThriftTraverseResult::OK;
      }
    } catch (const std::exception&) {
      return ThriftTraverseResult::VISITOR_EXCEPTION;
    }
  }

  if constexpr (std::is_const_v<Node>) {
    return PathVisitor<TC>::visit(
        *node.getFields(), begin, end, mode, std::forward<Func>(f));
  } else {
    return PathVisitor<TC>::visit(
        *node.writableFields(), begin, end, mode, std::forward<Func>(f));
  }
}

} // namespace pv_detail

/**
 * Set
 */
template <typename ValueTypeClass>
struct PathVisitor<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;

  template <
      typename Node,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    return pv_detail::visitNode<TC>(
        node, begin, end, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    using ValueTType = typename Fields::ValueTType;

    // Get value
    auto token = *begin++;
    ValueTType value;

    // need to handle enumeration, integral, string types
    if constexpr (std::is_same_v<
                      ValueTypeClass,
                      apache::thrift::type_class::enumeration>) {
      if (fatal::enum_traits<ValueTType>::try_parse(value, token)) {
        if (auto it = fields.find(value); it != fields.end()) {
          // Recurse further
          return PathVisitor<ValueTypeClass>::visit(
              **it, begin, end, mode, std::forward<Func>(f));
        }
      }
    }

    auto valueTry = folly::tryTo<ValueTType>(token);
    if (!valueTry.hasError()) {
      value = valueTry.value();
      if (auto it = fields.find(value); it != fields.end()) {
        // Recurse further
        return PathVisitor<ValueTypeClass>::visit(
            **it, begin, end, mode, std::forward<Func>(f));
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
struct PathVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  using TC = apache::thrift::type_class::list<ValueTypeClass>;

  template <
      typename Node,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    return pv_detail::visitNode<TC>(
        node, begin, end, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    // Parse and pop token. Also check for index bound
    auto index = folly::tryTo<size_t>(*begin++);
    if (index.hasError() || index.value() >= fields.size()) {
      return ThriftTraverseResult::INVALID_ARRAY_INDEX;
    }

    // Recurse at a given index
    if constexpr (std::is_const_v<Fields>) {
      const auto& next = *fields.ref(index.value());
      return PathVisitor<ValueTypeClass>::visit(
          next, begin, end, mode, std::forward<Func>(f));
    } else {
      return PathVisitor<ValueTypeClass>::visit(
          *fields.ref(index.value()), begin, end, mode, std::forward<Func>(f));
    }
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct PathVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;

  template <
      typename Node,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    return pv_detail::visitNode<TC>(
        node, begin, end, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    using key_type = typename Fields::key_type;

    // Get key
    auto token = *begin++;

    if constexpr (std::is_same_v<
                      KeyTypeClass,
                      apache::thrift::type_class::enumeration>) {
      // special handling for enum keyed maps
      key_type value;
      if (fatal::enum_traits<key_type>::try_parse(value, token)) {
        if (fields.find(value) != fields.end()) {
          // Recurse further
          return PathVisitor<MappedTypeClass>::visit(
              *fields.ref(value), begin, end, mode, std::forward<Func>(f));
        } else {
          return ThriftTraverseResult::NON_EXISTENT_NODE;
        }
      }
    }

    auto key = folly::tryTo<key_type>(token);
    if (key.hasValue()) {
      if (fields.find(key.value()) != fields.end()) {
        // Recurse further
        if constexpr (std::is_const_v<Fields>) {
          const auto& next = *fields.ref(key.value());
          return PathVisitor<MappedTypeClass>::visit(
              next, begin, end, mode, std::forward<Func>(f));
        } else {
          return PathVisitor<MappedTypeClass>::visit(
              *fields.ref(key.value()),
              begin,
              end,
              mode,
              std::forward<Func>(f));
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
struct PathVisitor<apache::thrift::type_class::variant> {
  using TC = apache::thrift::type_class::variant;

  template <
      typename Node,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    return pv_detail::visitNode<TC>(
        node, begin, end, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
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
        result = PathVisitor<tc>::visit(
            next, begin, end, mode, std::forward<Func>(f));
      } else {
        result = PathVisitor<tc>::visit(
            *child, begin, end, mode, std::forward<Func>(f));
      }
    });

    return result;
  }
};

/**
 * Structure
 */
template <>
struct PathVisitor<apache::thrift::type_class::structure> {
  using TC = apache::thrift::type_class::structure;

  template <
      typename Node,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    return pv_detail::visitNode<TC>(
        node, begin, end, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static ThriftTraverseResult visit(
      Fields& fields,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
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
        result = PathVisitor<tc>::visit(
            next, begin, end, mode, std::forward<Func>(f));
      } else {
        result = PathVisitor<tc>::visit(
            *child, begin, end, mode, std::forward<Func>(f));
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
struct PathVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Node, typename Func>
  static ThriftTraverseResult visit(
      Node& node,
      pv_detail::PathIter begin,
      pv_detail::PathIter end,
      const PathVisitMode& mode,
      Func&& f) {
    if (mode == PathVisitMode::FULL || begin == end) {
      try {
        pv_detail::invokeVisitorFnHelper(
            node, begin, end, std::forward<Func>(f));
        if (begin == end) {
          return ThriftTraverseResult::OK;
        }
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }
    return ThriftTraverseResult::NON_EXISTENT_NODE;
  }
};

using RootPathVisitor = PathVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::thrift_cow
