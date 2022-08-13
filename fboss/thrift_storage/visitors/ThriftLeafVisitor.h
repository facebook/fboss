// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <algorithm>

#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::fsdb {

template <typename>
struct ThriftLeafVisitor;

/**
 * Enumeration
 */
template <>
struct ThriftLeafVisitor<apache::thrift::type_class::enumeration> {
  template <typename Node, typename Func>
  static void
  visit(std::vector<std::string>& path, const Node& node, Func&& f) {
    using TC = apache::thrift::type_class::enumeration;
    f(path, TC{}, node);
  }
};

/**
 * Set
 */
template <typename ValueTypeClass>
struct ThriftLeafVisitor<apache::thrift::type_class::set<ValueTypeClass>> {
  template <typename Node, typename Func>
  static void
  visit(std::vector<std::string>& path, const Node& node, Func&& f) {
    for (auto& val : node) {
      path.push_back(folly::to<std::string>(val));
      f(path, ValueTypeClass{}, val);
      path.pop_back();
    }
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct ThriftLeafVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  template <typename Node, typename Func>
  static void
  visit(std::vector<std::string>& path, const Node& node, Func&& f) {
    for (int i = 0; i < node.size(); ++i) {
      path.push_back(folly::to<std::string>(i));
      ThriftLeafVisitor<ValueTypeClass>::visit(
          path, node.at(i), std::forward<Func>(f));
      path.pop_back();
    }
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct ThriftLeafVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename Node, typename Func>
  static void
  visit(std::vector<std::string>& path, const Node& node, Func&& f) {
    for (const auto& [key, val] : node) {
      path.push_back(folly::to<std::string>(key));
      ThriftLeafVisitor<MappedTypeClass>::visit(
          path, val, std::forward<Func>(f));
      path.pop_back();
    }
  }
};

/**
 * Variant
 */
template <>
struct ThriftLeafVisitor<apache::thrift::type_class::variant> {
  template <typename Node, typename Func>
  static void
  visit(std::vector<std::string>& path, const Node& node, Func&& f) {
    using descriptors = typename apache::thrift::reflect_variant<
        folly::remove_cvref_t<Node>>::traits::descriptors;

    fatal::scalar_search<descriptors, fatal::get_type::id>(
        node.getType(), [&](auto indexed) {
          using descriptor = decltype(fatal::tag_type(indexed));

          const std::string memberName(
              fatal::z_data<typename descriptor::metadata::name>(),
              fatal::size<typename descriptor::metadata::name>::value);

          path.push_back(memberName);
          ThriftLeafVisitor<typename descriptor::metadata::type_class>::visit(
              path, typename descriptor::getter()(node), std::forward<Func>(f));
          path.pop_back();
        });
  }
};

/**
 * Structure
 */
template <>
struct ThriftLeafVisitor<apache::thrift::type_class::structure> {
  template <typename Node, typename Func>
  static void visit(const Node& node, Func&& f) {
    std::vector<std::string> path;
    visit(path, node, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static void
  visit(std::vector<std::string>& path, const Node& node, Func&& f) {
    fatal::foreach<typename apache::thrift::reflect_struct<
        folly::remove_cvref_t<Node>>::members>([&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));

      // Look for the expected member name
      const std::string memberName(
          fatal::z_data<typename member::name>(),
          fatal::size<typename member::name>::value);

      path.push_back(memberName);

      // Recurse further
      ThriftLeafVisitor<typename member::type_class>::visit(
          path, typename member::getter{}(node), std::forward<Func>(f));

      path.pop_back();
    });
  }
};

/**
 * Primitives - fallback specialization
 * - string / binary
 * - floating_point
 * - integral
 */
template <typename TC>
struct ThriftLeafVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Node, typename Func>
  static void
  visit(std::vector<std::string>& path, const Node& node, Func&& f) {
    f(path, TC{}, node);
  }
};

using RootThriftLeafVisitor =
    ThriftLeafVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::fsdb
