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
folly::StringPiece packetRxReasonToString(cfg::PacketRxReason rxReason);

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

// Formatting for sai_port_eye_values_list_t
template <>
struct formatter<sai_port_lane_eye_values_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const sai_port_lane_eye_values_t& eyeVal, FormatContext& ctx) {
    return format_to(
        ctx.out(),
        "(eye_value: eyeVal.lane: {}, "
        "eyeVal.left: {}, eyeVal.right: {}, "
        "eyeVal.up: {}, eyeVal.down: {})",
        eyeVal.lane,
        eyeVal.left,
        eyeVal.right,
        eyeVal.up,
        eyeVal.down);
  }
};

#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
// Formatting for sai_prbs_rx_state_t
template <>
struct formatter<sai_prbs_rx_state_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const sai_prbs_rx_state_t& prbsStats, FormatContext& ctx) {
    return format_to(
        ctx.out(),
        "(PRBS Rx stats:{}, "
        "PRBS error count: {})",
        prbsStats.rx_status,
        prbsStats.error_count);
  }
};
#endif

// Formatting for sai_port_err_status_list_t
template <>
struct formatter<sai_port_err_status_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const sai_port_err_status_t& errStatus, FormatContext& ctx) {
    return format_to(ctx.out(), "{}", static_cast<int>(errStatus));
  }
};

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3) || defined(TAJO_SDK_VERSION_1_42_8)
// Formatting for sai_port_lane_latch_status_list_t
template <>
struct formatter<sai_port_lane_latch_status_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const sai_port_lane_latch_status_t& latchStatus,
      FormatContext& ctx) {
    return format_to(
        ctx.out(),
        "(lane_latch_status: lane_latch_status.lane: {}, "
        "lane_latch_status.value.current_status: {}, lane_latch_status.value.changed: {}",
        latchStatus.lane,
        latchStatus.value.current_status,
        latchStatus.value.changed);
  }
};
#endif

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

// Formatting for char[32]
template <>
struct formatter<SaiCharArray32> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>

  auto format(const SaiCharArray32& data, FormatContext& ctx) {
    return format_to(ctx.out(), "{}", std::string(data.begin(), data.end()));
  }
};

template <>
struct formatter<facebook::fboss::SaiPortDescriptor> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const facebook::fboss::SaiPortDescriptor& port,
      FormatContext& ctx) {
    return format_to(ctx.out(), "{}", port.str());
  }
};

// Formatting for sai_system_port_config_t
template <>
struct formatter<sai_system_port_config_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const sai_system_port_config_t& sysPortConf, FormatContext& ctx) {
    return format_to(
        ctx.out(),
        "(port_id: {}, switch_id: {}, "
        " attached_core_index: {}, attached_core_port_index: {}, "
        " speed: {}, num_voqs: {})",
        sysPortConf.port_id,
        sysPortConf.attached_switch_id,
        sysPortConf.attached_core_index,
        sysPortConf.attached_core_port_index,
        sysPortConf.speed,
        sysPortConf.num_voq);
  }
};

template <>
struct formatter<sai_fabric_port_reachability_t> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const sai_fabric_port_reachability_t& reach, FormatContext& ctx) {
    return format_to(
        ctx.out(),
        "(SwitchId: {}, "
        " Reachable: {})",
        reach.switch_id,
        reach.reachable);
  }
};
} // namespace fmt
