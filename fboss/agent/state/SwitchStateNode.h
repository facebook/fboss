// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/thrift_cow/nodes/Types.h"

namespace facebook::fboss {

template <typename T>
struct SwitchStateThriftTypeResolver
    : public thrift_cow::DefaultTypeResolver<T> {};

#define ADD_THRIFT_RESOLVER_MAPPING(ThriftType, CppType) \
  /* make sure resolved type is declared */              \
  class CppType;                                         \
  template <>                                            \
  struct SwitchStateThriftTypeResolver<ThriftType> {     \
    using type = CppType;                                \
  };

template <typenaem TType>
using SwitchStateNode = ThriftStructNode<TType, SwitchStateThriftTypeResolver>;

// TODO: node map definition

} // namespace facebook::fboss
