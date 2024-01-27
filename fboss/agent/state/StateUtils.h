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

#include <string>

#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

namespace facebook::fboss {
class SwitchState;

namespace utility {

/**
 * Utility functions for InterfaceID <-> ifName (on host)
 */
std::string createTunIntfName(InterfaceID ifID);
bool isTunIntfName(const std::string& ifName);
InterfaceID getIDFromTunIntfName(const std::string& ifName);

template <typename MswitchMapT>
auto getFirstMap(const std::shared_ptr<MswitchMapT>& map) {
  return map->size() ? map->cbegin()->second : nullptr;
}

template <typename MultiSwitchMapT>
auto getFirstNodeIf(const std::shared_ptr<MultiSwitchMapT>& map) {
  return map->size() ? map->cbegin()->second : nullptr;
}

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf);
folly::MacAddress getFirstInterfaceMac(
    const std::shared_ptr<SwitchState>& state);
std::optional<VlanID> firstVlanID(const std::shared_ptr<SwitchState>& state);
VlanID getIngressVlan(const std::shared_ptr<SwitchState>& state, PortID port);
InterfaceID firstInterfaceID(const std::shared_ptr<SwitchState>& state);
} // namespace utility
} // namespace facebook::fboss
