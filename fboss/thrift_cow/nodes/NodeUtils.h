// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/thrift_cow/nodes/Serializer.h"

#include <fatal/type/enum.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

// All thrift_cow types subclass Serializable, use that to figure out if this is
// our type or a thrift type
template <typename T>
using is_cow_type = std::is_base_of<Serializable, std::remove_cvref_t<T>>;
template <typename T>
constexpr bool is_cow_type_v = is_cow_type<T>::value;

} // namespace facebook::fboss::thrift_cow
