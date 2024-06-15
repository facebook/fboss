// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <fboss/thrift_cow/nodes/NodeUtils.h>
#include <fboss/thrift_cow/nodes/Serializer.h>
#include <fboss/thrift_cow/visitors/PatchHelpers.h>
#include <fboss/thrift_cow/visitors/gen-cpp2/results_types.h>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include <optional>

#pragma once

namespace facebook::fboss::thrift_cow {

template <typename TC>
struct PatchApplier;

struct NodeType;
struct FieldsType;

namespace pa_detail {

template <typename TC, typename Node>
inline PatchApplyResult
patchNode(Node& n, ByteBuffer&& buf, fsdb::OperProtocol protocol) {
  if constexpr (is_cow_type_v<Node>) {
    n.fromEncodedBuf(protocol, std::move(buf));
  } else {
    n = deserializeBuf<TC, Node>(protocol, std::move(buf));
  }
  return PatchApplyResult::OK;
}
} // namespace pa_detail

class PatchTraverser {
 public:
  void push(int tok);
  void push(std::string tok);
  PatchApplyResult traverseResult(const PatchApplyResult& result);
  void pop();

  PatchApplyResult currentResult() {
    return curResult_;
  }

 private:
  std::vector<std::string> curPath_;
  PatchApplyResult curResult_{PatchApplyResult::OK};
};

/*
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct PatchApplier<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;

  template <typename Node>
  static inline PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol = fsdb::OperProtocol::COMPACT) {
    PatchTraverser traverser;
    return apply(node, std::move(patch), protocol, traverser);
  }

  template <typename Node>
  static PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol,
      PatchTraverser& traverser) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode<TC>(node, patch.move_val(), protocol);
    }
    if (patch.getType() != PatchNode::Type::map_node) {
      return PatchApplyResult::INVALID_PATCH_TYPE;
    }

    using Fields = typename Node::Fields;
    using key_type = typename Fields::key_type;

    decompressPatch(patch);
    auto mapPatch = patch.move_map_node();
    for (auto&& [key, childPatch] : *std::move(mapPatch).children()) {
      traverser.push(key);
      if (auto parsedKey = tryParseKey<key_type, KeyTypeClass>(key)) {
        if (childPatch.getType() == PatchNode::Type::del) {
          node.remove(std::move(*parsedKey));
        } else {
          node.modifyTyped(*parsedKey);
          traverser.traverseResult(PatchApplier<MappedTypeClass>::apply(
              *node.ref(std::move(*parsedKey)),
              std::move(childPatch),
              protocol,
              traverser));
        }
      } else {
        traverser.traverseResult(PatchApplyResult::KEY_PARSE_ERROR);
      }
      traverser.pop();
    }

    return traverser.currentResult();
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct PatchApplier<apache::thrift::type_class::list<ValueTypeClass>> {
  using TC = apache::thrift::type_class::list<ValueTypeClass>;

  template <typename Node>
  static inline PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol = fsdb::OperProtocol::COMPACT) {
    PatchTraverser traverser;
    return apply(node, std::move(patch), protocol, traverser);
  }

  template <typename Node>
  static PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol,
      PatchTraverser& traverser) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode<TC>(node, patch.move_val(), protocol);
    }
    if (patch.getType() != PatchNode::Type::list_node) {
      return PatchApplyResult::INVALID_PATCH_TYPE;
    }

    decompressPatch(patch);
    auto listPatch = patch.move_list_node();

    // In case of removals, we want to make sure we resolve later indices first.
    // So first we need to sort the indices
    std::vector<int> indices;
    indices.reserve(listPatch.children()->size());
    for (const auto& pair : *listPatch.children()) {
      indices.push_back(pair.first);
    }
    std::sort(indices.begin(), indices.end(), std::greater<>());

    // Iterate through sorted keys and access values in the map
    for (const int index : indices) {
      traverser.push(index);
      auto& childPatch = listPatch.children()->at(index);
      if (childPatch.getType() == PatchNode::Type::del) {
        node.remove(index);
      } else {
        node.modify(index);
        traverser.traverseResult(PatchApplier<ValueTypeClass>::apply(
            *node.ref(index),
            std::move(std::move(childPatch)),
            protocol,
            traverser));
      }
      traverser.pop();
    }

    return traverser.currentResult();
  }
};

/**
 * Set
 */
template <typename ValueTypeClass>
struct PatchApplier<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;

  template <typename Node>
  static inline PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol = fsdb::OperProtocol::COMPACT) {
    PatchTraverser traverser;
    return apply(node, std::move(patch), protocol, traverser);
  }

  template <typename Node>
  static PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol,
      PatchTraverser& traverser) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode<TC>(node, patch.move_val(), protocol);
    }
    if (patch.getType() != PatchNode::Type::set_node) {
      return PatchApplyResult::INVALID_PATCH_TYPE;
    }

    using ValueTType = typename Node::ValueTType;

    decompressPatch(patch);
    auto setPatch = patch.move_set_node();
    for (auto&& [key, childPatch] : *std::move(setPatch).children()) {
      traverser.push(key);
      // for sets keys are our values
      std::optional<ValueTType> value =
          tryParseKey<ValueTType, ValueTypeClass>(key);
      if (!value) {
        traverser.traverseResult(PatchApplyResult::KEY_PARSE_ERROR);
      } else {
        // We only support sets of primitives
        // lets not recurse and just handle add/remove here
        if (childPatch.getType() == PatchNode::Type::del) {
          node.erase(std::move(*value));
        } else if (childPatch.getType() == PatchNode::Type::val) {
          node.emplace(std::move(*value));
        } else {
          traverser.traverseResult(PatchApplyResult::INVALID_PATCH_TYPE);
        }
      }
      traverser.pop();
    }

    return traverser.currentResult();
  }
};

/**
 * Variant
 */
template <>
struct PatchApplier<apache::thrift::type_class::variant> {
  using TC = apache::thrift::type_class::variant;

  template <typename Node>
  static inline PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol = fsdb::OperProtocol::COMPACT) {
    PatchTraverser traverser;
    return apply(node, std::move(patch), protocol, traverser);
  }

  template <typename Node>
  static PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol,
      PatchTraverser& traverser) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode<TC>(node, patch.move_val(), protocol);
    }
    if (patch.getType() != PatchNode::Type::variant_node) {
      return PatchApplyResult::INVALID_PATCH_TYPE;
    }

    using Fields = typename Node::Fields;
    using Members = typename Fields::MemberTypes;

    auto variantPatch = patch.variant_node_ref();
    auto key = *variantPatch->id();
    traverser.push(key);
    auto found =
        fatal::scalar_search<Members, fatal::get_type::id>(key, [&](auto tag) {
          using descriptor = typename decltype(fatal::tag_type(tag))::member;
          using name = typename descriptor::metadata::name;
          using tc = typename descriptor::metadata::type_class;

          node.template modify<name>();
          auto& child = node.template ref<name>();

          if (!child) {
            // child is unset, cannot traverse through missing optional child
            traverser.traverseResult(PatchApplyResult::NON_EXISTENT_NODE);
          } else {
            traverser.traverseResult(PatchApplier<tc>::apply(
                *child,
                std::move(*variantPatch->child()),
                protocol,
                traverser));
          }
        });
    if (!found) {
      traverser.traverseResult(PatchApplyResult::INVALID_VARIANT_MEMBER);
    }
    traverser.pop();

    return traverser.currentResult();
  }
};

/**
 * Structure
 */
template <>
struct PatchApplier<apache::thrift::type_class::structure> {
  using TC = apache::thrift::type_class::structure;

  template <typename Node>
  static inline PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol = fsdb::OperProtocol::COMPACT) {
    PatchTraverser traverser;
    return apply(node, std::move(patch), protocol, traverser);
  }

  template <typename Node>
  static PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol,
      PatchTraverser& traverser) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode<TC>(node, patch.move_val(), protocol);
    }
    if (patch.getType() != PatchNode::Type::struct_node) {
      return PatchApplyResult::INVALID_PATCH_TYPE;
    }

    using Fields = typename Node::Fields;
    decompressPatch(patch);
    auto structPatch = patch.move_struct_node();
    for (auto&& [key, childPatch] : *std::move(structPatch).children()) {
      auto found =
          fatal::scalar_search<typename Fields::Members, fatal::get_type::id>(
              key,
              [&, childPatch = std::move(childPatch)](auto indexed) mutable {
                using member = decltype(fatal::tag_type(indexed));
                using name = typename member::name;
                using tc = typename member::type_class;

                if (childPatch.getType() == PatchNode::Type::del) {
                  node.template remove<name>();
                  return;
                }

                auto& child = node.template modify<name>();
                traverser.traverseResult(PatchApplier<tc>::apply(
                    *child, std::move(childPatch), protocol, traverser));
              });
      if (!found) {
        traverser.traverseResult(PatchApplyResult::INVALID_STRUCT_MEMBER);
      }
    }
    return traverser.currentResult();
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
struct PatchApplier {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Node>
  static inline PatchApplyResult apply(
      Node& node,
      PatchNode&& patch,
      fsdb::OperProtocol protocol = fsdb::OperProtocol::COMPACT) {
    PatchTraverser traverser;
    return apply(node, std::move(patch), protocol, traverser);
  }

  template <typename Fields>
  static PatchApplyResult apply(
      Fields& fields,
      PatchNode&& patch,
      fsdb::OperProtocol protocol,
      PatchTraverser& /* traverser */) {
    if (patch.getType() != PatchNode::Type::val) {
      return PatchApplyResult::INVALID_PATCH_TYPE;
    }
    if constexpr (is_cow_type_v<Fields>) {
      if constexpr (Fields::immutable) {
        return PatchApplyResult::PATCHING_IMMUTABLE_NODE;
      }
    }
    return pa_detail::patchNode<TC>(fields, patch.move_val(), protocol);
  }
};

using RootPatchApplier = PatchApplier<apache::thrift::type_class::structure>;
} // namespace facebook::fboss::thrift_cow
