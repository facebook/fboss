// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fatal/type/enum.h>
#include <fatal/type/slice.h>
#include <fatal/type/trie.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

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
  UNSUPPORTED_WILDCARD_PATH,
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
  using ext_path_token_citr = std::vector<OperPathElem>::const_iterator;

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

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
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
  using Self =
      NameToPathVisitor<apache::thrift::type_class::set<ValueTypeClass>>;
  using path_token_citr = std::vector<std::string>::const_iterator;
  using ext_path_token_citr = std::vector<OperPathElem>::const_iterator;

  template <typename Path>
  using KeyType = typename folly::remove_cvref_t<Path>::DataT::key_type;

  template <typename Path>
  static std::optional<KeyType<Path>> parse(
      const Path& /*path*/,
      const std::string& token) {
    using KeyT = KeyType<Path>;

    auto key = folly::tryTo<KeyT>(token);
    if (key.hasValue()) {
      return key.value();
    }

    if constexpr (std::is_same_v<
                      ValueTypeClass,
                      apache::thrift::type_class::enumeration>) {
      // special handling for enum sets
      KeyT value;
      if (fatal::enum_traits<KeyT>::try_parse(value, token)) {
        return value;
      }
    }

    return std::nullopt;
  }

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
    const auto& token = *curr++;
    auto key = Self::parse(std::forward<Path>(path), token);
    if (!key) {
      return NameToPathResult::INVALID_PATH;
    }

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::TC>::
        visit(path[key.value()], begin, curr, end, std::forward<Func>(f));
  }

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    KeyType<Path> key{};
    auto elem = *curr++;
    if (auto raw = elem.raw_ref()) {
      auto parsed = Self::parse(std::forward<Path>(path), *raw);
      if (!parsed) {
        return NameToPathResult::INVALID_PATH;
      }
      key = parsed.value();
    } else {
      // in this case, we are using a wildcard match. We create a
      // dummy child path w/ the default constructed key.
    }

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::TC>::
        visitExtended(path[key], curr, end, std::forward<Func>(f));
  }
};

/**
 * List
 */
template <typename ValueTypeClass>
struct NameToPathVisitor<apache::thrift::type_class::list<ValueTypeClass>> {
  using Self =
      NameToPathVisitor<apache::thrift::type_class::list<ValueTypeClass>>;
  using path_token_citr = std::vector<std::string>::const_iterator;
  using ext_path_token_citr = std::vector<OperPathElem>::const_iterator;

  using KeyType = int32_t;

  static std::optional<KeyType> parse(const std::string& token) {
    auto key = folly::tryTo<KeyType>(token);
    if (key.hasValue()) {
      return key.value();
    }

    return std::nullopt;
  }

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

    const auto& token = *curr++;
    auto key = Self::parse(token);
    if (!key) {
      return NameToPathResult::INVALID_ARRAY_INDEX;
    }

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::TC>::
        visit(path[key.value()], begin, curr, end, std::forward<Func>(f));
  }

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    KeyType key{};
    auto elem = *curr++;
    if (auto raw = elem.raw_ref()) {
      auto parsed = Self::parse(*raw);
      if (!parsed) {
        return NameToPathResult::INVALID_ARRAY_INDEX;
      }
      key = parsed.value();
    } else {
      // in this case, we are using a wildcard match. We create a
      // dummy child path w/ the default constructed key.
    }

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::TC>::
        visitExtended(path[key], curr, end, std::forward<Func>(f));
  }
};

/**
 * Map
 */
template <typename KeyTypeClass, typename MappedTypeClass>
struct NameToPathVisitor<
    apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>> {
  using Self = NameToPathVisitor<
      apache::thrift::type_class::map<KeyTypeClass, MappedTypeClass>>;
  using path_token_citr = std::vector<std::string>::const_iterator;
  using ext_path_token_citr = std::vector<OperPathElem>::const_iterator;

  template <typename Path>
  using KeyType = typename folly::remove_cvref_t<Path>::DataT::key_type;

  template <typename Path>
  static std::optional<KeyType<Path>> parse(
      const Path& /*path*/,
      const std::string& token) {
    using KeyT = KeyType<Path>;

    auto key = folly::tryTo<KeyT>(token);
    if (key.hasValue()) {
      return key.value();
    }

    if constexpr (std::is_same_v<
                      KeyTypeClass,
                      apache::thrift::type_class::enumeration>) {
      // special handling for enum sets
      KeyT value;
      if (fatal::enum_traits<KeyT>::try_parse(value, token)) {
        return value;
      }
    }

    return std::nullopt;
  }

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
    const auto& token = *curr++;
    auto key = Self::parse(std::forward<Path>(path), token);
    if (!key) {
      return NameToPathResult::INVALID_MAP_KEY;
    }

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::TC>::
        visit(path[key.value()], begin, curr, end, std::forward<Func>(f));
  }

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    KeyType<Path> key{};
    auto elem = *curr++;
    if (auto raw = elem.raw_ref()) {
      auto parsed = Self::parse(std::forward<Path>(path), *raw);
      if (!parsed) {
        return NameToPathResult::INVALID_MAP_KEY;
      }
      key = parsed.value();
    } else {
      // in this case, we are using a wildcard match. We create a
      // dummy child path and recurse
    }

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::TC>::
        visitExtended(path[key], curr, end, std::forward<Func>(f));
  }
};

/**
 * Variant
 */
template <>
struct NameToPathVisitor<apache::thrift::type_class::variant> {
  using path_token_citr = std::vector<std::string>::const_iterator;
  using ext_path_token_citr = std::vector<OperPathElem>::const_iterator;

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

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    auto elem = *curr++;
    if (auto raw = elem.raw_ref()) {
      auto token = *raw;
      auto result = NameToPathResult::INVALID_VARIANT_MEMBER;
      fatal::trie_find<
          typename folly::remove_cvref_t<Path>::Children,
          fatal::get_first>(token.begin(), token.end(), [&](auto tag) {
        using PathT =
            typename folly::remove_cvref_t<decltype(tag)>::type::second_type;
        auto newPath = path.tokens();
        newPath.push_back(token);
        auto childPath = PathT(std::move(newPath));
        result = NameToPathVisitor<typename PathT::TC>::visitExtended(
            childPath, curr, end, std::forward<Func>(f));
      });
      return result;
    } else {
      // Regex/any matches are not allowed against structs
      return NameToPathResult::UNSUPPORTED_WILDCARD_PATH;
    }

    return NameToPathResult::INVALID_PATH;
  }
};

/**
 * Structure
 */
template <>
struct NameToPathVisitor<apache::thrift::type_class::structure> {
  using path_token_citr = std::vector<std::string>::const_iterator;
  using ext_path_token_citr = std::vector<OperPathElem>::const_iterator;

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

    auto visitChild = [&](auto tag) {
      using PathT =
          typename folly::remove_cvref_t<decltype(tag)>::type::second_type;
      auto childPath = PathT(std::vector<std::string>(begin, curr));
      result = NameToPathVisitor<typename PathT::TC>::visit(
          childPath, begin, curr, end, std::forward<Func>(f));
    };

    auto idTry = folly::tryTo<apache::thrift::field_id_t>(token);
    if (!idTry.hasError()) {
      fatal::scalar_search<
          typename folly::remove_cvref_t<Path>::ChildrenById,
          fatal::get_first>(idTry.value(), std::move(visitChild));
    } else {
      fatal::trie_find<
          typename folly::remove_cvref_t<Path>::Children,
          fatal::get_first>(token.begin(), token.end(), std::move(visitChild));
    }
    return result;
  }

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(path);
      return NameToPathResult::OK;
    }

    auto elem = *curr++;
    if (auto raw = elem.raw_ref()) {
      auto token = *raw;
      auto result = NameToPathResult::INVALID_STRUCT_MEMBER;
      fatal::trie_find<
          typename folly::remove_cvref_t<Path>::Children,
          fatal::get_first>(token.begin(), token.end(), [&](auto tag) {
        using PathT =
            typename folly::remove_cvref_t<decltype(tag)>::type::second_type;
        auto newPath = path.tokens();
        newPath.push_back(token);
        auto childPath = PathT(std::move(newPath));
        result = NameToPathVisitor<typename PathT::TC>::visitExtended(
            childPath, curr, end, std::forward<Func>(f));
      });
      return result;
    } else {
      // Regex/any matches are not allowed against structs
      return NameToPathResult::UNSUPPORTED_WILDCARD_PATH;
    }

    return NameToPathResult::INVALID_PATH;
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
  using ext_path_token_citr = std::vector<OperPathElem>::const_iterator;

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

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
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
