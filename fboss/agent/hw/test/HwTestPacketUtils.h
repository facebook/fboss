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
#include <vector>

#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Optional.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {
class HwSwitch;
class EthHdr;
class UDPHeader;
} // namespace fboss
} // namespace facebook

namespace facebook {
namespace fboss {
namespace utility {

std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    folly::Optional<std::vector<uint8_t>> payload =
        folly::Optional<std::vector<uint8_t>>());

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    const std::vector<uint8_t>& payload);

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    folly::Optional<std::vector<uint8_t>> payload =
        folly::Optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t dscp = 0,
    uint8_t ttl = 255,
    folly::Optional<std::vector<uint8_t>> payload =
        folly::Optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    folly::Optional<std::vector<uint8_t>> payload =
        folly::Optional<std::vector<uint8_t>>());
} // namespace utility
} // namespace fboss
} // namespace facebook
