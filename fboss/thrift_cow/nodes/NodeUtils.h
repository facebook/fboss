// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/thrift_cow/nodes/Serializer.h"

#include <fatal/type/enum.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace facebook::fboss::thrift_cow {

// Parse keys for maps and sets. Keeping this outside nodes to reduce
// instatiations because it only depends on the type and tc of the key, not the
// type of the node itself.
template <typename KeyT, typename KeyTC>
std::optional<KeyT> tryParseKey(const std::string& token) {
  if constexpr (std::
                    is_same_v<KeyTC, apache::thrift::type_class::enumeration>) {
    // special handling for enum keyed maps
    KeyT enumKey;
    if (apache::thrift::util::tryParseEnum(token, &enumKey)) {
      return enumKey;
    }
  }
  auto key = folly::tryTo<KeyT>(token);
  if (key.hasValue()) {
    return *key;
  }

  return std::nullopt;
}

// All thrift_cow types subclass Serializable, use that to figure out if this is
// our type or a thrift type
template <typename T>
using is_cow_type = std::is_base_of<Serializable, std::remove_cvref_t<T>>;
template <typename T>
constexpr bool is_cow_type_v = is_cow_type<T>::value;

} // namespace facebook::fboss::thrift_cow
