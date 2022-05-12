// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fatal/type/enum.h>
#include <fatal/type/slice.h>
#include <fatal/type/trie.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp2/TypeClass.h>

namespace facebook::fboss::fsdb {

/*
 * This visitor is used to start with a string path and then to ensure
 * that the path is consistent with the underlying data model. If it
 * is, we resolve the type to a thrift_path::Path object. If not,
 * we return an error result when visiting.
 *
 * This is used to move from a runtime path (vector<string>) to a
 * typed path object that can be used to efficiently access the store.
 *
 * Example usage in tests dire
 */

enum class NameToPathResult {
  OK,
  INVALID_PATH,
  INVALID_STRUCT_MEMBER,
  INVALID_VARIANT_MEMBER,
  INVALID_ARRAY_INDEX,
  INVALID_MAP_KEY,
  VISITOR_EXCEPTION,
};

/*
 * Base template, deliberately left undefined since
 * this should never be instantiated.
 */
template <typename>
struct NameToPathVisitor;

/**
 * Enumeration
 */
template <>
struct NameToPathVisitor<apache::thrift::type_class::enumeration> {
  using path_token_citr = std::vector<std::string>::const_iterator;

  template <typename Path, typename Func>
  static NameToPathResult visit(
      Path&& path,
      path_token_citr /*begin*/,
      path_token_citr curr,
      path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    return NameToPathResult::INVALID_PATH;
  }
};

/**
 * Set
 */
template <typename ValueTypeClass>
struct NameToPathVisitor<apache::thrift::type_class::set<ValueTypeClass>> {
  using path_token_citr = std::vector<std::string>::const_iterator;

  template <typename Path, typename Func>
  static NameToPathResult visit(
      Path&& path,
      path_token_citr begin,
      path_token_citr curr,
      path_token_citr end,
      Func&& f) {
    if (curr == end) {
      try {
        f(path);
        return NameToPathResult::OK;
      } catch (const std::exception&) {
        return NameToPathResult::VISITOR_EXCEPTION;
      }
    }

    using KeyT = typename folly::remove_cvref_t<Path>::DataT::key_type;
    // Get key
    auto token = *curr++;
    auto key = folly::tryTo<KeyT>(token);
    if (key.hasValue()) {
      return NameToPathVisitor<
          typename folly::remove_cvref_t<Path>::Child::TC>::
          visit(path[key.value()], begin, curr, end, std::forward<Func>(f));
    }

    if constexpr (std::is_same_v<
                      ValueTypeClass,
                      apache::thrift::type_class::enumeration>) {
      // special handling for enum sets
      KeyT value;
      if (fatal::enum_traits<KeyT>::try_parse(value, token)) {
        return NameToPathVisitor<
            typename folly::remove_cvref_t<Path>::Child::TC>::
            visit(path[value], begin, curr, end, std::forward<Func>(f));
      }
    }
    return NameToPathResult::INVALID_PATH;
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct NameToPathVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  using path_token_citr = std::vector<std::string>::const_iterator;

  template <typename Path, typename Func>
  static NameToPathResult visit(
      Path&& path,
      path_token_citr begin,
      path_token_citr curr,
      path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    auto token = *curr++;
    auto key = folly::tryTo<int32_t>(token);
    if (key.hasError()) {
      return NameToPathResult::INVALID_ARRAY_INDEX;
    }

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::TC>::
        visit(path[key.value()], begin, curr, end, std::forward<Func>(f));
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct NameToPathVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using path_token_citr = std::vector<std::string>::const_iterator;

  template <typename Path, typename Func>
  static NameToPathResult visit(
      Path&& path,
      path_token_citr begin,
      path_token_citr curr,
      path_token_citr end,
      Func&& f) {
    if (curr == end) {
      try {
        f(path);
        return NameToPathResult::OK;
      } catch (const std::exception&) {
        return NameToPathResult::VISITOR_EXCEPTION;
      }
    }

    using KeyT = typename folly::remove_cvref_t<Path>::DataT::key_type;
    // Get key
    auto token = *curr++;
    auto key = folly::tryTo<KeyT>(token);
    if (key.hasValue()) {
      return NameToPathVisitor<
          typename folly::remove_cvref_t<Path>::Child::TC>::
          visit(path[key.value()], begin, curr, end, std::forward<Func>(f));
    }

    if constexpr (std::is_same_v<
                      KeyTypeClass,
                      apache::thrift::type_class::enumeration>) {
      // special handling for enum keyed maps
      KeyT value;
      if (fatal::enum_traits<KeyT>::try_parse(value, token)) {
        return NameToPathVisitor<
            typename folly::remove_cvref_t<Path>::Child::TC>::
            visit(path[value], begin, curr, end, std::forward<Func>(f));
      }
    }
    return NameToPathResult::INVALID_MAP_KEY;
  }
};

/**
 * Variant
 */
template <>
struct NameToPathVisitor<apache::thrift::type_class::variant> {
  using path_token_citr = std::vector<std::string>::const_iterator;

  template <typename Path, typename Func>
  static NameToPathResult visit(
      Path&& path,
      path_token_citr begin,
      path_token_citr curr,
      path_token_citr end,
      Func&& f) {
    if (curr == end) {
      try {
        f(path);
        return NameToPathResult::OK;
      } catch (const std::exception&) {
        return NameToPathResult::VISITOR_EXCEPTION;
      }
    }

    // Get key
    auto token = *curr++;
    auto result = NameToPathResult::INVALID_VARIANT_MEMBER;
    fatal::trie_find<
        typename folly::remove_cvref_t<Path>::Children,
        fatal::get_first>(token.begin(), token.end(), [&](auto tag) {
      using PathT =
          typename folly::remove_cvref_t<decltype(tag)>::type::second_type;
      auto childPath = PathT(std::vector<std::string>(begin, curr));
      result = NameToPathVisitor<typename PathT::TC>::visit(
          childPath, begin, curr, end, std::forward<Func>(f));
    });
    return result;
  }
};

/**
 * Structure
 */
template <>
struct NameToPathVisitor<apache::thrift::type_class::structure> {
  using path_token_citr = std::vector<std::string>::const_iterator;

  template <typename Path, typename Func>
  static NameToPathResult visit(
      Path&& path,
      path_token_citr begin,
      path_token_citr curr,
      path_token_citr end,
      Func&& f) {
    if (curr == end) {
      try {
        f(path);
        return NameToPathResult::OK;
      } catch (const std::exception&) {
        return NameToPathResult::VISITOR_EXCEPTION;
      }
    }

    // Get key
    auto token = *curr++;
    auto result = NameToPathResult::INVALID_STRUCT_MEMBER;
    fatal::trie_find<
        typename folly::remove_cvref_t<Path>::Children,
        fatal::get_first>(token.begin(), token.end(), [&](auto tag) {
      using PathT =
          typename folly::remove_cvref_t<decltype(tag)>::type::second_type;
      auto childPath = PathT(std::vector<std::string>(begin, curr));
      result = NameToPathVisitor<typename PathT::TC>::visit(
          childPath, begin, curr, end, std::forward<Func>(f));
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
template <typename TC>
struct NameToPathVisitor {
  static_assert(
      !std::is_same<apache::thrift::type_class::unknown, TC>::value,
      "No static reflection support for the given type. "
      "Forgot to specify reflection option or include fatal header file? "
      "Refer to thrift/lib/cpp2/reflection/reflection.h");

  using path_token_citr = std::vector<std::string>::const_iterator;

  template <typename Path, typename Func>
  static NameToPathResult visit(
      Path&& path,
      path_token_citr /*begin*/,
      path_token_citr curr,
      path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    return NameToPathResult::INVALID_PATH;
  }
};

using RootNameToPathVisitor =
    NameToPathVisitor<apache::thrift::type_class::structure>;

} // namespace facebook::fboss::fsdb
