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

#include <fboss/agent/Constants.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/types.h"

namespace {
inline constexpr auto kMac = "mac";
inline constexpr auto kNeighborResponseIntf = "interfaceId";
} // namespace

namespace facebook::fboss {

template <typename IPADDR>
state::NeighborResponseEntryFields
NeighborResponseEntryFields<IPADDR>::toThrift() const {
  state::NeighborResponseEntryFields entryTh;
  entryTh.ipAddress_ref() = ipAddress.str();
  entryTh.mac_ref() = mac.toString();
  entryTh.interfaceId_ref() = static_cast<uint32_t>(interfaceID);
  return entryTh;
}

template <typename IPADDR>
NeighborResponseEntryFields<IPADDR>
NeighborResponseEntryFields<IPADDR>::fromThrift(
    state::NeighborResponseEntryFields const& entryTh) {
  IPADDR ip(entryTh.get_ipAddress());
  folly::MacAddress mac(entryTh.get_mac());
  InterfaceID intf(entryTh.get_interfaceId());
  return NeighborResponseEntryFields(ip, mac, intf);
}
} // namespace facebook::fboss
