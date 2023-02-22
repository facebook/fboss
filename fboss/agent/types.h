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

#include <folly/Conv.h>
#include <iosfwd>

#include <boost/serialization/strong_typedef.hpp>
#include <fmt/format.h>
#include <cstdint>
#include <type_traits>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {
template <typename>
struct is_fboss_key_object_type {
  static constexpr bool value = false;
};
} // namespace facebook::fboss

#define FBOSS_STRONG_TYPE(primitive, new_type)                                \
  namespace facebook::fboss {                                                 \
                                                                              \
  BOOST_STRONG_TYPEDEF(primitive, new_type);                                  \
                                                                              \
  /* Define toAppend() so folly::to<string> will work */                      \
  inline void toAppend(new_type value, std::string* result) {                 \
    folly::toAppend(static_cast<primitive>(value), result);                   \
  }                                                                           \
  inline void toAppend(new_type value, folly::fbstring* result) {             \
    folly::toAppend(static_cast<primitive>(value), result);                   \
  }                                                                           \
                                                                              \
  } /* facebook::fboss */                                                     \
  namespace std {                                                             \
                                                                              \
  template <>                                                                 \
  struct hash<facebook::fboss::new_type> {                                    \
    size_t operator()(const facebook::fboss::new_type& x) const {             \
      return hash<primitive>()(static_cast<primitive>(x));                    \
    }                                                                         \
  };                                                                          \
  } /* std */                                                                 \
                                                                              \
  /* Support formatting with fmt::format as well */                           \
  namespace fmt {                                                             \
  template <>                                                                 \
  struct formatter<facebook::fboss::new_type> {                               \
    template <typename ParseContext>                                          \
    constexpr auto parse(ParseContext& ctx) {                                 \
      return ctx.begin();                                                     \
    }                                                                         \
                                                                              \
    template <typename FormatContext>                                         \
    auto format(const facebook::fboss::new_type& value, FormatContext& ctx) { \
      return format_to(                                                       \
          ctx.out(), "{}({})", #new_type, static_cast<primitive>(value));     \
    }                                                                         \
  };                                                                          \
  } /* fmt */

FBOSS_STRONG_TYPE(uint8_t, ChannelID)
FBOSS_STRONG_TYPE(uint16_t, TransceiverID)
FBOSS_STRONG_TYPE(uint16_t, AggregatePortID)
FBOSS_STRONG_TYPE(uint16_t, PortID)
FBOSS_STRONG_TYPE(uint8_t, PimID)
FBOSS_STRONG_TYPE(uint16_t, VlanID)
FBOSS_STRONG_TYPE(uint32_t, RouterID)
FBOSS_STRONG_TYPE(uint32_t, InterfaceID)
FBOSS_STRONG_TYPE(int, VrfID)
FBOSS_STRONG_TYPE(uint16_t, SwitchID);
FBOSS_STRONG_TYPE(uint16_t, BridgeID);
FBOSS_STRONG_TYPE(uint16_t, TrafficClass);
FBOSS_STRONG_TYPE(uint8_t, DSCP);
FBOSS_STRONG_TYPE(uint8_t, EXP);
FBOSS_STRONG_TYPE(uint32_t, PciVendorId);
FBOSS_STRONG_TYPE(uint32_t, PciDeviceId);
FBOSS_STRONG_TYPE(uint8_t, PfcPriority);
FBOSS_STRONG_TYPE(uint8_t, MdioControllerID);
FBOSS_STRONG_TYPE(uint8_t, PhyAddr);
FBOSS_STRONG_TYPE(uint16_t, GlobalXphyID);
FBOSS_STRONG_TYPE(uint16_t, XphyId);
FBOSS_STRONG_TYPE(uint32_t, LaneID);
FBOSS_STRONG_TYPE(int32_t, LabelID);
FBOSS_STRONG_TYPE(int64_t, SystemPortID);
FBOSS_STRONG_TYPE(uint16_t, IngressPriorityGroupID);

/*
 * A unique ID identifying a node in our state tree.
 */
FBOSS_STRONG_TYPE(uint64_t, NodeID)

/*
 * Timestamp of a stat
 */
FBOSS_STRONG_TYPE(int64_t, StatTimestamp)

namespace facebook::fboss {

using LoadBalancerID = cfg::LoadBalancerID;

namespace cfg {

std::ostream& operator<<(std::ostream& out, LoadBalancerID id);

} // namespace cfg

} // namespace facebook::fboss
