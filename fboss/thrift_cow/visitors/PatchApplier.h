// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

#include <optional>

#pragma once

namespace facebook::fboss::thrift_cow {

enum class PatchResult {
  OK,
  INVALID_PATCH_TYPE,
  NON_EXISTENT_NODE,
  KEY_PARSE_ERROR,
};

template <typename TC>
struct PatchApplier;

struct NodeType;
struct FieldsType;

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
  static inline PatchResult apply(
      std::shared_ptr<Node>& node,
      PatchNode&& patch) {
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
        // TODO: create if does not exist?
        if (auto child = node->find(parsedKey.value()); child != node->end()) {
          auto res = PatchApplier<MappedTypeClass>::apply(
              child->second, std::forward<PatchNode>(childPatch));
          // Continue patching even if there is an error, but still return an
          // error if encountered
          if (res != PatchResult::OK) {
            result = res;
          }
        } else {
          result = PatchResult::NON_EXISTENT_NODE;
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
  static inline PatchResult apply(
      std::shared_ptr<Node>& /*node*/,
      PatchNode&& patch) {
    // TODO: handle val
    if (patch.getType() != PatchNode::Type::list_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }
    return PatchResult::OK;
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
  static inline PatchResult apply(
      std::shared_ptr<Node>& /*node*/,
      PatchNode&& patch) {
    // TODO: handle val
    if (patch.getType() != PatchNode::Type::set_node) {
      return PatchResult::INVALID_PATCH_TYPE;
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
  static inline PatchResult apply(
      std::shared_ptr<Node>& /*node*/,
      PatchNode&& patch) {
    if (patch.getType() != PatchNode::Type::variant_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }
    return PatchResult::OK;
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
  static inline PatchResult apply(
      std::shared_ptr<Node>& node,
      PatchNode&& patch) {
    // TODO: handle val
    if (patch.getType() != PatchNode::Type::struct_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }

    using Fields = typename Node::Fields;
    PatchResult result = PatchResult::OK;
    auto structPatch = patch.move_struct_node();
    for (auto&& [key, childPatch] : *std::move(structPatch).children()) {
      fatal::scalar_search<typename Fields::Members, fatal::get_type::id>(
          key, [&, childPatch = std::move(childPatch)](auto indexed) mutable {
            using member = decltype(fatal::tag_type(indexed));
            using name = typename member::name;
            using tc = typename member::type_class;

            auto& child = node->template modify<name>(&node);
            auto res = PatchApplier<tc>::apply(child, std::move(childPatch));
            // Continue patching even if there is an error, but still return an
            // error if encountered
            if (res != PatchResult::OK) {
              result = res;
            }
          });
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
  static PatchResult apply(std::optional<Fields>& fields, PatchNode&& patch) {
    if (patch.getType() != PatchNode::Type::val) {
      return PatchResult::INVALID_PATCH_TYPE;
    }
    // TODO: construct if nullopt?
    fields->fromEncodedBuf(fsdb::OperProtocol::COMPACT, patch.move_val());
    return PatchResult::OK;
  }
};
} // namespace facebook::fboss::thrift_cow
