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

#include <memory>
#include <optional>
#include <random>
#include <type_traits>
#include <vector>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwSwitch;
class SwitchState;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf);
folly::MacAddress getFirstInterfaceMac(const cfg::SwitchConfig& cfg);
folly::MacAddress getFirstInterfaceMac(
    const std::shared_ptr<SwitchState>& state);
std::optional<VlanID> firstVlanID(const cfg::SwitchConfig& cfg);
std::optional<VlanID> firstVlanID(const std::shared_ptr<SwitchState>& state);
VlanID getIngressVlan(const std::shared_ptr<SwitchState>& state, PortID port);

std::unique_ptr<facebook::fboss::TxPacket> makeLLDPPacket(
    const HwSwitch* hw,
    const folly::MacAddress srcMac,
    std::optional<VlanID> vlanid,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities);

} // namespace facebook::fboss::utility
