// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <type_traits>

#include <fboss/thrift_cow/visitors/VisitorUtils.h>
#include <re2/re2.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::thrift_cow {

struct HybridNodeType;

/*
 * This visitor takes a path object and a thrift type and is able to
 * traverse the object and then run the visitor function against the
 * final type.
 *
 * TODO: add error result handling similar to PathVisitor.
 */

template <typename TC>
struct ExtendedPathVisitor;

struct NodeType;
struct FieldsType;

struct ExtPathVisitorOptions {
  explicit ExtPathVisitorOptions(bool outputIdPaths = false)
      : outputIdPaths(outputIdPaths) {}

  bool outputIdPaths;
};

namespace epv_detail {

using ExtPathIter = typename std::vector<fsdb::OperPathElem>::const_iterator;

template <typename TC, typename Node, typename Func>
void visitNode(
    std::vector<std::string>& path,
    Node& node,
    ExtPathIter begin,
    ExtPathIter end,
    const ExtPathVisitorOptions& options,
    Func&& f)
  requires(std::is_same_v<typename Node::CowType, NodeType>)
{
  if (begin == end) {
    f(path, node);
    return;
  }

  if constexpr (std::is_const_v<Node>) {
    ExtendedPathVisitor<TC>::visit(
        path, *node.getFields(), begin, end, options, std::forward<Func>(f));
  } else {
    ExtendedPathVisitor<TC>::visit(
        path,
        *node.writableFields(),
        begin,
        end,
        options,
        std::forward<Func>(f));
  }
}

inline bool matchesStrToken(
    const std::string& tok,
    const fsdb::OperPathElem& elem) {
  if (elem.any_ref()) {
    return true;
  } else if (auto raw = elem.raw_ref()) {
    return *raw == tok;
  } else if (auto regex = elem.regex_ref()) {
    return re2::RE2::FullMatch(tok, *regex);
  }
  return false;
}

template <typename Enum>
std::optional<std::string> matchingEnumToken(
    const Enum& e,
    const fsdb::OperPathElem& elem) {
  // TODO: should we allow regex/raw matching by int value?
  auto enumName = apache::thrift::util::enumName(e);
  if (matchesStrToken(enumName, elem)) {
    return enumName;
  }

  auto enumValue = folly::to<std::string>(static_cast<int>(e));
  if (matchesStrToken(enumValue, elem)) {
    return enumValue;
  }

  return std::nullopt;
}

template <typename TC, typename TType>
std::optional<std::string> matchingToken(
    const TType& val,
    const fsdb::OperPathElem& elem) {
  if constexpr (std::is_same_v<TC, apache::thrift::type_class::string>) {
    if (matchesStrToken(val, elem)) {
      return val;
    }
  } else if constexpr (std::is_same_v<
                           TC,
                           apache::thrift::type_class::enumeration>) {
    return matchingEnumToken(val, elem);
  } else {
    auto strToken = folly::to<std::string>(val);
    if (matchesStrToken(strToken, elem)) {
      return strToken;
    }
  }

  return std::nullopt;
}

} // namespace epv_detail

/**
 * Set
 */
template <typename ValueTypeClass>
struct ExtendedPathVisitor<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;

  template <typename Node, typename Func>
  static inline void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    epv_detail::visitNode<TC>(
        path, node, begin, end, options, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for HybridNode
  }

  template <typename Fields, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Fields& fields,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    const auto& elem = *begin++;

    for (auto& val : fields) {
      auto matching =
          epv_detail::matchingToken<ValueTypeClass>(val->ref(), elem);
      if (matching) {
        path.push_back(*matching);
        ExtendedPathVisitor<ValueTypeClass>::visit(
            path, *val, begin, end, options, std::forward<Func>(f));
        path.pop_back();
      }
    }
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct ExtendedPathVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  using TC = apache::thrift::type_class::list<ValueTypeClass>;

  template <typename Node, typename Func>
  static inline void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    epv_detail::visitNode<TC>(
        path, node, begin, end, options, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for HybridNode
  }

  template <typename Fields, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Fields& fields,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    const auto& elem = *begin++;
    for (int i = 0; i < fields.size(); ++i) {
      auto matching =
          epv_detail::matchingToken<apache::thrift::type_class::integral>(
              i, elem);
      if (matching) {
        path.push_back(*matching);
        if constexpr (std::is_const_v<Fields>) {
          const auto& next = *fields.ref(i);
          ExtendedPathVisitor<ValueTypeClass>::visit(
              path, next, begin, end, options, std::forward<Func>(f));
        } else {
          ExtendedPathVisitor<ValueTypeClass>::visit(
              path, *fields.ref(i), begin, end, options, std::forward<Func>(f));
        }
        path.pop_back();
      }
    }
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct ExtendedPathVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;

  template <typename Node, typename Func>
  static inline void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    epv_detail::visitNode<TC>(
        path, node, begin, end, options, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for HybridNode
  }

  template <typename Fields, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Fields& fields,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    const auto& elem = *begin++;
    for (auto& [key, val] : fields) {
      auto matching = epv_detail::matchingToken<KeyTypeClass>(key, elem);
      if (matching) {
        path.push_back(*matching);

        // ensure we propagate constness, since children will have type
        // const shared_ptr<T>, not shared_ptr<const T>.
        if constexpr (std::is_const_v<Fields>) {
          const auto& next = *val;
          ExtendedPathVisitor<MappedTypeClass>::visit(
              path, next, begin, end, options, std::forward<Func>(f));
        } else {
          ExtendedPathVisitor<MappedTypeClass>::visit(
              path, *val, begin, end, options, std::forward<Func>(f));
        }

        path.pop_back();
      }
    }
  }
};

/**
 * Variant
 */
template <>
struct ExtendedPathVisitor<apache::thrift::type_class::variant> {
  using TC = apache::thrift::type_class::variant;

  template <typename Node, typename Func>
  static inline void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    epv_detail::visitNode<TC>(
        path, node, begin, end, options, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for HybridNode
  }

  template <typename Fields, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Fields& fields,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    const auto& elem = *begin++;
    auto raw = elem.raw_ref();
    if (!raw) {
      // Error! wildcards not supported for enum or struct
      return;
    }

    // TODO: A lot of shared logic with PathVisitor. Could we share code?
    using MemberTypes = typename Fields::MemberTypes;
    visitMember<MemberTypes>(*raw, [&](auto tag) {
      using descriptor = typename decltype(fatal::tag_type(tag))::member;
      using name = typename descriptor::metadata::name;
      using tc = typename descriptor::metadata::type_class;

      if (fields.type() != descriptor::metadata::id::value) {
        // TODO: error handling
        return;
      }

      std::string memberName = options.outputIdPaths
          ? folly::to<std::string>(descriptor::metadata::id::value)
          : std::string(fatal::z_data<name>(), fatal::size<name>::value);

      path.push_back(std::move(memberName));

      // ensure we propagate constness, since children will have type
      // const shared_ptr<T>, not shared_ptr<const T>.
      auto& child = fields.template ref<name>();
      if constexpr (std::is_const_v<Fields>) {
        const auto& next = *child;
        ExtendedPathVisitor<tc>::visit(
            path, next, begin, end, options, std::forward<Func>(f));
      } else {
        ExtendedPathVisitor<tc>::visit(
            path, *child, begin, end, options, std::forward<Func>(f));
      }

      path.pop_back();
    });
  }
};

/**
 * Structure
 */
template <>
struct ExtendedPathVisitor<apache::thrift::type_class::structure> {
  using TC = apache::thrift::type_class::structure;

  template <typename Node, typename Func>
  static inline void visit(
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    std::vector<std::string> path;
    visit(path, node, begin, end, options, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static inline void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    epv_detail::visitNode<TC>(
        path, node, begin, end, options, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Node::CowType, HybridNodeType>)
  {
    // TODO: implement specialization for HybridNode
  }

  template <typename Fields, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Fields& fields,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& options,
      Func&& f)
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using Members = typename Fields::Members;

    const auto& elem = *begin++;
    auto raw = elem.raw_ref();
    if (!raw) {
      // Error! wildcards not supported for enum or struct
      return;
    }

    // Perform trie search over all members for key
    visitMember<Members>(*raw, [&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using name = typename member::name;
      using tc = typename member::type_class;

      // Recurse further
      auto& child = fields.template ref<name>();

      if (!child) {
        // child is unset, cannot traverse through missing optional child
        return;
      }
      std::string memberName = options.outputIdPaths
          ? folly::to<std::string>(member::id::value)
          : std::string(fatal::z_data<name>(), fatal::size<name>::value);

      path.push_back(std::move(memberName));

      // ensure we propagate constness, since children will have type
      // const shared_ptr<T>, not shared_ptr<const T>.
      if constexpr (std::is_const_v<Fields>) {
        const auto& next = *child;
        ExtendedPathVisitor<tc>::visit(
            path, next, begin, end, options, std::forward<Func>(f));
      } else {
        ExtendedPathVisitor<tc>::visit(
            path, *child, begin, end, options, std::forward<Func>(f));
      }

      path.pop_back();
    });
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
struct ExtendedPathVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Node, typename Func>
  static void visit(
      std::vector<std::string>& path,
      Node& node,
      epv_detail::ExtPathIter begin,
      epv_detail::ExtPathIter end,
      const ExtPathVisitorOptions& /* options */,
      Func&& f) {
    f(path, node);
  }
};

using RootExtendedPathVisitor =
    ExtendedPathVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::thrift_cow
