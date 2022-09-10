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

#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

template <typename T>
struct DefaultTypeResolver;

template <typename T, typename Default>
using DefaultIfVoid =
    typename std::conditional_t<std::is_same_v<T, void>, Default, T>;

template <typename TType, template <typename> typename Resolver>
using ResolvedType = DefaultIfVoid<
    typename Resolver<TType>::type,
    ThriftStructNode<TType, Resolver>>;

template <
    typename TC,
    typename TType,
    template <typename> typename Resolver = DefaultTypeResolver>
struct ConvertToNodeTraits {
  // we put primitives in optionals so that access patterns are the
  // same for all node types. Either will get std::optional<Field> or
  // std::shared_ptr<Child>. Could potentially replace w/ a ref class
  // that provides a common access pattern, but allows storing
  // non-optional primitive fields inline.
  using type = std::optional<ThriftPrimitiveNode<TC, TType>>;
  using isChild = std::false_type;
};

template <typename TType, template <typename> typename Resolver>
struct ConvertToNodeTraits<
    apache::thrift::type_class::structure,
    TType,
    Resolver> {
  using default_type = ThriftStructNode<TType, Resolver>;
  using struct_type = ResolvedType<TType, Resolver>;
  static_assert(
      std::is_base_of_v<default_type, struct_type>,
      "Resolved type needs to be a subclass of ThriftStructNode");
  static_assert(
      sizeof(default_type) == sizeof(struct_type),
      "Resolved type should not define any members. All data needs to be stored in defined thrift struct");

  using type = std::shared_ptr<struct_type>;
  using isChild = std::true_type;
};

template <typename TType, template <typename> typename Resolver>
struct ConvertToNodeTraits<
    apache::thrift::type_class::variant,
    TType,
    Resolver> {
  using type = std::shared_ptr<ThriftUnionNode<TType>>;
  using isChild = std::true_type;
};

template <
    typename ValueT,
    typename TType,
    template <typename>
    typename Resolver>
struct ConvertToNodeTraits<
    apache::thrift::type_class::list<ValueT>,
    TType,
    Resolver> {
  using type = std::shared_ptr<
      ThriftListNode<apache::thrift::type_class::list<ValueT>, TType>>;
  using isChild = std::true_type;
};

template <
    typename KeyT,
    typename ValueT,
    typename TType,
    template <typename>
    typename Resolver>
struct ConvertToNodeTraits<
    apache::thrift::type_class::map<KeyT, ValueT>,
    TType,
    Resolver> {
  using type = std::shared_ptr<
      ThriftMapNode<apache::thrift::type_class::map<KeyT, ValueT>, TType>>;
  using isChild = std::true_type;
};

template <
    typename ValueT,
    typename TType,
    template <typename>
    typename Resolver>
struct ConvertToNodeTraits<
    apache::thrift::type_class::set<ValueT>,
    TType,
    Resolver> {
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

template <typename Member, template <typename> typename Resolver>
struct StructMemberTraits {
  using member = Member;
  using traits = StructMemberTraits<Member, Resolver>;
  using name = typename Member::name;
  using ttype = typename Member::type;
  using tc = typename Member::type_class;
  // need to resolve here
  using type = typename ConvertToNodeTraits<tc, ttype, Resolver>::type;
  using isChild = typename ConvertToNodeTraits<tc, ttype, Resolver>::isChild;
};

template <template <typename> typename Resolver = DefaultTypeResolver>
struct ExtractStructFields {
  template <typename T>
  using apply = StructMemberTraits<T, Resolver>;
};

template <
    typename Member,
    template <typename> typename Resolver = DefaultTypeResolver>
struct UnionMemberTraits {
  using member = Member;
  using traits = UnionMemberTraits<Member, Resolver>;
  using name = typename Member::metadata::name;
  using ttype = typename Member::type;
  using tc = typename Member::metadata::type_class;
  using type = typename ConvertToNodeTraits<tc, ttype, Resolver>::type;
  using isChild = typename ConvertToNodeTraits<tc, ttype, Resolver>::isChild;
};

template <template <typename> typename Resolver = DefaultTypeResolver>
struct ExtractUnionFields {
  template <typename T>
  using apply = UnionMemberTraits<T, Resolver>;
};

} // namespace facebook::fboss::thrift_cow
