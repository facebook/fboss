/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestPacketUtils.h"

#include <folly/Range.h>
#include <folly/String.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>

#include "common/logging/logging.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using folly::MacAddress;

namespace facebook::fboss::utility {

folly::MacAddress getFirstInterfaceMac(const cfg::SwitchConfig& cfg) {
  auto intfCfg = cfg.interfaces()[0];

  if (!intfCfg.mac().has_value()) {
    throw FbossError(
        "No MAC address set for interface ", InterfaceID(*intfCfg.intfID()));
  }

  return folly::MacAddress(*intfCfg.mac());
}

std::optional<VlanID> firstVlanID(const cfg::SwitchConfig& cfg) {
  std::optional<VlanID> firstVlanId;
  if (cfg.vlanPorts()->size()) {
    firstVlanId = VlanID(*cfg.vlanPorts()[0].vlanID());
  }
  return firstVlanId;
}

VlanID getIngressVlan(const std::shared_ptr<SwitchState>& state, PortID port) {
  return state->getPorts()->getNode(port)->getIngressVlan();
}

std::unique_ptr<facebook::fboss::TxPacket> makeLLDPPacket(
    const HwSwitch* hw,
    const folly::MacAddress srcMac,
    std::optional<VlanID> vlanid,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities) {
  return LldpManager::createLldpPkt(
      makeAllocator(hw),
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

} // namespace facebook::fboss::utility
