/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentTestPacketUtils.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/packet/PktFactory.h"

using namespace facebook::fboss;
namespace {
auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);
} // namespace

namespace facebook::fboss::utility {

std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    SwSwitch* sw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    std::optional<std::vector<uint8_t>> payload) {
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, etherType);
  auto txPacket = sw->allocatePacket(EthHdr::SIZE + payloadBytes.size());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      vlan,
      ethHdr.getEtherType());
  rwCursor.push(payloadBytes.data(), payloadBytes.size());
  return txPacket;
}
} // namespace facebook::fboss::utility
