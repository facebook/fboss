// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/gen-cpp2/patch_types.h>

#pragma once

namespace facebook::fboss::thrift_cow {

enum class PatchResult {
  OK,
  INVALID_PATCH_TYPE,
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
      std::shared_ptr<Node>& /*node*/,
      PatchNode&& patch) {
    if (patch.getType() != PatchNode::Type::map_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }
    return PatchResult::OK;
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
      std::shared_ptr<Node>& /*node*/,
      PatchNode&& patch) {
    // TODO: handle val
    if (patch.getType() != PatchNode::Type::struct_node) {
      return PatchResult::INVALID_PATCH_TYPE;
    }
    return PatchResult::OK;
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
  static PatchResult visit(Fields& /*fields*/, PatchNode&& patch) {
    if (patch.getType() != PatchNode::Type::val) {
      return PatchResult::INVALID_PATCH_TYPE;
    }
    return PatchResult::OK;
  }
};
} // namespace facebook::fboss::thrift_cow
