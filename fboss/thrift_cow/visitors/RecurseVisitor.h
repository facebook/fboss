// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <algorithm>

#include <fboss/thrift_cow/visitors/TraverseHelper.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

enum class RecurseVisitMode {
  /*
   * In this mode, we visit every path in the tree, including
   * intermediate nodes.
   */
  FULL,

  /*
   * In this mode, we only visit leaf nodes.
   */
  LEAVES
};

template <typename>
struct RecurseVisitor;

struct NodeType;
struct FieldsType;

namespace rv_detail {
template <
    typename TC,
    typename Node,
    typename TraverseHelper,
    typename Func,
    // only enable for Node types
    std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
        true>
void visitNode(
    TraverseHelper& traverser,
    const std::shared_ptr<Node>& node,
    const RecurseVisitMode& mode,
    Func&& f) {
  if (mode == RecurseVisitMode::FULL) {
    f(traverser.path(), node);
  }

  if (traverser.shouldShortCircuit(VisitorType::RECURSE)) {
    return;
  }

  RecurseVisitor<TC>::visit(
      traverser, *node->getFields(), mode, std::forward<Func>(f));
}

} // namespace rv_detail

/**
 * Set
 */
template <typename ValueTypeClass>
struct RecurseVisitor<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;
  template <
      typename Node,
      typename TraverseHelper,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline void visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& node,
      const RecurseVisitMode& mode,
      Func&& f) {
    return rv_detail::visitNode<TC>(
        traverser, node, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename TraverseHelper,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static void visit(
      TraverseHelper& traverser,
      const Fields& fields,
      const RecurseVisitMode& /*mode*/,
      Func&& f) {
    for (auto& val : fields) {
      traverser.push(folly::to<std::string>(val->cref()));
      f(traverser.path(), typename Fields::value_type{val});
      traverser.pop();
    }
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct RecurseVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  using TC = apache::thrift::type_class::list<ValueTypeClass>;
  template <
      typename Node,
      typename TraverseHelper,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline void visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& node,
      const RecurseVisitMode& mode,
      Func&& f) {
    return rv_detail::visitNode<TC>(
        traverser, node, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename TraverseHelper,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static void visit(
      TraverseHelper& traverser,
      const Fields& fields,
      const RecurseVisitMode& mode,
      Func&& f) {
    for (int i = 0; i < fields.size(); ++i) {
      traverser.push(folly::to<std::string>(i));
      RecurseVisitor<ValueTypeClass>::visit(
          traverser, fields.at(i), mode, std::forward<Func>(f));
      traverser.pop();
    }
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct RecurseVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;
  template <
      typename Node,
      typename TraverseHelper,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline void visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& node,
      const RecurseVisitMode& mode,
      Func&& f) {
    return rv_detail::visitNode<TC>(
        traverser, node, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename TraverseHelper,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static void visit(
      TraverseHelper& traverser,
      const Fields& fields,
      const RecurseVisitMode& mode,
      Func&& f) {
    for (const auto& [key, val] : fields) {
      traverser.push(folly::to<std::string>(key));
      RecurseVisitor<MappedTypeClass>::visit(
          traverser, val, mode, std::forward<Func>(f));
      traverser.pop();
    }
  }
};

/**
 * Variant
 */
template <>
struct RecurseVisitor<apache::thrift::type_class::variant> {
  using TC = apache::thrift::type_class::variant;
  template <
      typename Node,
      typename TraverseHelper,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline void visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& node,
      const RecurseVisitMode& mode,
      Func&& f) {
    return rv_detail::visitNode<TC>(
        traverser, node, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename TraverseHelper,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static void visit(
      TraverseHelper& traverser,
      const Fields& fields,
      const RecurseVisitMode& mode,
      Func&& f) {
    using Members = typename Fields::Members;

    fatal::scalar_search<Members, fatal::get_type::id>(
        fields.type(), [&](auto indexed) {
          using descriptor = decltype(fatal::tag_type(indexed));
          using name = typename descriptor::metadata::name;
          using tc = typename descriptor::metadata::type_class;

          std::string memberName =
              std::string(fatal::z_data<name>(), fatal::size<name>::value);

          traverser.push(std::move(memberName));

          const auto& ref = fields.template cref<name>();
          RecurseVisitor<tc>::visit(
              traverser, ref, mode, std::forward<Func>(f));
          traverser.pop();
        });
  }
};

/**
 * Structure
 */
template <>
struct RecurseVisitor<apache::thrift::type_class::structure> {
  using TC = apache::thrift::type_class::structure;
  template <typename Node, typename Func>
  static void visit(
      const std::shared_ptr<Node>& node,
      const RecurseVisitMode& mode,
      Func&& f) {
    SimpleTraverseHelper traverser;
    return visit(traverser, node, mode, std::forward<Func>(f));
  }

  template <
      typename Node,
      typename TraverseHelper,
      typename Func,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline void visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& node,
      const RecurseVisitMode& mode,
      Func&& f) {
    return rv_detail::visitNode<TC>(
        traverser, node, mode, std::forward<Func>(f));
  }

  template <
      typename Fields,
      typename TraverseHelper,
      typename Func,
      // only enable for Fields types
      std::enable_if_t<
          std::is_same_v<typename Fields::CowType, FieldsType>,
          bool> = true>
  static void visit(
      TraverseHelper& traverser,
      const Fields& fields,
      const RecurseVisitMode& mode,
      Func&& f) {
    using Members = typename Fields::Members;

    fatal::foreach<Members>([&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using name = typename member::name;

      // Look for the expected member name
      std::string memberName(fatal::z_data<name>(), fatal::size<name>::value);

      traverser.push(std::move(memberName));
      const auto& ref = fields.template cref<name>();

      if (ref) {
        RecurseVisitor<typename member::type_class>::visit(
            traverser, ref, mode, std::forward<Func>(f));
      }
      traverser.pop();
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
struct RecurseVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Fields, typename TraverseHelper, typename Func>
  static void visit(
      TraverseHelper& traverser,
      const Fields& fields,
      const RecurseVisitMode& mode,
      Func&& f) {
    f(traverser.path(), fields);
  }
};

using RootRecurseVisitor =
    RecurseVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::thrift_cow
