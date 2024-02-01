// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fatal/type/enum.h>
#include <folly/Conv.h>
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
    if (fatal::enum_traits<KeyT>::try_parse(enumKey, token)) {
      return enumKey;
    }
  }
  auto key = folly::tryTo<KeyT>(token);
  if (key.hasValue()) {
    return *key;
  }

  return std::nullopt;
}

} // namespace facebook::fboss::thrift_cow
