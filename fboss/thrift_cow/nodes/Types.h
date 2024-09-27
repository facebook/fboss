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

#include "fboss/thrift_cow/nodes/Serializer.h"

#include <functional>
#include <memory>
#include <optional>

namespace facebook::fboss::thrift_cow {

namespace detail {

// thrift cow objects are represented either as shared_ptr or optional. when
// const reference to these objects, these are represented as when const
// reference to these objects are returned, they're returned as const
// std::shared_ptr<T>& or const std::optional<T>& . when these references are
// dereferenced (or accessed), the inner type loses constness. this leads to
// crash as non-const methods are invoked on const(or published)  objects.
//
// to avoid this unacceptable behavior, a caller must be aware of always to
// invoke const methods even on children(or nested) objects especially if an
// object is published.
//
// providing this safe reference access wrapper, so called won't have to worry
// on constness of inner type.
//
// if a const shared_ptr<T>& or const std::optional<T>& is dereferenced either
// with * operator or -> operator, inner type is returned with constness of
// reference. this ensures method with an appropriate constness is invoked on
// thrift cow object.
template <typename Type>
struct ReferenceWrapper : std::reference_wrapper<Type> {
  using Base = std::reference_wrapper<Type>;
  using Base::Base;

  template <
      typename T = Type,
      std::enable_if_t<!std::is_const_v<T>, bool> = true>
  auto& operator*() {
    return *(this->get());
  }

  template <
      typename T = Type,
      std::enable_if_t<std::is_const_v<T>, bool> = true>
  const auto& operator*() const {
    return std::as_const(*(this->get()));
  }

  template <
      typename T = Type,
      std::enable_if_t<!std::is_const_v<T>, bool> = true>
  auto* operator->() {
    return std::addressof(**this);
  }

  template <
      typename T = Type,
      std::enable_if_t<std::is_const_v<T>, bool> = true>
  const auto* operator->() const {
    return std::addressof(**this);
  }

  template <
      typename T = Type,
      std::enable_if_t<!std::is_const_v<T>, bool> = true>
  void reset() {
    this->get().reset();
  }

  explicit operator bool() const {
    return bool(this->get());
  }

  auto& unwrap() const {
    return this->get();
  }
};

} // namespace detail

// used to denote node vs field types
struct NodeType {};

struct FieldsType {};

// used to differentiate from COW NodeType
struct HybridNodeType {};

template <typename TType, typename Derived>
struct ThriftStructFields;

template <typename TypeClass, typename TType>
struct ThriftHybridNode;

template <typename TType, typename>
class ThriftStructNode;

template <typename TType>
struct ThriftUnionFields;

template <typename TType>
class ThriftUnionNode;

template <typename TypeClass, typename TType>
struct ThriftListFields;

template <typename TypeClass, typename TType>
class ThriftListNode;

template <typename Traits>
struct ThriftMapFields;

template <typename Traits, typename>
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
#include "fboss/thrift_cow/nodes/ThriftHybridNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftStructNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftListNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftMapNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftSetNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftUnionNode-inl.h"
// clang-format on
