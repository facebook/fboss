// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fatal/type/search.h>
#include <fatal/type/trie.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_visitors/gen-cpp2/results_types.h"

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

/*
 * Base template, deliberately left undefined since
 * this should never be instantiated.
 */
template <typename>
struct NameToPathVisitor;

/**
 * Enumeration
 */
template <typename T>
struct NameToPathVisitor<apache::thrift::type::enum_t<T>> {
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
      f(std::forward<Path>(path));
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
      f(std::forward<Path>(path));
      return NameToPathResult::OK;
    }

    return NameToPathResult::INVALID_PATH;
  }
};

/**
 * Set
 */
template <typename ValueTag>
struct NameToPathVisitor<apache::thrift::type::set<ValueTag>> {
  using Self = NameToPathVisitor<apache::thrift::type::set<ValueTag>>;
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

    if constexpr (apache::thrift::type::
                      is_a_v<ValueTag, apache::thrift::type::enum_c>) {
      // special handling for enum sets
      KeyT value;
      if (apache::thrift::util::tryParseEnum(token, &value)) {
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
        f(std::forward<Path>(path));
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

    using Tag = typename folly::remove_cvref_t<Path>::Child::Tag;
    return NameToPathVisitor<Tag>::visit(
        path[key.value()], begin, curr, end, std::forward<Func>(f));
  }

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(std::forward<Path>(path));
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

    return NameToPathVisitor<typename folly::remove_cvref_t<Path>::Child::Tag>::
        visitExtended(path[key], curr, end, std::forward<Func>(f));
  }
};

/**
 * List
 */
template <typename ValueTag>
struct NameToPathVisitor<apache::thrift::type::list<ValueTag>> {
  using Self = NameToPathVisitor<apache::thrift::type::list<ValueTag>>;
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
      f(std::forward<Path>(path));
      return NameToPathResult::OK;
    }

    const auto& token = *curr++;
    auto key = Self::parse(token);
    if (!key) {
      return NameToPathResult::INVALID_ARRAY_INDEX;
    }

    using Tag = typename folly::remove_cvref_t<Path>::Child::Tag;
    return NameToPathVisitor<Tag>::visit(
        path[key.value()], begin, curr, end, std::forward<Func>(f));
  }

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(std::forward<Path>(path));
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

    using Tag = typename folly::remove_cvref_t<Path>::Child::Tag;
    return NameToPathVisitor<Tag>::visitExtended(
        path[key], curr, end, std::forward<Func>(f));
  }
};

/**
 * Map
 */
template <typename KeyTag, typename MappedTag>
struct NameToPathVisitor<apache::thrift::type::map<KeyTag, MappedTag>> {
  using Self = NameToPathVisitor<apache::thrift::type::map<KeyTag, MappedTag>>;
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

    if constexpr (apache::thrift::type::
                      is_a_v<KeyTag, apache::thrift::type::enum_c>) {
      // special handling for enum sets
      KeyT value;
      if (apache::thrift::util::tryParseEnum(token, &value)) {
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
        f(std::forward<Path>(path));
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

    using Tag = typename folly::remove_cvref_t<Path>::Child::Tag;
    return NameToPathVisitor<Tag>::visit(
        path[key.value()], begin, curr, end, std::forward<Func>(f));
  }

  template <typename Path, typename Func>
  static NameToPathResult visitExtended(
      Path&& path,
      ext_path_token_citr curr,
      ext_path_token_citr end,
      Func&& f) {
    if (curr == end) {
      f(std::forward<Path>(path));
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

    using Tag = typename folly::remove_cvref_t<Path>::Child::Tag;
    return NameToPathVisitor<Tag>::visitExtended(
        path[key], curr, end, std::forward<Func>(f));
  }
};

/**
 * Variant
 */
template <typename T>
struct NameToPathVisitor<apache::thrift::type::union_t<T>> {
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
        f(std::forward<Path>(path));
        return NameToPathResult::OK;
      } catch (const std::exception&) {
        return NameToPathResult::VISITOR_EXCEPTION;
      }
    }

    // Get key
    const auto& token = *curr++;
    auto result = NameToPathResult::INVALID_VARIANT_MEMBER;

    auto visitChild = [&]<class Tag>(Tag) {
      using PathT = typename folly::remove_cvref_t<Tag>::type::second_type;
      using ChildKeyT = typename folly::remove_cvref_t<Tag>::type::first_type;
      using ChildTag = typename PathT::Tag;
      result = NameToPathVisitor<ChildTag>::visit(
          path(ChildKeyT()), begin, curr, end, std::forward<Func>(f));
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
      f(std::forward<Path>(path));
      return NameToPathResult::OK;
    }

    auto elem = *curr++;
    if (auto raw = elem.raw_ref()) {
      const auto& token = *raw;
      auto result = NameToPathResult::INVALID_VARIANT_MEMBER;

      auto visitChild = [&]<class Tag>(Tag) {
        using PathT = typename folly::remove_cvref_t<Tag>::type::second_type;
        using ChildKeyT = typename folly::remove_cvref_t<Tag>::type::first_type;
        using ChildTag = typename PathT::Tag;
        result = NameToPathVisitor<ChildTag>::visitExtended(
            path(ChildKeyT()), curr, end, std::forward<Func>(f));
      };

      auto idTry = folly::tryTo<apache::thrift::field_id_t>(token);
      if (!idTry.hasError()) {
        fatal::scalar_search<
            typename folly::remove_cvref_t<Path>::ChildrenById,
            fatal::get_first>(idTry.value(), std::move(visitChild));
      } else {
        fatal::trie_find<
            typename folly::remove_cvref_t<Path>::Children,
            fatal::get_first>(
            token.begin(), token.end(), std::move(visitChild));
      }

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
template <typename T>
struct NameToPathVisitor<apache::thrift::type::struct_t<T>> {
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
        f(std::forward<Path>(path));
        return NameToPathResult::OK;
      } catch (const std::exception&) {
        return NameToPathResult::VISITOR_EXCEPTION;
      }
    }

    // Get key
    const auto& token = *curr++;
    auto result = NameToPathResult::INVALID_STRUCT_MEMBER;

    auto visitChild = [&]<class Tag>(Tag) {
      using PathT = typename folly::remove_cvref_t<Tag>::type::second_type;
      using ChildKeyT = typename folly::remove_cvref_t<Tag>::type::first_type;
      using ChildTag = typename PathT::Tag;
      result = NameToPathVisitor<ChildTag>::visit(
          path(ChildKeyT()), begin, curr, end, std::forward<Func>(f));
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
      f(std::forward<Path>(path));
      return NameToPathResult::OK;
    }

    auto elem = *curr++;
    if (auto raw = elem.raw_ref()) {
      const auto& token = *raw;
      auto result = NameToPathResult::INVALID_STRUCT_MEMBER;

      auto visitChild = [&]<class Tag>(Tag) {
        using PathT = typename folly::remove_cvref_t<Tag>::type::second_type;
        using ChildKeyT = typename folly::remove_cvref_t<Tag>::type::first_type;
        result = NameToPathVisitor<typename PathT::Tag>::visitExtended(
            path(ChildKeyT()), curr, end, std::forward<Func>(f));
      };

      auto idTry = folly::tryTo<apache::thrift::field_id_t>(token);
      if (!idTry.hasError()) {
        fatal::scalar_search<
            typename folly::remove_cvref_t<Path>::ChildrenById,
            fatal::get_first>(idTry.value(), std::move(visitChild));
      } else {
        fatal::trie_find<
            typename folly::remove_cvref_t<Path>::Children,
            fatal::get_first>(
            token.begin(), token.end(), std::move(visitChild));
      }
      return result;
    } else {
      // Regex/any matches are not allowed against structs
      return NameToPathResult::UNSUPPORTED_WILDCARD_PATH;
    }

    return NameToPathResult::INVALID_PATH;
  }
};

/**
 * Cpp.Type
 */
template <typename T, typename Tag>
struct NameToPathVisitor<apache::thrift::type::cpp_type<T, Tag>>
    : public NameToPathVisitor<Tag> {};

/**
 * Primitives - fallback specialization
 * - string / binary
 * - floating_point
 * - integral
 */
template <typename Tag>
struct NameToPathVisitor {
  static_assert(
      apache::thrift::type::is_a_v<Tag, apache::thrift::type::primitive_c>,
      "expected primitive type");

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
      f(std::forward<Path>(path));
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
      f(std::forward<Path>(path));
      return NameToPathResult::OK;
    }

    return NameToPathResult::INVALID_PATH;
  }
};

template <typename Node>
using RootNameToPathVisitor =
    NameToPathVisitor<apache::thrift::type::struct_t<Node>>;

} // namespace facebook::fboss::fsdb
