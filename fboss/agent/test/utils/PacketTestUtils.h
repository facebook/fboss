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

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

namespace facebook::fboss {
class SwSwitch;
class TxPacket;

namespace utility {

std::unique_ptr<TxPacket> makeLLDPPacket(
    const SwSwitch* hw,
    const folly::MacAddress srcMac,
    std::optional<VlanID> vlanid,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities);

bool isPtpEventPacket(folly::io::Cursor& cursor);
uint8_t getIpHopLimit(folly::io::Cursor& cursor);

template <typename SwitchT>
void sendTcpPkts(
    SwitchT* switchPtr,
    int numPktsToSend,
    std::optional<VlanID> vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    PortID outPort,
    uint8_t trafficClass = 0,
    std::optional<std::vector<uint8_t>> payload = std::nullopt) {
  for (int i = 0; i < numPktsToSend; i++) {
    auto txPacket = utility::makeTCPTxPacket(
        switchPtr,
        vlanId,
        dstMac,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        trafficClass,
        payload);
    if constexpr (std::is_same_v<SwitchT, HwSwitch>) {
      switchPtr->sendPacketOutOfPortSync(std::move(txPacket), outPort);
    } else {
      switchPtr->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
    }
  }
}
} // namespace utility
} // namespace facebook::fboss
