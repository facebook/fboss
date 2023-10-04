// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

#include <optional>

#pragma once

namespace facebook::fboss::thrift_cow {

enum class PatchResult {
  OK,
  INVALID_STRUCT_MEMBER,
  INVALID_VARIANT_MEMBER,
  INVALID_PATCH_TYPE,
  NON_EXISTENT_NODE,
  KEY_PARSE_ERROR,
  PATCHING_IMMUTABLE_NODE,
};

template <typename TC>
struct PatchApplier;

struct NodeType;
struct FieldsType;

namespace pa_detail {
template <typename Node>
PatchResult patchNode(Node& n, ByteBuffer&& buf) {
  n.fromEncodedBuf(fsdb::OperProtocol::COMPACT, std::move(buf));
  return PatchResult::OK;
}
} // namespace pa_detail

/*
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct PatchApplier<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;

  template <
      typename Node,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline PatchResult apply(Node& node, PatchNode&& patch) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode(node, patch.move_val());
    }
    if (patch.getType() != PatchNode::Type::map_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }

    using Fields = typename Node::Fields;
    using key_type = typename Fields::key_type;

    auto parseKey = [](auto&& key) {
      key_type parsedKey;
      if constexpr (std::is_same_v<
                        KeyTypeClass,
                        apache::thrift::type_class::enumeration>) {
        if (fatal::enum_traits<key_type>::try_parse(
                parsedKey, std::move(key))) {
          return std::make_optional(parsedKey);
        }
      } else {
        auto keyTry = folly::tryTo<key_type>(std::move(key));
        if (keyTry.hasValue()) {
          return std::make_optional(keyTry.value());
        }
      }
      return std::optional<key_type>();
    };

    PatchResult result = PatchResult::OK;

    auto mapPatch = patch.move_map_node();
    for (auto&& [key, childPatch] : *std::move(mapPatch).children()) {
      if (auto parsedKey = parseKey(key)) {
        if (childPatch.getType() == PatchNode::Type::del) {
          node.remove(std::move(*parsedKey));
        } else {
          node.modifyImpl(*parsedKey);

          auto res = PatchApplier<MappedTypeClass>::apply(
              *node.ref(std::move(*parsedKey)), std::move(childPatch));
          // Continue patching even if there is an error, but still return an
          // error if encountered
          if (res != PatchResult::OK) {
            result = res;
          }
        }
      } else {
        result = PatchResult::KEY_PARSE_ERROR;
      }
    }

    return result;
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct PatchApplier<apache::thrift::type_class::list<ValueTypeClass>> {
  using TC = apache::thrift::type_class::list<ValueTypeClass>;

  template <
      typename Node,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline PatchResult apply(Node& node, PatchNode&& patch) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode(node, patch.move_val());
    }
    if (patch.getType() != PatchNode::Type::list_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }

    PatchResult result = PatchResult::OK;

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
      auto& childPatch = listPatch.children()->at(index);
      if (childPatch.getType() == PatchNode::Type::del) {
        node.remove(index);
        continue;
      }
      node.modify(index);
      auto res = PatchApplier<ValueTypeClass>::apply(
          *node.ref(index), std::move(std::move(childPatch)));
      if (res != PatchResult::OK) {
        result = res;
      }
    }

    return result;
  }
};

/**
 * Set
 */
template <typename ValueTypeClass>
struct PatchApplier<apache::thrift::type_class::set<ValueTypeClass>> {
  using TC = apache::thrift::type_class::set<ValueTypeClass>;
  template <
      typename Node,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline PatchResult apply(Node& node, PatchNode&& patch) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode(node, patch.move_val());
    }
    if (patch.getType() != PatchNode::Type::set_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }

    using ValueTType = typename Node::ValueTType;

    auto parseKey = [](auto&& key) {
      ValueTType parsedKey;
      if constexpr (std::is_same_v<
                        ValueTType,
                        apache::thrift::type_class::enumeration>) {
        if (fatal::enum_traits<ValueTType>::try_parse(
                parsedKey, std::move(key))) {
          return std::make_optional(parsedKey);
        }
      } else {
        auto keyTry = folly::tryTo<ValueTType>(std::move(key));
        if (keyTry.hasValue()) {
          return std::make_optional(std::move(keyTry.value()));
        }
      }
      return std::optional<ValueTType>();
    };

    PatchResult result = PatchResult::OK;

    auto setPatch = patch.move_set_node();
    for (auto&& [key, childPatch] : *std::move(setPatch).children()) {
      // for sets keys are our values
      std::optional<ValueTType> value = parseKey(key);
      if (!value) {
        result = PatchResult::KEY_PARSE_ERROR;
        break;
      }
      // We only support sets of primitives
      // lets not recurse and just handle add/remove here
      if (childPatch.getType() == PatchNode::Type::del) {
        node.erase(std::move(*value));
      } else if (childPatch.getType() == PatchNode::Type::val) {
        node.emplace(std::move(*value));
      } else {
        result = PatchResult::INVALID_PATCH_TYPE;
      }
    }

    return PatchResult::OK;
  }
};

/**
 * Variant
 */
template <>
struct PatchApplier<apache::thrift::type_class::variant> {
  using TC = apache::thrift::type_class::variant;

  template <
      typename Node,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline PatchResult apply(Node& node, PatchNode&& patch) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode(node, patch.move_val());
    }
    if (patch.getType() != PatchNode::Type::variant_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }

    PatchResult result = PatchResult::INVALID_VARIANT_MEMBER;

    using Fields = typename Node::Fields;
    using Members = typename Fields::MemberTypes;

    auto variantPatch = patch.variant_node_ref();
    auto key = *variantPatch->id();

    fatal::scalar_search<Members, fatal::get_type::id>(key, [&](auto tag) {
      using descriptor = typename decltype(fatal::tag_type(tag))::member;
      using name = typename descriptor::metadata::name;
      using tc = typename descriptor::metadata::type_class;

      node.template modify<name>();
      auto& child = node.template ref<name>();

      if (!child) {
        // child is unset, cannot traverse through missing optional child
        result = PatchResult::NON_EXISTENT_NODE;
        return;
      }
      result =
          PatchApplier<tc>::apply(*child, std::move(*variantPatch->child()));
    });

    return result;
  }
};

/**
 * Structure
 */
template <>
struct PatchApplier<apache::thrift::type_class::structure> {
  using TC = apache::thrift::type_class::structure;

  template <
      typename Node,
      // only enable for Node types
      std::enable_if_t<std::is_same_v<typename Node::CowType, NodeType>, bool> =
          true>
  static inline PatchResult apply(Node& node, PatchNode&& patch) {
    if (patch.getType() == PatchNode::Type::val) {
      return pa_detail::patchNode(node, patch.move_val());
    }
    if (patch.getType() != PatchNode::Type::struct_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }

    using Fields = typename Node::Fields;
    PatchResult result = PatchResult::OK;
    auto structPatch = patch.move_struct_node();
    for (auto&& [key, childPatch] : *std::move(structPatch).children()) {
      auto invalidMember = true;
      fatal::scalar_search<typename Fields::Members, fatal::get_type::id>(
          key, [&, childPatch = std::move(childPatch)](auto indexed) mutable {
            invalidMember = false;
            using member = decltype(fatal::tag_type(indexed));
            using name = typename member::name;
            using tc = typename member::type_class;

            if (childPatch.getType() == PatchNode::Type::del) {
              node.template remove<name>();
              return;
            }

            auto& child = node.template modify<name>();
            auto res = PatchApplier<tc>::apply(*child, std::move(childPatch));
            // Continue patching even if there is an error, but still return an
            // error if encountered
            if (res != PatchResult::OK) {
              result = res;
            }
          });
      if (invalidMember) {
        result = PatchResult::INVALID_STRUCT_MEMBER;
      }
    }
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
struct PatchApplier {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Fields>
  static PatchResult apply(Fields& fields, PatchNode&& patch) {
    if (patch.getType() != PatchNode::Type::val) {
      return PatchResult::INVALID_PATCH_TYPE;
    }
    // This can only happen if we are trying to apply a patch a set entry which
    // are immutable. In practice this should never happen because for sets we
    // always patch at the set level, though it is technically possible because
    // we allow for patching at a base path, which could technically be a set
    // entry
    if constexpr (Fields::immutable) {
      return PatchResult::PATCHING_IMMUTABLE_NODE;
    } else {
      return pa_detail::patchNode(fields, patch.move_val());
    }
  }
};

using RootPatchApplier = PatchApplier<apache::thrift::type_class::structure>;
} // namespace facebook::fboss::thrift_cow
