// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <algorithm>

#include <boost/function_output_iterator.hpp>

#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::fsdb {

/*
 * This visitor takes two thrift objects, comparing each field and
 * then running the provided function against any paths that differ
 * between the two objects.
 *
 * This can be used to find changes between two versions of the state
 * so that we can serve any related subscriptions.
 *
 * NOTE: this is not super efficient right now. In particular, we use
 * operator== to recursively compare the objects, which can be
 * nontrivially expensive.
 */

template <typename>
struct ThriftDeltaVisitor;

/**
 * Enumeration
 */
template <>
struct ThriftDeltaVisitor<apache::thrift::type_class::enumeration> {
  template <typename Node, typename Func>
  static bool visit(
      std::vector<std::string>& path,
      const Node& oldNode,
      const Node& newNode,
      Func&& f) {
    using TC = apache::thrift::type_class::enumeration;

    if (oldNode != newNode) {
      f(path, TC{}, std::make_optional(oldNode), std::make_optional(newNode));
      return true;
    }

    return false;
  }
};

/**
 * Set
 */
template <typename ValueTypeClass>
struct ThriftDeltaVisitor<apache::thrift::type_class::set<ValueTypeClass>> {
  template <typename Node, typename Func>
  static bool visit(
      std::vector<std::string>& path,
      const Node& oldNode,
      const Node& newNode,
      Func&& f) {
    // assuming that sets cannot contain any complex types, so just
    // calculating difference.
    bool hasDifferences{false};
    std::set_symmetric_difference(
        oldNode.begin(),
        oldNode.end(),
        newNode.begin(),
        newNode.end(),
        boost::make_function_output_iterator([&](const auto& val) {
          hasDifferences = true;
          path.push_back(folly::to<std::string>(val));
          if (std::find(oldNode.begin(), oldNode.end(), val) != oldNode.end()) {
            f(path,
              ValueTypeClass{},
              std::make_optional(val),
              std::optional<folly::remove_cvref_t<decltype(val)>>());
          } else {
            f(path,
              ValueTypeClass{},
              std::optional<folly::remove_cvref_t<decltype(val)>>(),
              std::make_optional(val));
          }
          path.pop_back();
        }));

    if (hasDifferences) {
      using TC = apache::thrift::type_class::set<ValueTypeClass>;
      f(path, TC{}, std::make_optional(oldNode), std::make_optional(newNode));
    }
    return hasDifferences;
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct ThriftDeltaVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  template <typename Node, typename Func>
  static bool visit(
      std::vector<std::string>& path,
      const Node& oldNode,
      const Node& newNode,
      Func&& f) {
    size_t minSize = std::min(oldNode.size(), newNode.size());

    bool hasDifferences{false};

    if (oldNode.size() > newNode.size()) {
      hasDifferences = true;
      for (int i = minSize; i < oldNode.size(); ++i) {
        path.push_back(folly::to<std::string>(i));
        f(path,
          ValueTypeClass{},
          std::make_optional(oldNode.at(i)),
          std::optional<typename Node::value_type>());
        path.pop_back();
      }
    } else if (oldNode.size() < newNode.size()) {
      hasDifferences = true;
      for (int i = minSize; i < newNode.size(); ++i) {
        path.push_back(folly::to<std::string>(i));
        f(path,
          ValueTypeClass{},
          std::optional<typename Node::value_type>(),
          std::make_optional(newNode.at(i)));
        path.pop_back();
      }
    }

    for (int i = 0; i < minSize; ++i) {
      path.push_back(folly::to<std::string>(i));
      auto differs = ThriftDeltaVisitor<ValueTypeClass>::visit(
          path, oldNode.at(i), newNode.at(i), std::forward<Func>(f));
      hasDifferences = hasDifferences || differs;
      path.pop_back();
    }

    if (hasDifferences) {
      using TC = apache::thrift::type_class::list<ValueTypeClass>;
      f(path, TC{}, std::make_optional(oldNode), std::make_optional(newNode));
    }

    return hasDifferences;
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct ThriftDeltaVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename Node, typename Func>
  static bool visit(
      std::vector<std::string>& path,
      const Node& oldNode,
      const Node& newNode,
      Func&& f) {
    bool hasDifferences{false};

    for (const auto& [key, val] : oldNode) {
      path.push_back(folly::to<std::string>(key));
      if (auto it = newNode.find(key); it != newNode.end()) {
        bool different = ThriftDeltaVisitor<MappedTypeClass>::visit(
            path, val, it->second, std::forward<Func>(f));
        hasDifferences = hasDifferences || different;
      } else {
        hasDifferences = true;
        f(path,
          MappedTypeClass{},
          std::make_optional(val),
          std::optional<folly::remove_cvref_t<decltype(val)>>());
      }
      path.pop_back();
    }

    for (const auto& [key, val] : newNode) {
      if (oldNode.find(key) == oldNode.end()) {
        // only look at keys that didn't exist. First loop should handle all
        // replacement deltas
        hasDifferences = true;
        path.push_back(folly::to<std::string>(key));
        f(path,
          MappedTypeClass{},
          std::optional<folly::remove_cvref_t<decltype(val)>>(),
          std::make_optional(val));
        path.pop_back();
      }
    }

    if (hasDifferences) {
      using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;
      f(path, TC{}, std::make_optional(oldNode), std::make_optional(newNode));
    }

    return hasDifferences;
  }
};

/**
 * Variant
 */
template <>
struct ThriftDeltaVisitor<apache::thrift::type_class::variant> {
  template <typename Node, typename Func>
  static bool visit(
      std::vector<std::string>& path,
      const Node& oldNode,
      const Node& newNode,
      Func&& f) {
    bool hasDifferences{false};

    using descriptors = typename apache::thrift::reflect_variant<
        folly::remove_cvref_t<Node>>::traits::descriptors;

    if (oldNode.getType() != newNode.getType()) {
      hasDifferences = true;
      fatal::scalar_search<descriptors, fatal::get_type::id>(
          oldNode.getType(), [&](auto indexed) {
            using descriptor = decltype(fatal::tag_type(indexed));

            const std::string memberName = std::string(
                fatal::z_data<typename descriptor::metadata::name>(),
                fatal::size<typename descriptor::metadata::name>::value);

            path.push_back(memberName);
            f(path,
              apache::thrift::type_class::variant{},
              std::make_optional(oldNode),
              std::optional<Node>());
            path.pop_back();
          });
      fatal::scalar_search<descriptors, fatal::get_type::id>(
          newNode.getType(), [&](auto indexed) {
            using descriptor = decltype(fatal::tag_type(indexed));

            const std::string memberName = std::string(
                fatal::z_data<typename descriptor::metadata::name>(),
                fatal::size<typename descriptor::metadata::name>::value);

            path.push_back(memberName);
            f(path,
              apache::thrift::type_class::variant{},
              std::optional<Node>(),
              std::make_optional(newNode));
            path.pop_back();
          });

    } else {
      fatal::scalar_search<descriptors, fatal::get_type::id>(
          oldNode.getType(), [&](auto indexed) {
            using descriptor = decltype(fatal::tag_type(indexed));

            const std::string memberName(
                fatal::z_data<typename descriptor::metadata::name>(),
                fatal::size<typename descriptor::metadata::name>::value);

            path.push_back(memberName);
            hasDifferences =
                ThriftDeltaVisitor<typename descriptor::metadata::type_class>::
                    visit(
                        path,
                        typename descriptor::getter()(oldNode),
                        typename descriptor::getter()(newNode),
                        std::forward<Func>(f));
            path.pop_back();
          });
    }

    if (hasDifferences) {
      using TC = apache::thrift::type_class::variant;
      f(path, TC{}, std::make_optional(oldNode), std::make_optional(newNode));
    }

    return hasDifferences;
  }
};

/**
 * Structure
 */
template <>
struct ThriftDeltaVisitor<apache::thrift::type_class::structure> {
  template <typename Node, typename Func>
  static bool visit(const Node& oldNode, const Node& newNode, Func&& f) {
    std::vector<std::string> path;
    return visit(path, oldNode, newNode, std::forward<Func>(f));
  }

  template <typename Node, typename Func>
  static bool visit(
      std::vector<std::string>& path,
      const Node& oldNode,
      const Node& newNode,
      Func&& f) {
    bool hasDifferences{false};

    fatal::foreach<typename apache::thrift::reflect_struct<
        folly::remove_cvref_t<Node>>::members>([&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));

      // Look for the expected member name
      const std::string memberName(
          fatal::z_data<typename member::name>(),
          fatal::size<typename member::name>::value);

      path.push_back(memberName);

      // Check for optionality
      if (member::optional::value == apache::thrift::optionality::optional) {
        if (!member::is_set(oldNode) && member::is_set(newNode)) {
          hasDifferences = true;
          f(path,
            typename member::type_class{},
            std::optional<typename member::type>(),
            std::make_optional(typename member::getter{}(newNode)));
        } else if (member::is_set(oldNode) && !member::is_set(newNode)) {
          hasDifferences = true;
          f(path,
            typename member::type_class{},
            std::make_optional(typename member::getter{}(newNode)),
            std::optional<typename member::type>());
        }
      }

      // Recurse further
      auto different = ThriftDeltaVisitor<typename member::type_class>::visit(
          path,
          typename member::getter{}(oldNode),
          typename member::getter{}(newNode),
          std::forward<Func>(f));
      hasDifferences = hasDifferences || different;
      path.pop_back();
    });

    if (hasDifferences) {
      using TC = apache::thrift::type_class::structure;
      f(path, TC{}, std::make_optional(oldNode), std::make_optional(newNode));
    }

    return hasDifferences;
  }
};

/**
 * Primitives - fallback specialization
 * - string / binary
 * - floating_point
 * - integral
 */
template <typename TC>
struct ThriftDeltaVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <typename Node, typename Func>
  static bool visit(
      std::vector<std::string>& path,
      const Node& oldNode,
      const Node& newNode,
      Func&& f) {
    if (oldNode != newNode) {
      f(path, TC{}, std::make_optional(oldNode), std::make_optional(newNode));
      return true;
    }

    return false;
  }
};

using RootThriftDeltaVisitor =
    ThriftDeltaVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::fsdb
