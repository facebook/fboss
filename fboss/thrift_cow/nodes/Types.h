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

#include <functional>
#include <memory>
#include <optional>

namespace facebook::fboss::thrift_cow {

namespace detail {

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

template <typename TType, typename Derived>
struct ThriftStructFields;

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
#include "fboss/thrift_cow/nodes/ThriftStructNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftListNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftMapNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftSetNode-inl.h"
#include "fboss/thrift_cow/nodes/ThriftUnionNode-inl.h"
// clang-format on
