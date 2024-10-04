// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <glog/logging.h>
#include <type_traits>
#include "folly/Conv.h"
#include "folly/ScopeGuard.h"

#include <boost/iterator/function_output_iterator.hpp>

#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

#include <fboss/thrift_cow/visitors/RecurseVisitor.h>
#include <fboss/thrift_cow/visitors/TraverseHelper.h>
#include <fboss/thrift_cow/visitors/VisitorUtils.h>

namespace facebook::fboss::thrift_cow {

/*
 * This visitor takes two ThriftCow objects, finds changed paths and
 * then runs the provided function against any paths that differ
 * between the two objects.
 *
 * This can be used to find changes between two versions of the state
 * so that we can serve any related subscriptions.
 *
 * NOTE: this MAY emit some bogus ...
 */

template <typename TC>
struct DeltaVisitor;

struct NodeType;
struct FieldsType;

/*
 * We pass this tag to visitors to signal additional information about
 * the change. For now, we only signal whether it is MINIMAL or not,
 * but this could change.
 *
 * This makes it so clients can visit all differences, but still be
 * able extract a minimal delta.
 */
enum class DeltaElemTag { MINIMAL, NOT_MINIMAL };

enum class DeltaVisitMode {
  /*
   * In this mode, we emit a delta for every changed path in the tree,
   * ending at any newly added nodes. This means if I add a node at path
   * a.b.c.d, I will emit a delta value for a, a.b, a.b.c and a.b.c.d.
   *
   * Note we do not recurse past additions, so if there are additional
   * paths under d, we do not emit the deltas for those.
   */
  PARENTS,

  /*
   * In this mode, we emit a "minimal" delta, where we do not visit
   * parents along the path to a changed node. This is useful if
   * you want to try to serialize a delta efficiently.
   */
  MINIMAL,

  /*
   * In this mode, we emit a delta for every changed path in the tree,
   * including subpaths of newly added/removed paths. This is similar
   * to PARENTS, except that we don't end at subpaths that are
   * added/removed.
   */
  FULL
};

// Visit order for added/removed paths, not applicable for changed paths.
// For changed paths, DeltaVisitor will always visit children first to
// determine whether there is any delta and skip visiting parent if the
// subtree is unchanged.
enum class DeltaVisitOrder {
  PARENTS_FIRST,

  CHILDREN_FIRST
};

struct DeltaVisitOptions {
  explicit DeltaVisitOptions(
      DeltaVisitMode mode,
      DeltaVisitOrder order = DeltaVisitOrder::PARENTS_FIRST,
      bool outputIdPaths = false,
      bool recurseIntoHybridNodes = false)
      : mode(mode),
        order(order),
        outputIdPaths(outputIdPaths),
        recurseIntoHybridNodes(recurseIntoHybridNodes) {}

  DeltaVisitMode mode;
  DeltaVisitOrder order;
  bool outputIdPaths;
  bool recurseIntoHybridNodes;
};

namespace dv_detail {

/*
 * invokeVisitorFnHelper allows us to support two different visitor
 * signatures:
 *
 * 1. f(traverser, ...)
 * 2. f(path, ...)
 *
 * This allows a visitor to leverage a stateful TraverseHelper if
 * desired in the visit function.
 */

template <typename Node, typename TraverseHelper, typename Func>
auto invokeVisitorFnHelper(
    TraverseHelper& traverser,
    const Node& oldNode,
    const Node& newNode,
    DeltaElemTag deltaElemTag,
    Func&& f)
    -> std::invoke_result_t<
        Func,
        TraverseHelper&,
        const Node&,
        const Node&,
        DeltaElemTag> {
  return f(traverser, oldNode, newNode, deltaElemTag);
}

template <typename Node, typename TraverseHelper, typename Func>
auto invokeVisitorFnHelper(
    TraverseHelper& traverser,
    const Node& oldNode,
    const Node& newNode,
    DeltaElemTag deltaElemTag,
    Func&& f)
    -> std::invoke_result_t<
        Func,
        const std::vector<std::string>&,
        const Node&,
        const Node&,
        DeltaElemTag> {
  return f(traverser.path(), oldNode, newNode, deltaElemTag);
}

template <typename TC, typename Node, typename TraverseHelper, typename Func>
bool visitNode(
    TraverseHelper& traverser,
    const std::shared_ptr<Node>& oldNode,
    const std::shared_ptr<Node>& newNode,
    const DeltaVisitOptions& options,
    Func&& f)
    // only enable for Node types
  requires(std::is_same_v<typename Node::CowType, NodeType>)
{
  if (oldNode == newNode) {
    // This is the main benefit of deltas against thrift cow trees,
    // ability to shortcircuit delta when we KNOW there are no changes.
    return false;
  }

  if (traverser.shouldShortCircuit(VisitorType::DELTA)) {
    return false;
  }

  auto hasDifferences = DeltaVisitor<TC>::visit(
      traverser,
      *oldNode->getFields(),
      *newNode->getFields(),
      options,
      std::forward<Func>(f));

  if (hasDifferences &&
      (options.mode == DeltaVisitMode::PARENTS ||
       options.mode == DeltaVisitMode::FULL)) {
    invokeVisitorFnHelper(
        traverser,
        oldNode,
        newNode,
        DeltaElemTag::NOT_MINIMAL,
        std::forward<Func>(f));
  }

  return hasDifferences;
}

template <typename TC, typename Node, typename TraverseHelper, typename Func>
void visitAddedOrRemovedNode(
    TraverseHelper& traverser,
    const std::shared_ptr<Node>& oldNode,
    const std::shared_ptr<Node>& newNode,
    const DeltaVisitOptions& options,
    Func&& f)
    // only enable for Node types
  requires(std::is_same_v<typename Node::CowType, NodeType>)
{
  // Exactly one node must be non-null
  DCHECK(static_cast<bool>(oldNode) != static_cast<bool>(newNode));

  auto path = traverser.path();
  if (options.order == DeltaVisitOrder::PARENTS_FIRST) {
    // for added/removed paths, visit parent first, then children
    invokeVisitorFnHelper(
        traverser,
        oldNode,
        newNode,
        DeltaElemTag::MINIMAL,
        std::forward<Func>(f));
  }

  if (options.mode == DeltaVisitMode::FULL) {
    bool isAdd = static_cast<bool>(newNode);
    const auto target = (newNode) ? newNode : oldNode;
    auto processChange =
        [isAdd, initialPathSize = path.size(), f = std::forward<Func>(f)](
            TraverseHelper& subTraverser, const auto& node) mutable {
          const auto& subpath = subTraverser.path();
          if (subpath.size() == initialPathSize) {
            // skip visiting root, as we already visited it
            return;
          }

          using SubNode = decltype(node);
          SubNode oldSubNode = (isAdd) ? SubNode{} : node;
          SubNode newSubNode = (isAdd) ? node : SubNode{};
          invokeVisitorFnHelper(
              subTraverser,
              oldSubNode,
              newSubNode,
              DeltaElemTag::NOT_MINIMAL,
              std::forward<Func>(f));
        };

    if (traverser.shouldShortCircuit(VisitorType::DELTA)) {
      // Traverse helper says we don't need to recurse further
      //
      // TODO: use traversehelper in RecurseVisitor too to be able to
      // short circuit during the full recurse.
      return;
    }

    auto subtreeVisitOrder = (options.order == DeltaVisitOrder::PARENTS_FIRST)
        ? RecurseVisitOrder::PARENTS_FIRST
        : RecurseVisitOrder::CHILDREN_FIRST;
    RecurseVisitor<TC>::visit(
        traverser,
        target,
        RecurseVisitOptions(
            RecurseVisitMode::FULL,
            subtreeVisitOrder,
            options.outputIdPaths,
            options.recurseIntoHybridNodes),
        std::move(processChange));
  }

  if (options.order != DeltaVisitOrder::PARENTS_FIRST) {
    invokeVisitorFnHelper(
        traverser,
        oldNode,
        newNode,
        DeltaElemTag::MINIMAL,
        std::forward<Func>(f));
  }
}

template <typename TC, typename Node, typename TraverseHelper, typename Func>
void visitAddedOrRemovedNode(
    TraverseHelper& traverser,
    const std::optional<Node>& oldNode,
    const std::optional<Node>& newNode,
    const DeltaVisitOptions& /*mode*/,
    Func&& f) {
  // specialization for primitive node members
  CHECK(oldNode.has_value() != newNode.has_value());

  invokeVisitorFnHelper(
      traverser,
      oldNode,
      newNode,
      DeltaElemTag::MINIMAL,
      std::forward<Func>(f));
}

} // namespace dv_detail

/**
 * Set
 */
template <typename ValueTypeClass>
struct DeltaVisitor<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;
  template <typename Node, typename TraverseHelper, typename Func>
  static inline bool visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return dv_detail::visitNode<TC>(
        traverser, oldNode, newNode, options, std::forward<Func>(f));
  }

  template <typename Fields, typename TraverseHelper, typename Func>
  static bool visit(
      TraverseHelper& traverser,
      const Fields& oldFields,
      const Fields& newFields,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    // assuming that sets cannot contain any complex types, so just
    // calculating difference.
    bool hasDifferences{false};
    std::set_symmetric_difference(
        oldFields.begin(),
        oldFields.end(),
        newFields.begin(),
        newFields.end(),
        boost::make_function_output_iterator([&](const auto& val) {
          if (!val) {
            // shouldn't happen...
            return;
          }
          if (oldFields.count(val) && newFields.count(val)) {
            return;
          }

          hasDifferences = true;
          traverser.push(
              folly::to<std::string>(val->cref()), TCType<ValueTypeClass>);
          if (auto it = oldFields.find(val); it != oldFields.end()) {
            dv_detail::visitAddedOrRemovedNode<ValueTypeClass>(
                traverser,
                typename Fields::value_type{val},
                typename Fields::value_type{},
                options,
                std::forward<Func>(f));
          } else {
            dv_detail::visitAddedOrRemovedNode<ValueTypeClass>(
                traverser,
                typename Fields::value_type{},
                typename Fields::value_type{val},
                options,
                std::forward<Func>(f));
          }
          traverser.pop(TCType<ValueTypeClass>);
        }));

    return hasDifferences;
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct DeltaVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  using TC = apache::thrift::type_class::list<ValueTypeClass>;
  template <typename Node, typename TraverseHelper, typename Func>
  static inline bool visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return dv_detail::visitNode<TC>(
        traverser, oldNode, newNode, options, std::forward<Func>(f));
  }

  template <typename Fields, typename TraverseHelper, typename Func>
  static bool visit(
      TraverseHelper& traverser,
      const Fields& oldFields,
      const Fields& newFields,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    int minSize = std::min(oldFields.size(), newFields.size());

    bool hasDifferences{false};

    if (oldFields.size() > newFields.size()) { // entries removed
      hasDifferences = true;
      // loop in reverse order for removals
      for (int i = oldFields.size() - 1; i >= minSize; --i) {
        traverser.push(folly::to<std::string>(i), TCType<ValueTypeClass>);
        dv_detail::visitAddedOrRemovedNode<ValueTypeClass>(
            traverser,
            oldFields.at(i),
            typename Fields::value_type{},
            options,
            std::forward<Func>(f));
        traverser.pop(TCType<ValueTypeClass>);
      }
    } else if (oldFields.size() < newFields.size()) { // entries added
      hasDifferences = true;
      for (int i = minSize; i < newFields.size(); ++i) {
        traverser.push(folly::to<std::string>(i), TCType<ValueTypeClass>);
        dv_detail::visitAddedOrRemovedNode<ValueTypeClass>(
            traverser,
            typename Fields::value_type{},
            newFields.at(i),
            options,
            std::forward<Func>(f));
        traverser.pop(TCType<ValueTypeClass>);
      }
    }

    for (int i = 0; i < minSize; ++i) {
      const auto& oldRef = oldFields.at(i);
      const auto& newRef = newFields.at(i);
      if (oldRef != newRef) {
        traverser.push(folly::to<std::string>(i), TCType<ValueTypeClass>);
        if (DeltaVisitor<ValueTypeClass>::visit(
                traverser, oldRef, newRef, options, std::forward<Func>(f))) {
          hasDifferences = true;
        }
        traverser.pop(TCType<ValueTypeClass>);
      }
    }

    return hasDifferences;
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct DeltaVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;
  template <typename Node, typename TraverseHelper, typename Func>
  static inline bool visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return dv_detail::visitNode<TC>(
        traverser, oldNode, newNode, options, std::forward<Func>(f));
  }

  template <typename Fields, typename TraverseHelper, typename Func>
  static bool visit(
      TraverseHelper& traverser,
      const Fields& oldFields,
      const Fields& newFields,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    bool hasDifferences{false};

    // changed fields
    for (const auto& [key, val] : oldFields) {
      traverser.push(folly::to<std::string>(key), TCType<MappedTypeClass>);
      if (auto it = newFields.find(key); it != newFields.end()) {
        if (DeltaVisitor<MappedTypeClass>::visit(
                traverser, val, it->second, options, std::forward<Func>(f))) {
          hasDifferences = true;
        }
      } else {
        hasDifferences = true;
        dv_detail::visitAddedOrRemovedNode<MappedTypeClass>(
            traverser, val, decltype(val){}, options, std::forward<Func>(f));
      }
      traverser.pop(TCType<MappedTypeClass>);
    }

    for (const auto& [key, val] : newFields) {
      if (oldFields.find(key) == oldFields.end()) {
        // only look at keys that didn't exist. First loop should handle all
        // replacement deltas
        hasDifferences = true;
        traverser.push(folly::to<std::string>(key), TCType<MappedTypeClass>);

        dv_detail::visitAddedOrRemovedNode<MappedTypeClass>(
            traverser, decltype(val){}, val, options, std::forward<Func>(f));

        traverser.pop(TCType<MappedTypeClass>);
      }
    }

    return hasDifferences;
  }
};

/**
 * Variant
 */
template <>
struct DeltaVisitor<apache::thrift::type_class::variant> {
  using TC = apache::thrift::type_class::variant;
  template <typename Node, typename TraverseHelper, typename Func>
  static inline bool visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return dv_detail::visitNode<TC>(
        traverser, oldNode, newNode, options, std::forward<Func>(f));
  }

  template <typename Fields, typename TraverseHelper, typename Func>
  static bool visit(
      TraverseHelper& traverser,
      const Fields& oldFields,
      const Fields& newFields,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using Members = typename Fields::Members;

    bool hasDifferences{false};

    bool wasSet = oldFields.type() != Fields::TypeEnum::__EMPTY__;
    bool isSet = newFields.type() != Fields::TypeEnum::__EMPTY__;

    if (oldFields.type() != newFields.type()) {
      hasDifferences = true;
      if (wasSet) {
        fatal::scalar_search<Members, fatal::get_type::id>(
            oldFields.type(), [&](auto indexed) {
              using descriptor = decltype(fatal::tag_type(indexed));
              using name = typename descriptor::metadata::name;
              using tc = typename descriptor::metadata::type_class;

              std::string memberName =
                  getMemberName<typename descriptor::metadata>(
                      options.outputIdPaths);

              traverser.push(std::move(memberName), TCType<tc>);

              const auto& oldRef = oldFields.template cref<name>();

              dv_detail::visitAddedOrRemovedNode<tc>(
                  traverser,
                  oldRef,
                  // default constructed member type
                  decltype(oldRef){},
                  options,
                  std::forward<Func>(f));

              traverser.pop(TCType<tc>);
            });
      }
      if (isSet) {
        fatal::scalar_search<Members, fatal::get_type::id>(
            newFields.type(), [&](auto indexed) {
              using descriptor = decltype(fatal::tag_type(indexed));
              using name = typename descriptor::metadata::name;
              using tc = typename descriptor::metadata::type_class;

              std::string memberName =
                  getMemberName<typename descriptor::metadata>(
                      options.outputIdPaths);

              traverser.push(std::move(memberName), TCType<tc>);

              const auto& newRef = newFields.template cref<name>();

              dv_detail::visitAddedOrRemovedNode<tc>(
                  traverser,
                  // default constructed member type
                  decltype(newRef){},
                  newRef,
                  options,
                  std::forward<Func>(f));

              traverser.pop(TCType<tc>);
            });
      }
    } else {
      fatal::scalar_search<Members, fatal::get_type::id>(
          oldFields.type(), [&](auto indexed) {
            using descriptor = decltype(fatal::tag_type(indexed));
            using name = typename descriptor::metadata::name;
            using tc = typename descriptor::metadata::type_class;

            std::string memberName =
                getMemberName<typename descriptor::metadata>(
                    options.outputIdPaths);

            traverser.push(std::move(memberName), TCType<tc>);
            hasDifferences = DeltaVisitor<tc>::visit(
                traverser,
                oldFields.template cref<name>(),
                newFields.template cref<name>(),
                options,
                std::forward<Func>(f));
            traverser.pop(TCType<tc>);
          });
    }

    return hasDifferences;
  }
};

/**
 * Structure
 */
template <>
struct DeltaVisitor<apache::thrift::type_class::structure> {
  using TC = apache::thrift::type_class::structure;
  template <typename Node, typename Func>
  static bool visit(
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const DeltaVisitOptions& options,
      Func&& f) {
    SimpleTraverseHelper traverser;
    return visit(traverser, oldNode, newNode, options, std::forward<Func>(f));
  }

  template <typename Node, typename TraverseHelper, typename Func>
  static inline bool visit(
      TraverseHelper& traverser,
      const std::shared_ptr<Node>& oldNode,
      const std::shared_ptr<Node>& newNode,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Node types
    requires(std::is_same_v<typename Node::CowType, NodeType>)
  {
    return dv_detail::visitNode<TC>(
        traverser, oldNode, newNode, options, std::forward<Func>(f));
  }

  template <typename Fields, typename TraverseHelper, typename Func>
  static bool visit(
      TraverseHelper& traverser,
      const Fields& oldFields,
      const Fields& newFields,
      const DeltaVisitOptions& options,
      Func&& f)
      // only enable for Fields types
    requires(std::is_same_v<typename Fields::CowType, FieldsType>)
  {
    using Members = typename Fields::Members;

    bool hasDifferences{false};

    fatal::foreach<Members>([&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using name = typename member::name;
      using tc = typename member::type_class;

      // Look for the expected member name
      std::string memberName = getMemberName<member>(options.outputIdPaths);

      traverser.push(std::move(memberName), TCType<tc>);
      SCOPE_EXIT {
        traverser.pop(TCType<tc>);
      };

      const auto& oldRef = oldFields.template cref<name>();
      const auto& newRef = newFields.template cref<name>();

      // Check for optionality
      if (member::optional::value == apache::thrift::optionality::optional) {
        if (!oldRef && !newRef) {
          return;
        } else if (!oldRef || !newRef) {
          hasDifferences = true;
          dv_detail::visitAddedOrRemovedNode<tc>(
              traverser, oldRef, newRef, options, std::forward<Func>(f));
          return;
        }
      }

      // Recurse further if pointer has changed
      if (DeltaVisitor<tc>::visit(
              traverser, oldRef, newRef, options, std::forward<Func>(f))) {
        hasDifferences = true;
      }
    });

    return hasDifferences;
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
struct DeltaVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Fields, typename TraverseHelper, typename Func>
  static bool visit(
      TraverseHelper& traverser,
      const Fields& oldFields,
      const Fields& newFields,
      const DeltaVisitOptions& /*mode*/,
      Func&& f) {
    if (oldFields != newFields) {
      dv_detail::invokeVisitorFnHelper(
          traverser,
          oldFields,
          newFields,
          DeltaElemTag::MINIMAL,
          std::forward<Func>(f));
      return true;
    }

    return false;
  }
};

using RootDeltaVisitor = DeltaVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::thrift_cow
