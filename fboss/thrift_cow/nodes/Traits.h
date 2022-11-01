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

#include <fboss/thrift_cow/nodes/Types.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

template <typename TType>
struct ThriftStructResolver {
  // if resolver is not specialized for given thrift type, default to
  // ThriftStructNode
  using type = ThriftStructNode<TType, ThriftStructResolver<TType>>;
};

template <typename Traits>
struct ThriftMapResolver {
  // if resolver is not specialized for given thrift type, default to
  // ThriftStructNode
  using type = ThriftMapNode<Traits, ThriftMapResolver<Traits>>;
};

#define ADD_THRIFT_RESOLVER_MAPPING(ThriftType, CppType)                 \
  /* make sure resolved type is declared */                              \
  class CppType;                                                         \
  template <>                                                            \
  struct facebook::fboss::thrift_cow::ThriftStructResolver<ThriftType> { \
    using type = CppType;                                                \
  };

#define ADD_THRIFT_MAP_RESOLVER_MAPPING(Traits, CppType)          \
  /* make sure resolved type is declared */                       \
  class CppType;                                                  \
  template <>                                                     \
  struct facebook::fboss::thrift_cow::ThriftMapResolver<Traits> { \
    using type = CppType;                                         \
  };

template <typename TType>
using ResolvedType = typename ThriftStructResolver<TType>::type;

template <typename Traits>
using ResolvedMapType = typename ThriftMapResolver<Traits>::type;

template <typename TC, typename TType>
struct ConvertToNodeTraits {
  // we put primitives in optionals so that access patterns are the
  // same for all node types. Either will get std::optional<Field> or
  // std::shared_ptr<Child>. Could potentially replace w/ a ref class
  // that provides a common access pattern, but allows storing
  // non-optional primitive fields inline.
  using type = std::optional<ThriftPrimitiveNode<TC, TType>>;
  using isChild = std::false_type;
};

template <typename TType>
struct ConvertToNodeTraits<apache::thrift::type_class::structure, TType> {
  using default_type = ThriftStructNode<TType, ThriftStructResolver<TType>>;
  using struct_type = ResolvedType<TType>;
  static_assert(
      std::is_base_of_v<default_type, struct_type>,
      "Resolved type needs to be a subclass of ThriftStructNode");
  static_assert(
      sizeof(default_type) == sizeof(struct_type),
      "Resolved type should not define any members. All data needs to be stored in defined thrift struct");

  using type = std::shared_ptr<struct_type>;
  using isChild = std::true_type;
};

template <typename TType>
struct ConvertToNodeTraits<apache::thrift::type_class::variant, TType> {
  using type = std::shared_ptr<ThriftUnionNode<TType>>;
  using isChild = std::true_type;
};

template <typename ValueT, typename TType>
struct ConvertToNodeTraits<apache::thrift::type_class::list<ValueT>, TType> {
  using type = std::shared_ptr<
      ThriftListNode<apache::thrift::type_class::list<ValueT>, TType>>;
  using isChild = std::true_type;
};

template <
    typename TypeClass,
    typename TType,
    template <typename...> typename ConvertToNodeTraitsT = ConvertToNodeTraits>
struct ThriftMapTraits {
  using TC = TypeClass;
  using Type = TType;
  using KeyType = typename TType::key_type;
  template <typename... T>
  using ConvertToNodeTraits = ConvertToNodeTraitsT<T...>;
};

template <typename KeyT, typename ValueT, typename TType>
struct ConvertToNodeTraits<
    apache::thrift::type_class::map<KeyT, ValueT>,
    TType> {
  using type_class = apache::thrift::type_class::map<KeyT, ValueT>;
  using map_type = ResolvedMapType<ThriftMapTraits<type_class, TType>>;
  using type = std::shared_ptr<map_type>;
  using isChild = std::true_type;
};

template <typename ValueT, typename TType>
struct ConvertToNodeTraits<apache::thrift::type_class::set<ValueT>, TType> {
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

template <typename Member>
struct StructMemberTraits {
  using member = Member;
  using traits = StructMemberTraits<Member>;
  using name = typename Member::name;
  using ttype = typename Member::type;
  using tc = typename Member::type_class;
  // need to resolve here
  using type = typename ConvertToNodeTraits<tc, ttype>::type;
  using isChild = typename ConvertToNodeTraits<tc, ttype>::isChild;
};

struct ExtractStructFields {
  template <typename T>
  using apply = StructMemberTraits<T>;
};

template <typename Member>
struct UnionMemberTraits {
  using member = Member;
  using traits = UnionMemberTraits<Member>;
  using name = typename Member::metadata::name;
  using ttype = typename Member::type;
  using tc = typename Member::metadata::type_class;
  using type = typename ConvertToNodeTraits<tc, ttype>::type;
  using isChild = typename ConvertToNodeTraits<tc, ttype>::isChild;
};

struct ExtractUnionFields {
  template <typename T>
  using apply = UnionMemberTraits<T>;
};

template <typename Node, typename MemberName>
struct TypeClassForT {
  using type = typename ThriftStructFields<Node>::template MemberFor<
      MemberName>::type_class;
};

template <typename Node, typename MemberName>
using TypeClassFor = typename TypeClassForT<Node, MemberName>::type;

} // namespace facebook::fboss::thrift_cow
