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

namespace facebook::fboss::thrift_cow {

// used to denote node vs field types
struct NodeType {};

struct FieldsType {};

template <typename TType>
struct DefaultTypeResolver {
  // if resolver is not specialized for given thrift type, don't specify a type
  using type = void;
};

template <
    typename TType,
    template <typename> typename Resolver = DefaultTypeResolver>
struct ThriftStructFields;

template <
    typename TType,
    template <typename> typename Resolver = DefaultTypeResolver>
class ThriftStructNode;

template <typename TType>
struct ThriftUnionFields;

template <typename TType>
class ThriftUnionNode;

template <typename TypeClass, typename TType>
struct ThriftListFields;

template <typename TypeClass, typename TType>
class ThriftListNode;

template <typename TypeClass, typename TType>
struct ThriftMapFields;

template <typename TypeClass, typename TType>
class ThriftMapNode;

template <typename TypeClass, typename TType>
struct ThriftSetFields;

template <typename TypeClass, typename TType>
class ThriftSetNode;

template <typename TypeClass, typename TType, bool Immutable = false>
class ThriftPrimitiveNode;

} // namespace facebook::fboss::thrift_cow

// clang-format off
#include "fboss/thrift_cow/nodes/ThriftPrimitiveNode-inl.h"
#include "fboss/thrift_cow/nodes/Traits.h"
#include "fboss/thrift_cow/nodes/ThriftStructNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftListNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftMapNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftSetNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftUnionNode-inl.h"
// clang-format on
