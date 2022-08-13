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
  using type = std::shared_ptr<ThriftStructNode<TType>>;
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

template <typename KeyT, typename ValueT, typename TType>
struct ConvertToNodeTraits<
    apache::thrift::type_class::map<KeyT, ValueT>,
    TType> {
  using type = std::shared_ptr<
      ThriftMapNode<apache::thrift::type_class::map<KeyT, ValueT>, TType>>;
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

} // namespace facebook::fboss::thrift_cow
