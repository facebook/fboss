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
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::RWPrivateCursor;

namespace {
auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);
} // namespace

namespace facebook::fboss::utility {

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan) {
  return state->getInterfaces()->getInterfaceInVlan(vlan)->getMac();
}
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf) {
  return state->getInterfaces()->getNode(intf)->getMac();
}

folly::MacAddress getFirstInterfaceMac(const cfg::SwitchConfig& cfg) {
  auto intfCfg = cfg.interfaces()[0];

  if (!intfCfg.mac().has_value()) {
    throw FbossError(
        "No MAC address set for interface ", InterfaceID(*intfCfg.intfID()));
  }

  return folly::MacAddress(*intfCfg.mac());
}

folly::MacAddress getFirstInterfaceMac(
    const std::shared_ptr<SwitchState>& state) {
  const auto& intfMap = state->getInterfaces()->cbegin()->second;
  const auto& intf = std::as_const(*intfMap->cbegin()).second;
  return intf->getMac();
}

std::optional<VlanID> firstVlanID(const cfg::SwitchConfig& cfg) {
  std::optional<VlanID> firstVlanId;
  if (cfg.vlanPorts()->size()) {
    firstVlanId = VlanID(*cfg.vlanPorts()[0].vlanID());
  }
  return firstVlanId;
}

std::optional<VlanID> firstVlanID(const std::shared_ptr<SwitchState>& state) {
  std::optional<VlanID> firstVlanId;
  if (state->getVlans()->numNodes()) {
    firstVlanId =
        util::getFirstMap(state->getVlans())->cbegin()->second->getID();
  }
  return firstVlanId;
}

VlanID getIngressVlan(const std::shared_ptr<SwitchState>& state, PortID port) {
  return state->getPorts()->getNode(port)->getIngressVlan();
}

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makePTPUDPTxPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    PTPMessageType ptpPktType) {
  int payloadSize = PTPHeader::getPayloadSize(ptpPktType);
  auto txPacket = hw->allocatePacket(
      ethHdr.size() + ipHdr.size() + udpHdr.size() + payloadSize);

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  writeEthHeader(
      txPacket,
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      ethHdr.getVlanTags(),
      ethHdr.getEtherType());

  ipHdr.serialize(&rwCursor);

  // write UDP header, payload and compute checksum
  rwCursor.writeBE<uint16_t>(udpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(udpHdr.dstPort);
  rwCursor.writeBE<uint16_t>(udpHdr.length);
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  folly::io::Cursor payloadStart(rwCursor);
  // PTPHeader
  PTPHeader ptpHeader(
      static_cast<uint8_t>(ptpPktType),
      static_cast<uint8_t>(PTPVersion::PTP_V2));
  ptpHeader.write(&rwCursor);
  uint16_t csum = udpHdr.computeChecksum(ipHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
}

std::unique_ptr<facebook::fboss::TxPacket> makePTPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint8_t trafficClass,
    uint8_t hopLimit,
    PTPMessageType ptpPktType) {
  int payloadSize = PTPHeader::getPayloadSize(ptpPktType);
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_IPV6);
  // IPv6Hdr
  IPv6Hdr ipHdr(srcIp, dstIp);
  ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  ipHdr.trafficClass = trafficClass;
  ipHdr.payloadLength = UDPHeader::size() + payloadSize;
  ipHdr.hopLimit = hopLimit;
  // UDPHeader
  UDPHeader udpHdr(
      PTP_UDP_EVENT_PORT, PTP_UDP_EVENT_PORT, UDPHeader::size() + payloadSize);

  return makePTPUDPTxPacket(hw, ethHdr, ipHdr, udpHdr, ptpPktType);
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
      makeAllocater(hw),
      srcMac,
      vlanid,
      hostname,
      portname,
      portdesc,
      ttl,
      capabilities);
}

void sendTcpPkts(
    facebook::fboss::HwSwitch* hwSwitch,
    int numPktsToSend,
    std::optional<VlanID> vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    PortID outPort,
    uint8_t trafficClass,
    std::optional<std::vector<uint8_t>> payload) {
  for (int i = 0; i < numPktsToSend; i++) {
    auto txPacket = utility::makeTCPTxPacket(
        hwSwitch,
        vlanId,
        dstMac,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        trafficClass,
        payload);
    hwSwitch->sendPacketOutOfPortSync(std::move(txPacket), outPort);
  }
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
