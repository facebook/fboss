// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <type_traits>

#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::fsdb {

/*
 * This visitor takes a path object and a thrift type and is able to
 * traverse the object and then run the visitor function against the
 * final type.
 *
 * TODO: examples
 */

struct CreateNodeIfMissing {};

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

template <typename T>
inline constexpr bool isConst =
    std::is_const_v<typename std::remove_reference_t<T>>;

template <typename T>
inline constexpr bool shouldCreateNodeIfMissing =
    std::is_same_v<T, CreateNodeIfMissing>;

template <typename TC, typename Options>
struct ThriftPathVisitor;

/**
 * Enumeration
 */
template <typename Options>
struct ThriftPathVisitor<apache::thrift::type_class::enumeration, Options> {
  template <
      typename Node,
      typename PathIter,
      typename Func,
      typename = std::enable_if_t<
          std::is_same_v<
              typename std::iterator_traits<PathIter>::value_type,
              std::string>,
          void>>
  static ThriftTraverseResult
  visit(Node&& node, PathIter begin, PathIter end, Func&& f) {
    using TC = apache::thrift::type_class::enumeration;

    // Check for terminal condition
    if (begin == end) {
      try {
        f(node, TC{});
        return ThriftTraverseResult::OK;
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }

    return ThriftTraverseResult::NON_EXISTENT_NODE;
  }
};

/**
 * Set
 */
template <typename ValueTypeClass, typename Options>
struct ThriftPathVisitor<
    apache::thrift::type_class::set<ValueTypeClass>,
    Options> {
  template <
      typename Node,
      typename PathIter,
      typename Func,
      typename = std::enable_if_t<
          std::is_same_v<
              typename std::iterator_traits<PathIter>::value_type,
              std::string>,
          void>>
  static ThriftTraverseResult
  visit(Node&& node, PathIter begin, PathIter end, Func&& f) {
    using TC = apache::thrift::type_class::set<ValueTypeClass>;
    using ValueT = typename folly::remove_cvref_t<Node>::value_type;

    static_assert(
        !(shouldCreateNodeIfMissing<Options> && isConst<Node>),
        "Cannot use CreateNodeIfMissing option against a const node");
    constexpr auto createNodeIfMissing =
        shouldCreateNodeIfMissing<Options> && !isConst<Node>;

    if (begin == end) {
      try {
        f(node, TC{});
        return ThriftTraverseResult::OK;
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }

    auto tok = *begin++;
    ValueT value;

    // need to handle enumeration, integral, string types
    if constexpr (std::is_same_v<
                      ValueTypeClass,
                      apache::thrift::type_class::enumeration>) {
      if (fatal::enum_traits<ValueT>::try_parse(value, tok)) {
        auto it = node.find(value);
        if (it == node.end()) {
          if constexpr (!createNodeIfMissing) {
            return ThriftTraverseResult::NON_EXISTENT_NODE;
          } else {
            node.insert(value);
          }
        }

        // Recurse further
        return ThriftPathVisitor<ValueTypeClass, Options>::visit(
            value, begin, end, std::forward<Func>(f));
      }
    }

    auto valueTry = folly::tryTo<ValueT>(tok);
    if (!valueTry.hasError()) {
      value = valueTry.value();
      auto it = node.find(value);
      if (it == node.end()) {
        if constexpr (!createNodeIfMissing) {
          return ThriftTraverseResult::NON_EXISTENT_NODE;
        } else {
          node.insert(value);
        }
      }
      // Recurse further
      return ThriftPathVisitor<ValueTypeClass, Options>::visit(
          value, begin, end, std::forward<Func>(f));
    }

    // if we get here, we must have a malformed key
    return ThriftTraverseResult::INVALID_SET_MEMBER;
  }
};

/**
 * List
 */
template <typename ValueTypeClass, typename Options>
struct ThriftPathVisitor<
    apache::thrift::type_class::list<ValueTypeClass>,
    Options> {
  template <
      typename Node,
      typename PathIter,
      typename Func,
      typename = std::enable_if_t<
          std::is_same_v<
              typename std::iterator_traits<PathIter>::value_type,
              std::string>,
          void>>
  static ThriftTraverseResult
  visit(Node&& node, PathIter begin, PathIter end, Func&& f) {
    using TC = apache::thrift::type_class::list<ValueTypeClass>;

    static_assert(
        !(shouldCreateNodeIfMissing<Options> && isConst<Node>),
        "Cannot use CreateNodeIfMissing option against a const node");
    constexpr auto createNodeIfMissing =
        shouldCreateNodeIfMissing<Options> && !isConst<Node>;

    if (begin == end) {
      try {
        f(node, TC{});
        return ThriftTraverseResult::OK;
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }

    // Parse and pop token. Also check for index bound
    auto index = folly::tryTo<size_t>(*begin++);
    if (index.hasError()) {
      return ThriftTraverseResult::INVALID_ARRAY_INDEX;
    }
    if (index.value() >= node.size()) {
      if constexpr (!createNodeIfMissing) {
        return ThriftTraverseResult::NON_EXISTENT_NODE;
      } else {
        node.resize(index.value());
      }
    }

    // Recurse at a given index
    return ThriftPathVisitor<ValueTypeClass, Options>::visit(
        node.at(index.value()), begin, end, std::forward<Func>(f));
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass, typename Options>
struct ThriftPathVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>,
    Options> {
  template <
      typename Node,
      typename PathIter,
      typename Func,
      typename = std::enable_if_t<
          std::is_same_v<
              typename std::iterator_traits<PathIter>::value_type,
              std::string>,
          void>>
  static ThriftTraverseResult
  visit(Node&& node, PathIter begin, PathIter end, Func&& f) {
    using TC = apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>;

    static_assert(
        !(shouldCreateNodeIfMissing<Options> && isConst<Node>),
        "Cannot use CreateNodeIfMissing option against a const node");
    constexpr auto createNodeIfMissing =
        shouldCreateNodeIfMissing<Options> && !isConst<Node>;

    if (begin == end) {
      try {
        f(node, TC{});
        return ThriftTraverseResult::OK;
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }

    using KeyT = typename folly::remove_cvref_t<Node>::key_type;
    // Get key
    auto token = *begin++;
    auto key = folly::tryTo<KeyT>(token);
    if (!key.hasValue()) {
      if constexpr (std::is_same_v<
                        KeyTypeClass,
                        apache::thrift::type_class::enumeration>) {
        // special handling for enum keyed maps
        KeyT value;
        if (fatal::enum_traits<KeyT>::try_parse(value, token)) {
          key = value;
        } else {
          return ThriftTraverseResult::INVALID_MAP_KEY;
        }
      } else {
        return ThriftTraverseResult::INVALID_MAP_KEY;
      }
    }

    if (node.find(*key) == node.end()) {
      if constexpr (!createNodeIfMissing) {
        return ThriftTraverseResult::NON_EXISTENT_NODE;
      } else {
        node[*key];
      }
    }

    // Recurse further
    return ThriftPathVisitor<MappedTypeClass, Options>::visit(
        node.at(*key), begin, end, std::forward<Func>(f));
  }
};

/**
 * Variant
 */
template <typename Options>
struct ThriftPathVisitor<apache::thrift::type_class::variant, Options> {
  template <
      typename Node,
      typename PathIter,
      typename Func,
      typename = std::enable_if_t<
          std::is_same_v<
              typename std::iterator_traits<PathIter>::value_type,
              std::string>,
          void>>
  static ThriftTraverseResult
  visit(Node&& node, PathIter begin, PathIter end, Func&& f) {
    static_assert(
        !(shouldCreateNodeIfMissing<Options> && isConst<Node>),
        "Cannot use CreateNodeIfMissing option against a const node");
    constexpr auto createNodeIfMissing =
        shouldCreateNodeIfMissing<Options> && !isConst<Node>;

    if (begin == end) {
      try {
        f(node, apache::thrift::type_class::variant{});
        return ThriftTraverseResult::OK;
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }

    auto result = ThriftTraverseResult::INVALID_VARIANT_MEMBER;

    // Get key
    auto key = *begin++;

    using descriptors = typename apache::thrift::reflect_variant<
        folly::remove_cvref_t<Node>>::traits::descriptors;
    fatal::foreach<descriptors>([&](auto tag) {
      using descriptor = decltype(fatal::tag_type(tag));

      const std::string memberName(
          fatal::z_data<typename descriptor::metadata::name>(),
          fatal::size<typename descriptor::metadata::name>::value);

      if (memberName != key) {
        return;
      }

      if (node.getType() != descriptor::metadata::id::value) {
        if constexpr (!createNodeIfMissing) {
          result = ThriftTraverseResult::INCORRECT_VARIANT_MEMBER;
          return;
        } else {
          // switch union value to point at new path.
          descriptor::set(node);
        }
      }

      result = ThriftPathVisitor<
          typename descriptor::metadata::type_class,
          Options>::
          visit(
              typename descriptor::getter()(node),
              begin,
              end,
              std::forward<Func>(f));
    });
    return result;
  }
};

/**
 * Structure
 */
template <typename Options>
struct ThriftPathVisitor<apache::thrift::type_class::structure, Options> {
  template <
      typename Node,
      typename PathIter,
      typename Func,
      typename = std::enable_if_t<
          std::is_same_v<
              typename std::iterator_traits<PathIter>::value_type,
              std::string>,
          void>>
  static ThriftTraverseResult
  visit(Node&& node, PathIter begin, PathIter end, Func&& f) {
    static_assert(
        !(shouldCreateNodeIfMissing<Options> && isConst<Node>),
        "Cannot use CreateNodeIfMissing option against a const node");
    constexpr auto createNodeIfMissing =
        shouldCreateNodeIfMissing<Options> && !isConst<Node>;

    if (begin == end) {
      try {
        f(node, apache::thrift::type_class::structure{});
        return ThriftTraverseResult::OK;
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }

    // Get key
    auto key = *begin++;

    // Perform linear search over all members for key
    ThriftTraverseResult result = ThriftTraverseResult::INVALID_STRUCT_MEMBER;
    fatal::foreach<typename apache::thrift::reflect_struct<
        folly::remove_cvref_t<Node>>::members>([&](auto indexed) mutable {
      using member = decltype(fatal::tag_type(indexed));

      // Look for the expected member name
      const std::string memberName(
          fatal::z_data<typename member::name>(),
          fatal::size<typename member::name>::value);
      if (memberName != key) {
        return;
      }

      // Check for optionality
      if (member::optional::value == apache::thrift::optionality::optional &&
          !member::is_set(node)) {
        if constexpr (!createNodeIfMissing) {
          result = ThriftTraverseResult::NON_EXISTENT_NODE;
          return;
        } else {
          typename member::field_ref_getter{}(node) = {};
        }
      }

      // Recurse further
      result = ThriftPathVisitor<typename member::type_class, Options>::visit(
          typename member::getter{}(node), begin, end, std::forward<Func>(f));
    });

    return result;
  }
};

/**
 * Primitives - fallback specialization
 * - string / binary
 * - floating_point
 * - integral
 */
template <typename TC, typename Options>
struct ThriftPathVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  template <
      typename Node,
      typename PathIter,
      typename Func,
      typename = std::enable_if_t<
          std::is_same_v<
              typename std::iterator_traits<PathIter>::value_type,
              std::string>,
          void>>
  static ThriftTraverseResult
  visit(Node&& node, PathIter begin, PathIter end, Func&& f) {
    if (begin == end) {
      try {
        f(node, TC{});
        return ThriftTraverseResult::OK;
      } catch (const std::exception&) {
        return ThriftTraverseResult::VISITOR_EXCEPTION;
      }
    }
    return ThriftTraverseResult::NON_EXISTENT_NODE;
  }
};

using RootThriftPathVisitor =
    ThriftPathVisitor<apache::thrift::type_class::structure, void>;

template <typename Options>
using RootThriftPathVisitorWithOptions =
    ThriftPathVisitor<apache::thrift::type_class::structure, Options>;

} // namespace facebook::fboss::fsdb
