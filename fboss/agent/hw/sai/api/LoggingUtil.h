/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 * *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/lib/TupleUtils.h"

#include <fmt/format.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

folly::StringPiece saiApiTypeToString(sai_api_t apiType);
folly::StringPiece saiObjectTypeToString(sai_object_type_t objectType);
folly::StringPiece saiStatusToString(sai_status_t status);

} // namespace facebook::fboss

/*
 * fmt specializations for the types that we use in SaiApi
 * specifically:
 * any c++ value types used in attributes without one (e.g., folly::MacAddress)
 * sai attribute id enums
 * SaiAttribute itself
 * std::tuple (of attributes, ostensibly)
 */
namespace fmt {

// Formatting for folly::MacAddress
template <>
struct formatter<folly::MacAddress> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const folly::MacAddress& mac, FormatContext& ctx) {
    return format_to(ctx.out(), "{}", mac.toString());
  }
};

// Formatting for folly::IpAddress
template <>
struct formatter<folly::IPAddress> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const folly::IPAddress& ip, FormatContext& ctx) {
    return format_to(ctx.out(), "{}", ip.str());
  }
};

// Formatting for AdapterKeys which are SAI entry structs
template <typename AdapterKeyType>
struct formatter<
    AdapterKeyType,
    char,
    typename std::enable_if_t<
        facebook::fboss::IsSaiEntryStruct<AdapterKeyType>::value>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const AdapterKeyType& key, FormatContext& ctx) {
    return format_to(ctx.out(), "{}", key.toString());
  }
};

// Formatting for SaiAttributes
template <typename AttrEnumT, AttrEnumT AttrEnum, typename DataT>
struct formatter<
    facebook::fboss::SaiAttribute<AttrEnumT, AttrEnum, DataT, void>> {
  using AttrT = facebook::fboss::SaiAttribute<AttrEnumT, AttrEnum, DataT, void>;

  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const AttrT& attr, FormatContext& ctx) {
    return format_to(
        ctx.out(),
        "{}: {}",
        facebook::fboss::AttributeName<AttrT>::value,
        attr.value());
  }
};

} // namespace fmt
