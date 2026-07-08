/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <configerator/structs/neteng/fboss/thrift/gen-cpp2/common_types.h>
#include <fboss/thrift_cow/nodes/Types.h>
#include <fboss/thrift_cow/visitors/VisitorUtils.h>
#include <thrift/lib/cpp2/gen/module_types_h.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/type/Field.h>
#include <type_traits>

namespace facebook::fboss::thrift_cow {

// Concept to check if a Thrift field is annotated with AllowSkipThriftCow at
// compile time
template <typename TStruct, typename FieldId>
concept field_allow_skip_thrift_cow =
    apache::thrift::is_thrift_class_v<TStruct> &&
    apache::thrift::has_field_annotation<
        facebook::neteng::fboss::common::AllowSkipThriftCow,
        TStruct,
        FieldId>();

// Concept to check if a Thrift type is annotated with AllowSkipThriftCow at
// compile time
template <typename TType>
concept type_allow_skip_thrift_cow =
    apache::thrift::is_thrift_class_v<TType> &&
    apache::thrift::has_struct_annotation<
        facebook::neteng::fboss::common::AllowSkipThriftCow,
        TType>();

template <typename TType, bool EnableHybridStorage = false>
struct ThriftStructResolver {
  // if resolver is not specialized for given thrift type, default to
  // ThriftStructNode
  using type = ThriftStructNode<
      TType,
      ThriftStructResolver<TType, EnableHybridStorage>,
      EnableHybridStorage>;
};

template <typename Traits, bool EnableHybridStorage = false>
struct ThriftMapResolver {
  // if resolver is not specialized for given thrift type, default to
  // ThriftStructNode
  using type = ThriftMapNode<
      Traits,
      ThriftMapResolver<Traits, EnableHybridStorage>,
      EnableHybridStorage>;
};

#define ADD_THRIFT_RESOLVER_MAPPING(ThriftType, CppType) \
  /* make sure resolved type is declared */              \
  class CppType;                                         \
  template <>                                            \
  struct thrift_cow::ThriftStructResolver<ThriftType> {  \
    using type = CppType;                                \
  };

#define ADD_THRIFT_MAP_RESOLVER_MAPPING(Traits, CppType)          \
  /* make sure resolved type is declared */                       \
  class CppType;                                                  \
  template <>                                                     \
  struct facebook::fboss::thrift_cow::ThriftMapResolver<Traits> { \
    using type = CppType;                                         \
  };

template <typename TType, bool EnableHybridStorage>
using ResolvedType =
    typename ThriftStructResolver<TType, EnableHybridStorage>::type;

template <typename Traits, bool EnableHybridStorage>
using ResolvedMapType =
    typename ThriftMapResolver<Traits, EnableHybridStorage>::type;

template <bool EnableHybridStorage, typename TC, typename TType>
struct ConvertToNodeTraits {
  // we put primitives in optionals so that access patterns are the
  // same for all node types. Either will get std::optional<Field> or
  // std::shared_ptr<Child>. Could potentially replace w/ a ref class
  // that provides a common access pattern, but allows storing
  // non-optional primitive fields inline.
  using type = std::optional<ThriftPrimitiveNode<TC, TType>>;
  using isChild = std::false_type;
};

template <bool EnableHybridStorage, typename TType>
struct ConvertToNodeTraits<
    EnableHybridStorage,
    apache::thrift::type_class::structure,
    TType> {
  static constexpr bool skipThriftCow =
      EnableHybridStorage && type_allow_skip_thrift_cow<TType>;
  using default_type = std::conditional_t<
      skipThriftCow,
      ThriftHybridNode<apache::thrift::type_class::structure, TType>,
      ThriftStructNode<
          TType,
          ThriftStructResolver<TType, EnableHybridStorage>,
          EnableHybridStorage>>;
  using struct_type = std::conditional_t<
      skipThriftCow,
      default_type,
      ResolvedType<TType, EnableHybridStorage>>;
  static_assert(
      std::is_base_of_v<default_type, struct_type>,
      "Resolved type needs to be a subclass of ThriftStructNode");
  static_assert(
      sizeof(default_type) == sizeof(struct_type),
      "Resolved type should not define any members. All data needs to be stored in defined thrift struct");

  using type = std::shared_ptr<struct_type>;
  using isChild = std::true_type;
};

template <bool EnableHybridStorage, typename TType>
struct ConvertToNodeTraits<
    EnableHybridStorage,
    apache::thrift::type_class::variant,
    TType> {
  using type = std::shared_ptr<ThriftUnionNode<TType>>;
  using isChild = std::true_type;
};

template <bool EnableHybridStorage, typename ValueT, typename TType>
struct ConvertToNodeTraits<
    EnableHybridStorage,
    apache::thrift::type_class::list<ValueT>,
    TType> {
  using type = std::shared_ptr<ThriftListNode<
      apache::thrift::type_class::list<ValueT>,
      TType,
      EnableHybridStorage>>;
  using isChild = std::true_type;
};

template <bool EnableHybridStorage>
using bool_constant = std::integral_constant<bool, EnableHybridStorage>;

template <
    bool EnableHybridStorage,
    typename TypeClass,
    typename TType,
    template <bool, typename...> typename ConvertToNodeTraitsT =
        ConvertToNodeTraits>
struct ThriftMapTraits {
  using TC = TypeClass;
  using Type = TType;
  using KeyType = typename TType::key_type;
  using KeyCompare = std::less<KeyType>;
  using EnableHybridStorageT = bool_constant<EnableHybridStorage>;
  template <typename EnableHybridStorageT, typename... T>
  using ConvertToNodeTraits = ConvertToNodeTraitsT<EnableHybridStorage, T...>;
};

template <
    bool EnableHybridStorage,
    typename KeyT,
    typename ValueT,
    typename TType>
struct ConvertToNodeTraits<
    EnableHybridStorage,
    apache::thrift::type_class::map<KeyT, ValueT>,
    TType> {
  using type_class = apache::thrift::type_class::map<KeyT, ValueT>;
  using map_type = ResolvedMapType<
      ThriftMapTraits<EnableHybridStorage, type_class, TType>,
      EnableHybridStorage>;
  using type = std::shared_ptr<map_type>;
  using isChild = std::true_type;
};

template <bool EnableHybridStorage, typename ValueT, typename TType>
struct ConvertToNodeTraits<
    EnableHybridStorage,
    apache::thrift::type_class::set<ValueT>,
    TType> {
  using type = std::shared_ptr<
      ThriftSetNode<apache::thrift::type_class::set<ValueT>, TType>>;
  using isChild = std::true_type;
};

template <typename TC, typename TType>
struct ConvertToImmutableNodeTraits {
  // we put primitives in optionals so that access patterns are the
  // same for all node types. Either will get std::optional<Field> or
  // std::shared_ptr<Child>. Could potentially replace w/ a ref class
  // that provides a common access pattern, but allows storing
  // non-optional primitive fields inline.
  using type = std::optional<ImmutableThriftPrimitiveNode<TC, TType>>;
  using isChild = std::false_type;
};

template <typename Derived, typename Ident>
struct ResolveMemberType : std::false_type {};

// Per-field traits computed from modern reflection APIs.
// Replaces the old StructMemberTraits / UnionMemberTraits.
template <
    typename TType,
    typename Id,
    typename Derived,
    bool EnableHybridStorage>
struct CowFieldTraits {
  using Ident = apache::thrift::op::get_ident<TType, Id>;
  using TypeTag = apache::thrift::op::get_type_tag<TType, Id>;
  using NativeType = apache::thrift::op::get_native_type<TType, Id>;
  using TC = typename TypeTagToTypeClass<TypeTag>::type;
  static constexpr auto fieldId = apache::thrift::op::get_field_id_v<TType, Id>;
  static constexpr bool isOptional =
      apache::thrift::type::is_optional_or_union_field_v<TType, Id>;

  static constexpr bool allowSkipThriftCow = EnableHybridStorage &&
      (field_allow_skip_thrift_cow<
           TType,
           apache::thrift::field_id<folly::to_underlying(fieldId)>> ||
       type_allow_skip_thrift_cow<NativeType>);

  using default_type = std::conditional_t<
      allowSkipThriftCow,
      std::shared_ptr<ThriftHybridNode<TC, NativeType>>,
      typename ConvertToNodeTraits<EnableHybridStorage, TC, NativeType>::type>;
  using isChild = std::conditional_t<
      allowSkipThriftCow,
      std::true_type,
      typename ConvertToNodeTraits<EnableHybridStorage, TC, NativeType>::
          isChild>;

  // if the member type is overridden, use the overridden type.
  using type = std::conditional_t<
      ResolveMemberType<Derived, Ident>::value,
      std::shared_ptr<typename ResolveMemberType<Derived, Ident>::type>,
      default_type>;
};

// Build a std::tuple type indexed by ordinal, replacing fatal::tuple_from.
template <
    typename TType,
    typename Derived,
    bool EnableHybridStorage,
    typename Indices>
struct CowStorageImpl;

template <
    typename TType,
    typename Derived,
    bool EnableHybridStorage,
    size_t... Is>
struct CowStorageImpl<
    TType,
    Derived,
    EnableHybridStorage,
    std::index_sequence<Is...>> {
  using type = std::tuple<typename CowFieldTraits<
      TType,
      apache::thrift::type::ordinal<Is + 1>,
      Derived,
      EnableHybridStorage>::type...>;
};

template <typename TType, typename Derived, bool EnableHybridStorage>
using CowStorage = typename CowStorageImpl<
    TType,
    Derived,
    EnableHybridStorage,
    std::make_index_sequence<apache::thrift::op::num_fields<TType>>>::type;

#if __cplusplus <= 202001L
template <typename T>
using TypeIdentity = std::enable_if<true, T>;
#else
template <typename T>
using TypeIdentity = std::type_identity<T>;
#endif

namespace detail {
template <typename Type>
detail::ReferenceWrapper<Type> ref(Type& obj) {
  return ReferenceWrapper<Type>(obj);
}

template <typename Type>
ReferenceWrapper<const Type> cref(const Type& obj) {
  return detail::ReferenceWrapper<const Type>(obj);
}
} // namespace detail

} // namespace facebook::fboss::thrift_cow
