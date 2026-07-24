/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/PacketTestUtils.h"

#include <exception>

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/packet/tajo/TajoPuntHeader.h"

namespace facebook::fboss::utility {

std::unique_ptr<facebook::fboss::TxPacket> makeLLDPPacket(
    const SwSwitch* sw,
    const folly::MacAddress srcMac,
    std::optional<VlanID> vlanid,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities) {
  return LldpManager::createLldpPkt(
      makeAllocator(sw),
      srcMac,
      vlanid,
      hostname,
      portname,
      portdesc,
      ttl,
      capabilities);
}

// parse the packet to evaluate if this is PTP packet or not
bool isPtpEventPacket(folly::io::Cursor& cursor) {
  EthHdr ethHdr(cursor);
  if (ethHdr.etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
    IPv6Hdr ipHdr(cursor);
    if (ipHdr.nextHeader != static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
      return false;
    }
  } else if (
      ethHdr.etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
    IPv4Hdr ipHdr(cursor);
    if (ipHdr.protocol != static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
      return false;
    }
  } else {
    // packet doesn't have ipv6/ipv4 header
    return false;
  }

  UDPHeader udpHdr;
  udpHdr.parse(&cursor, nullptr);
  if ((udpHdr.srcPort != PTP_UDP_EVENT_PORT) &&
      (udpHdr.dstPort != PTP_UDP_EVENT_PORT)) {
    return false;
  }
  return true;
}

bool isTajoMirrorOnDropPacket(folly::io::Cursor& cursor) {
  // A Tajo Mirror-on-Drop export is outer Eth / IPv6 / UDP followed by the
  // Tajo punt header. Identify it by successfully parsing that punt header off
  // the wire, so detection needs no test-specific collector IP or port.
  //
  // The header parsers throw on a short/malformed frame, and the only caller
  // (PacketSnooper::expectedReceivedPacketType) is noexcept -- so treat any
  // parse failure as "not a MoD packet" rather than letting it escape and
  // std::terminate the process.
  try {
    EthHdr ethHdr(cursor);
    if (ethHdr.etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    IPv6Hdr ipHdr(cursor);
    if (ipHdr.nextHeader != static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
      return false;
    }
    UDPHeader udpHdr;
    // Single-arg overload throws plain std::out_of_range on a short frame
    // (caught below); the two-arg overload would null-deref its PortStats* arg.
    udpHdr.parse(&cursor);
    return parseTajoPuntHeader(cursor).parsedByWire;
  } catch (const std::exception&) {
    return false;
  }
}

uint8_t getIpHopLimit(folly::io::Cursor& cursor) {
  EthHdr ethHdr(cursor);
  if (ethHdr.etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
    IPv6Hdr ipHdr(cursor);
    return ipHdr.hopLimit;
  } else if (
      ethHdr.etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
    IPv4Hdr ipHdr(cursor);
    return ipHdr.ttl;
  }
  throw FbossError("Not a valid IP packet : ", ethHdr.etherType);
}

template <typename SwitchT, typename VlanOrIntfT>
std::optional<VlanID> getSwitchVlanIDForTx(
    const SwitchT* switchPtr,
    std::shared_ptr<VlanOrIntfT> vlanOrIntf) {
  if constexpr (std::is_same_v<SwitchT, SwSwitch>) {
    return getVlanIDForTx(
        vlanOrIntf,
        switchPtr->getState(),
        switchPtr->getScopeResolver(),
        switchPtr->getHwAsicTable());
  } else if constexpr (std::is_same_v<SwitchT, HwSwitch>) {
    return getVlanIDForTx(
        vlanOrIntf,
        // HwSwitch should have its own switch settings
        utility::getFirstNodeIf(
            switchPtr->getProgrammedState()->getSwitchSettings()),
        switchPtr->getPlatform()->getAsic());
  } else {
    static_assert(
        std::is_same_v<SwitchT, SwSwitch> || std::is_same_v<SwitchT, HwSwitch>,
        "Unsupported switch type");
  }
}

template std::optional<VlanID> getSwitchVlanIDForTx<SwSwitch, Vlan>(
    const SwSwitch* switchPtr,
    std::shared_ptr<Vlan> vlanOrIntf);
template std::optional<VlanID> getSwitchVlanIDForTx<SwSwitch, Interface>(
    const SwSwitch* switchPtr,
    std::shared_ptr<Interface> vlanOrIntf);

template std::optional<VlanID> getSwitchVlanIDForTx<HwSwitch, Vlan>(
    const HwSwitch* switchPtr,
    std::shared_ptr<Vlan> vlanOrIntf);
template std::optional<VlanID> getSwitchVlanIDForTx<HwSwitch, Interface>(
    const HwSwitch* switchPtr,
    std::shared_ptr<Interface> vlanOrIntf);

} // namespace facebook::fboss::utility
