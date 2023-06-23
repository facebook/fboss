// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

template <typename Meta>
std::string getMemberName(bool useId) {
  using name = typename Meta::name;
  using id = typename Meta::id;
  if (useId) {
    return folly::to<std::string>(id::value);
  } else {
    return std::string(fatal::z_data<name>(), fatal::size<name>::value);
  }
}

} // namespace facebook::fboss::thrift_cow
