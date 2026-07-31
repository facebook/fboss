// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/common/PathHelpers.h"
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/String.h>
#include <folly/Unit.h>
#include <re2/re2.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/detail/TypeClassFromTypeTag.h>
#include <cstdint>
#include <string>
#include <type_traits>

namespace thriftpath {

namespace detail {
template <typename Key, typename... Rest>
struct type_map_find;

template <typename Key, typename V, typename... Rest>
struct type_map_find<Key, std::pair<Key, V>, Rest...> {
  using type = V;
};

template <typename Key, typename P, typename... Rest>
struct type_map_find<Key, P, Rest...> : type_map_find<Key, Rest...> {};

template <typename Tag>
using type_class_t = apache::thrift::type_class::from_type_tag_t<Tag>;

template <typename Tag>
struct unwrap_type_tag {
  using type = Tag;
};
template <typename Adapter, typename Tag>
struct unwrap_type_tag<apache::thrift::type::adapted<Adapter, Tag>>
    : unwrap_type_tag<Tag> {};
template <typename T, typename Tag>
struct unwrap_type_tag<apache::thrift::type::cpp_type<T, Tag>>
    : unwrap_type_tag<Tag> {};

template <typename Tag>
using unwrap_type_tag_t = typename unwrap_type_tag<Tag>::type;

template <typename Tag>
struct is_complex_type_tag : std::false_type {};
template <typename T>
struct is_complex_type_tag<apache::thrift::type::struct_t<T>> : std::true_type {
};
template <typename T>
struct is_complex_type_tag<apache::thrift::type::union_t<T>> : std::true_type {
};
template <typename T>
struct is_complex_type_tag<apache::thrift::type::exception_t<T>>
    : std::true_type {};
template <typename ValueTag>
struct is_complex_type_tag<apache::thrift::type::list<ValueTag>>
    : std::true_type {};
template <typename ValueTag>
struct is_complex_type_tag<apache::thrift::type::set<ValueTag>>
    : std::true_type {};
template <typename KeyTag, typename ValueTag>
struct is_complex_type_tag<apache::thrift::type::map<KeyTag, ValueTag>>
    : std::true_type {};

template <typename Tag>
inline constexpr bool is_complex_type_tag_v =
    is_complex_type_tag<unwrap_type_tag_t<Tag>>::value;

template <typename Tag>
struct is_container_type_tag : std::false_type {};
template <typename ValueTag>
struct is_container_type_tag<apache::thrift::type::list<ValueTag>>
    : std::true_type {};
template <typename ValueTag>
struct is_container_type_tag<apache::thrift::type::set<ValueTag>>
    : std::true_type {};
template <typename KeyTag, typename ValueTag>
struct is_container_type_tag<apache::thrift::type::map<KeyTag, ValueTag>>
    : std::true_type {};

template <typename Tag>
inline constexpr bool is_container_type_tag_v =
    is_container_type_tag<unwrap_type_tag_t<Tag>>::value;
} // namespace detail

template <typename... Pairs>
struct TypeMap {
  template <typename Key>
  using type_of = typename detail::type_map_find<Key, Pairs...>::type;
};

#define STRUCT_CHILD_GETTERS(child, childId)                              \
  TypeFor<apache::thrift::ident::child> child() const& {                  \
    return this->template get<apache::thrift::ident::child>();            \
  }                                                                       \
  TypeFor<apache::thrift::ident::child> child() && {                      \
    return std::move(*this).template get<apache::thrift::ident::child>(); \
  }
#define CONTAINER_CHILD_GETTERS(key_type)                             \
  Child operator[](key_type token) const& {                           \
    const std::string strToken = folly::to<std::string>(token);       \
    facebook::fboss::fsdb::OperPathElem elem;                         \
    elem.set_raw(strToken);                                           \
    return Child(                                                     \
        copyAndExtendVec(this->tokens_, strToken),                    \
        copyAndExtendVec(this->idTokens_, strToken),                  \
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),     \
        this->hasWildcards_);                                         \
  }                                                                   \
  Child operator[](key_type token) && {                               \
    const std::string strToken = folly::to<std::string>(token);       \
    this->tokens_.push_back(strToken);                                \
    this->idTokens_.push_back(strToken);                              \
    facebook::fboss::fsdb::OperPathElem elem;                         \
    elem.set_raw(strToken);                                           \
    this->extendedTokens_.push_back(std::move(elem));                 \
    return Child(                                                     \
        std::move(this->tokens_),                                     \
        std::move(this->idTokens_),                                   \
        std::move(this->extendedTokens_),                             \
        this->hasWildcards_);                                         \
  }                                                                   \
  Child operator[](facebook::fboss::fsdb::OperPathElem elem) const& { \
    return Child(                                                     \
        copyAndExtendVec(this->tokens_, pathElemToString(elem)),      \
        copyAndExtendVec(this->idTokens_, pathElemToString(elem)),    \
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),     \
        true /* hasWildcards */);                                     \
  }                                                                   \
  Child operator[](facebook::fboss::fsdb::OperPathElem elem) && {     \
    this->tokens_.push_back(pathElemToString(elem));                  \
    this->idTokens_.push_back(pathElemToString(elem));                \
    this->extendedTokens_.push_back(std::move(elem));                 \
    return Child(                                                     \
        std::move(this->tokens_),                                     \
        std::move(this->idTokens_),                                   \
        std::move(this->extendedTokens_),                             \
        true /* hasWildcards */);                                     \
  }

class BasePath {
 public:
  BasePath(
      std::vector<std::string> tokens,
      std::vector<std::string> idTokens,
      std::vector<facebook::fboss::fsdb::OperPathElem> extendedTokens,
      bool hasWildcards)
      : tokens_(std::move(tokens)),
        idTokens_(std::move(idTokens)),
        extendedTokens_(std::move(extendedTokens)),
        hasWildcards_(hasWildcards) {}

  auto begin() const {
    return tokens_.cbegin();
  }

  auto end() const {
    return tokens_.cend();
  }

  const std::vector<std::string>& tokens() const {
    if (hasWildcards_) {
      throw std::runtime_error("Cannot get raw tokens if path has wildcards");
    }
    return tokens_;
  }

  const std::vector<std::string>& idTokens() const {
    if (hasWildcards_) {
      throw std::runtime_error("Cannot get raw tokens if path has wildcards");
    }
    return idTokens_;
  }

  const std::vector<facebook::fboss::fsdb::OperPathElem>& extendedTokens()
      const {
    return extendedTokens_;
  }

  facebook::fboss::fsdb::ExtendedOperPath extendedPath() const {
    facebook::fboss::fsdb::ExtendedOperPath path;
    path.path() = extendedTokens_;
    return path;
  }

  bool matchesPath(const std::vector<std::string>& other) const {
    if (other.size() != extendedTokens_.size()) {
      return false;
    }
    using OperPathElem = facebook::fboss::fsdb::OperPathElem;
    for (int i = 0; i < other.size(); i++) {
      const auto& elem = extendedTokens_.at(i);
      const auto& token = other.at(i);
      if (elem.getType() == OperPathElem::Type::raw) {
        if (token != idTokens_.at(i) && token != tokens_.at(i)) {
          // raw token didn't match either id or name token
          return false;
        }
      } else if (elem.getType() == OperPathElem::Type::regex) {
        if (!re2::RE2::FullMatch(token, *elem.regex())) {
          return false;
        }
      } else if (elem.getType() == OperPathElem::Type::any) {
        // always match
      }
    }
    // no violations
    return true;
  }

  std::string str() const {
    // TODO: better format
    return "/" + folly::join('/', tokens_.begin(), tokens_.end());
  }

 protected:
  std::vector<std::string> tokens_;
  std::vector<std::string> idTokens_;
  // ids by default
  std::vector<facebook::fboss::fsdb::OperPathElem> extendedTokens_;
  bool hasWildcards_;
};

template <
    typename _DataT,
    typename _RootT,
    typename _TC,
    typename _Tag,
    typename _ParentT>
class Path : public BasePath {
 public:
  using DataT = _DataT;
  using RootT = _RootT;
  using TC = _TC;
  using Tag = _Tag;
  using ParentT = _ParentT;

  using BasePath::BasePath;
};

std::vector<std::string> copyAndExtendVec(
    const std::vector<std::string>& parents,
    std::string last);
std::vector<facebook::fboss::fsdb::OperPathElem> copyAndExtendVec(
    const std::vector<facebook::fboss::fsdb::OperPathElem>& parents,
    facebook::fboss::fsdb::OperPathElem last);

std::string pathElemToString(const facebook::fboss::fsdb::OperPathElem& elem);

template <
    typename T,
    typename Root,
    typename Parent,
    typename Tag = apache::thrift::type::infer_tag<T>>
class ChildThriftPath;

namespace detail {

template <
    typename DataT,
    typename Root,
    typename Parent,
    typename Tag,
    bool = is_complex_type_tag_v<Tag>>
struct path_for_tag {
  using type = Path<DataT, Root, type_class_t<Tag>, Tag, Parent>;
};

template <typename DataT, typename Root, typename Parent, typename Tag>
struct path_for_tag<DataT, Root, Parent, Tag, true> {
  using type = ChildThriftPath<DataT, Root, Parent, Tag>;
};

template <typename DataT, typename Root, typename Parent, typename Tag>
using path_for_tag_t = typename path_for_tag<DataT, Root, Parent, Tag>::type;

template <typename Tag>
struct container_traits;

template <typename ValueTag>
struct container_traits<apache::thrift::type::list<ValueTag>> {
  using key_type = std::int32_t;
  using value_tag = ValueTag;
};

template <typename ValueTag>
struct container_traits<apache::thrift::type::set<ValueTag>> {
  using key_type = apache::thrift::type::native_type<ValueTag>;
  using value_tag = ValueTag;
};

template <typename KeyTag, typename ValueTag>
struct container_traits<apache::thrift::type::map<KeyTag, ValueTag>> {
  using key_type = apache::thrift::type::native_type<KeyTag>;
  using value_tag = ValueTag;
};

} // namespace detail

template <typename DataT, typename Root, typename Parent, typename StructT>
class StructuredThriftPath
    : public Path<
          DataT,
          Root,
          detail::type_class_t<apache::thrift::type::infer_tag<StructT>>,
          apache::thrift::type::infer_tag<StructT>,
          Parent> {
 public:
  using Self = Path<
      DataT,
      Root,
      detail::type_class_t<apache::thrift::type::infer_tag<StructT>>,
      apache::thrift::type::infer_tag<StructT>,
      Parent>;

  template <typename Id>
  using TypeFor = detail::path_for_tag_t<
      apache::thrift::op::get_native_type<StructT, Id>,
      Root,
      Self,
      apache::thrift::op::get_type_tag<StructT, Id>>;

  using Self::Self;

  template <typename Id>
  TypeFor<Id> get() const& {
    const std::string childId = folly::to<std::string>(
        folly::to_underlying(apache::thrift::op::get_field_id_v<StructT, Id>));
    facebook::fboss::fsdb::OperPathElem elem;
    elem.set_raw(childId);
    return TypeFor<Id>(
        copyAndExtendVec(
            this->tokens_,
            std::string(apache::thrift::op::get_field_name<StructT, Id>())),
        copyAndExtendVec(this->idTokens_, childId),
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),
        this->hasWildcards_);
  }

  template <typename Id>
  TypeFor<Id> get() && {
    this->tokens_.emplace_back(
        apache::thrift::op::get_field_name<StructT, Id>());
    const std::string childId = folly::to<std::string>(
        folly::to_underlying(apache::thrift::op::get_field_id_v<StructT, Id>));
    this->idTokens_.push_back(childId);
    facebook::fboss::fsdb::OperPathElem elem;
    elem.set_raw(childId);
    this->extendedTokens_.push_back(std::move(elem));
    return TypeFor<Id>(
        std::move(this->tokens_),
        std::move(this->idTokens_),
        std::move(this->extendedTokens_),
        this->hasWildcards_);
  }

  template <apache::thrift::FieldId Id>
  auto operator()(
      const std::integral_constant<apache::thrift::FieldId, Id>&) const& {
    using Ident = apache::thrift::op::
        get_ident<StructT, apache::thrift::type::field_id_tag<Id>>;
    return get<Ident>();
  }

  template <apache::thrift::FieldId Id>
  auto operator()(
      const std::integral_constant<apache::thrift::FieldId, Id>&) && {
    using Ident = apache::thrift::op::
        get_ident<StructT, apache::thrift::type::field_id_tag<Id>>;
    return std::move(*this).template get<Ident>();
  }
};

template <typename DataT, typename Root, typename Parent, typename Tag>
class ContainerThriftPath
    : public Path<
          DataT,
          Root,
          detail::type_class_t<detail::unwrap_type_tag_t<Tag>>,
          detail::unwrap_type_tag_t<Tag>,
          Parent> {
 private:
  using ContainerTag = detail::unwrap_type_tag_t<Tag>;
  using Traits = detail::container_traits<ContainerTag>;

 public:
  using Self = Path<
      DataT,
      Root,
      detail::type_class_t<ContainerTag>,
      ContainerTag,
      Parent>;
  using Child = detail::path_for_tag_t<
      apache::thrift::type::native_type<typename Traits::value_tag>,
      Root,
      Self,
      typename Traits::value_tag>;
  using Key = typename Traits::key_type;

  using Self::Self;

  Child operator[](Key token) const& {
    const std::string strToken = folly::to<std::string>(token);
    facebook::fboss::fsdb::OperPathElem elem;
    elem.set_raw(strToken);
    return Child(
        copyAndExtendVec(this->tokens_, strToken),
        copyAndExtendVec(this->idTokens_, strToken),
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),
        this->hasWildcards_);
  }

  Child operator[](Key token) && {
    const std::string strToken = folly::to<std::string>(token);
    this->tokens_.push_back(strToken);
    this->idTokens_.push_back(strToken);
    facebook::fboss::fsdb::OperPathElem elem;
    elem.set_raw(strToken);
    this->extendedTokens_.push_back(std::move(elem));
    return Child(
        std::move(this->tokens_),
        std::move(this->idTokens_),
        std::move(this->extendedTokens_),
        this->hasWildcards_);
  }

  Child operator[](facebook::fboss::fsdb::OperPathElem elem) const& {
    const auto token = pathElemToString(elem);
    return Child(
        copyAndExtendVec(this->tokens_, token),
        copyAndExtendVec(this->idTokens_, token),
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),
        true);
  }

  Child operator[](facebook::fboss::fsdb::OperPathElem elem) && {
    const auto token = pathElemToString(elem);
    this->tokens_.push_back(token);
    this->idTokens_.push_back(token);
    this->extendedTokens_.push_back(std::move(elem));
    return Child(
        std::move(this->tokens_),
        std::move(this->idTokens_),
        std::move(this->extendedTokens_),
        true);
  }
};

template <typename T>
class RootThriftPath {
 public:
  // While this is always-false, it is dependent and therefore fires only
  // at instantiation time.
  static_assert(
      !std::is_same<T, T>::value,
      "You need to include the header file that the thriftpath plugin "
      "generated for T in order to use RootThriftPath<T>. Also ensure that "
      "you have annotated your root struct with (thriftpath.root)");
};

template <typename T, typename Root, typename Parent, typename Tag>
class ChildThriftPath : public ContainerThriftPath<T, Root, Parent, Tag> {
  // Only list/set/map reach the primary template; struct/union/exception types
  // are handled by the specialization the thriftpath plugin generates for them.
  // A non-container Tag here therefore means the generated header for T was not
  // included -- fire with guidance rather than a cryptic incomplete-type error
  // deep inside ContainerThriftPath.
  static_assert(
      detail::is_container_type_tag_v<Tag>,
      "You need to include the header file that the thriftpath plugin "
      "generated for T in order to use ChildThriftPath<T>.");

 public:
  using ContainerThriftPath<T, Root, Parent, Tag>::ContainerThriftPath;
};

} // namespace thriftpath
