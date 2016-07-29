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

#include <boost/serialization/strong_typedef.hpp>
#include <cstdint>
#include <type_traits>

namespace facebook { namespace fboss {

#define FBOSS_STRONG_TYPE(primitive, new_type) \
  BOOST_STRONG_TYPEDEF(primitive, new_type); \
  \
  /* Define toAppend() so folly::to<string> will work */ \
  inline void toAppend(new_type value, std::string* result) { \
    folly::toAppend(static_cast<primitive>(value), result); \
  } \
  inline void toAppend(new_type value, folly::fbstring* result) { \
    folly::toAppend(static_cast<primitive>(value), result); \
  }

FBOSS_STRONG_TYPE(uint8_t, ChannelID)
FBOSS_STRONG_TYPE(uint16_t, TransceiverID)
FBOSS_STRONG_TYPE(uint16_t, PortID)
FBOSS_STRONG_TYPE(uint16_t, VlanID)
FBOSS_STRONG_TYPE(uint32_t, RouterID)
FBOSS_STRONG_TYPE(uint32_t, InterfaceID)
FBOSS_STRONG_TYPE(uint32_t, AclEntryID)

/*
 * A unique ID identifying a node in our state tree.
 */
FBOSS_STRONG_TYPE(uint64_t, NodeID)

#undef FBOSS_STRONG_TYPE
}} // facebook::fboss
