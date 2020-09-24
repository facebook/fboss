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
sai_log_level_t saiLogLevelFromString(const std::string& logLevel);

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

// Formatting for std::variant
template <typename... Ts>
struct formatter<std::variant<Ts...>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::variant<Ts...>& var, FormatContext& ctx) {
    auto formatVariant = [&ctx](auto&& val) {
      return format_to(ctx.out(), "{}", val);
    };
    return std::visit(formatVariant, var);
  }
};

// Formatting for SaiAttributes
template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename DefaultGetterT>
struct formatter<
    facebook::fboss::
        SaiAttribute<AttrEnumT, AttrEnum, DataT, DefaultGetterT, void>> {
  using AttrT = facebook::fboss::
      SaiAttribute<AttrEnumT, AttrEnum, DataT, DefaultGetterT, void>;

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

// Formatting for std::monostate
template <>
struct formatter<std::monostate> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::monostate& unit, FormatContext& ctx) {
    return format_to(ctx.out(), "(monostate)");
  }
};

// Formatting for empty std::tuple
template <>
struct formatter<std::tuple<>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::tuple<>& tup, FormatContext& ctx) {
    return format_to(ctx.out(), "()");
  }
};

// Formatting for std::optional<SaiAttribute>
template <typename T>
struct formatter<std::optional<T>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::optional<T>& opt, FormatContext& ctx) {
    static_assert(
        facebook::fboss::IsSaiAttribute<T>::value,
        "format(std::optional) only valid for SaiAttributes");
    if (opt) {
      return format_to(ctx.out(), "{}", opt.value());
    } else {
      return format_to(ctx.out(), "nullopt");
    }
  }
};

// Formatting for sai_qos_map_t
template <>
struct formatter<sai_qos_map_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const sai_qos_map_t& qosMap, FormatContext& ctx) {
    return format_to(
        ctx.out(),
        "(qos_mapping: key.dscp: {}, key.tc: {}, "
        "value.tc: {}, value.queue_index: {})",
        qosMap.key.dscp,
        qosMap.key.tc,
        qosMap.value.tc,
        qosMap.value.queue_index);
  }
};

// Formatting for AclEntryField<T>
template <typename T>
struct formatter<facebook::fboss::AclEntryField<T>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>

  auto format(
      const facebook::fboss::AclEntryField<T>& aclEntryField,
      FormatContext& ctx) {
    return format_to(ctx.out(), "{}", aclEntryField.str());
  }
};

// Formatting for AclEntryAction<T>
template <typename T>
struct formatter<facebook::fboss::AclEntryAction<T>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>

  auto format(
      const facebook::fboss::AclEntryAction<T>& aclEntryAction,
      FormatContext& ctx) {
    return format_to(ctx.out(), "{}", aclEntryAction.str());
  }
};

// Formatting for sai_u32_range_t
template <>
struct formatter<sai_u32_range_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>

  auto format(const sai_u32_range_t& u32Range, FormatContext& ctx) {
    return format_to(
        ctx.out(), "u32 range: min: {}, max: {}", u32Range.min, u32Range.max);
  }
};

// Formatting for sai_s32_range_t
template <>
struct formatter<sai_s32_range_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>

  auto format(const sai_s32_range_t& s32Range, FormatContext& ctx) {
    return format_to(
        ctx.out(), "s32 range: min: {}, max: {}", s32Range.min, s32Range.max);
  }
};

// formatter for extension attributes
template <typename T>
struct formatter<
    T,
    char,
    std::enable_if_t<facebook::fboss::IsSaiExtensionAttribute<T>::value>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const T& attr, FormatContext& ctx) {
    // TODO: implement this
    return format_to(
        ctx.out(),
        "{}: {}",
        facebook::fboss::AttributeName<T>::value,
        attr.value());
  }
};

} // namespace fmt
