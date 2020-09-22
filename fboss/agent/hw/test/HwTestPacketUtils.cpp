/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "HwTestPacketUtils.h"

#include <folly/Range.h>
#include <folly/String.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>

#include "common/logging/logging.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/TCPHeader.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::RWPrivateCursor;

namespace {
auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);

EthHdr makeEthHdr(
    MacAddress srcMac,
    MacAddress dstMac,
    VlanID vlan,
    ETHERTYPE etherType) {
  EthHdr::VlanTags_t vlanTags;
  vlanTags.push_back(
      VlanTag(vlan, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN)));
  EthHdr ethHdr(dstMac, srcMac, vlanTags, static_cast<uint16_t>(etherType));
  return ethHdr;
}

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
  return state->getInterfaces()->getInterface(intf)->getMac();
}

VlanID firstVlanID(const cfg::SwitchConfig& cfg) {
  return VlanID(*cfg.vlanPorts_ref()[0].vlanID_ref());
}

std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
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
  auto txPacket = hw->allocatePacket(EthHdr::SIZE + payloadBytes.size());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      vlan,
      ethHdr.getEtherType());
  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payloadBytes.data(), payloadBytes.size());
  return txPacket;
}

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const std::vector<uint8_t>& payload) {
  auto txPacket =
      hw->allocatePacket(EthHdr::SIZE + ipHdr.size() + payload.size());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      VlanID(ethHdr.getVlanTags()[0].vid()),
      ethHdr.getEtherType());
  ipHdr.serialize(&rwCursor);

  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payload.data(), payload.size());
  return txPacket;
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint8_t trafficClass,
    uint8_t hopLimit,
    std::optional<std::vector<uint8_t>> payload) {
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_IPV6);
  // IPv6Hdr
  IPv6Hdr ipHdr(srcIp, dstIp);
  ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  ipHdr.trafficClass = trafficClass;
  ipHdr.payloadLength = UDPHeader::size() + payloadBytes.size();
  ipHdr.hopLimit = hopLimit;

  return makeIpTxPacket(hw, ethHdr, ipHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint8_t dscp,
    uint8_t ttl,
    std::optional<std::vector<uint8_t>> payload) {
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_IPV4);
  // IPv4Hdr
  IPv4Hdr ipHdr(
      srcIp,
      dstIp,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      payloadBytes.size());
  ipHdr.dscp = dscp;
  ipHdr.computeChecksum();
  ipHdr.ttl = ttl;

  return makeIpTxPacket(hw, ethHdr, ipHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    uint8_t trafficClass,
    uint8_t hopLimit,
    std::optional<std::vector<uint8_t>> payload) {
  CHECK_EQ(srcIp.isV6(), dstIp.isV6());
  return srcIp.isV6() ? makeIpTxPacket(
                            hw,
                            vlan,
                            srcMac,
                            dstMac,
                            srcIp.asV6(),
                            dstIp.asV6(),
                            trafficClass,
                            hopLimit,
                            payload)
                      : makeIpTxPacket(
                            hw,
                            vlan,
                            srcMac,
                            dstMac,
                            srcIp.asV4(),
                            dstIp.asV4(),
                            trafficClass,
                            hopLimit,
                            payload);
}

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    const std::vector<uint8_t>& payload) {
  auto txPacket = hw->allocatePacket(
      EthHdr::SIZE + ipHdr.size() + udpHdr.size() + payload.size());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      VlanID(ethHdr.getVlanTags()[0].vid()),
      ethHdr.getEtherType());
  ipHdr.serialize(&rwCursor);

  // write UDP header, payload and compute checksum
  rwCursor.writeBE<uint16_t>(udpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(udpHdr.dstPort);
  rwCursor.writeBE<uint16_t>(udpHdr.length);
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payload.data(), payload.size());
  uint16_t csum = udpHdr.computeChecksum(ipHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
}

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass,
    uint8_t hopLimit,
    std::optional<std::vector<uint8_t>> payload) {
  // TODO: Refactor such that both tests and DHCPv6Handler to use this
  // function for constructing v6 UDP packets
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_IPV6);
  // IPv6Hdr
  IPv6Hdr ipHdr(srcIp, dstIp);
  ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  ipHdr.trafficClass = trafficClass;
  ipHdr.payloadLength = UDPHeader::size() + payloadBytes.size();
  ipHdr.hopLimit = hopLimit;
  // UDPHeader
  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());

  return makeUDPTxPacket(hw, ethHdr, ipHdr, udpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t dscp,
    uint8_t ttl,
    std::optional<std::vector<uint8_t>> payload) {
  // TODO: Refactor such that both tests and DHCPv4Handler to use this
  // function for constructing v4 UDP packets
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_IPV4);
  // IPv4Hdr
  IPv4Hdr ipHdr(
      srcIp,
      dstIp,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      payloadBytes.size());
  ipHdr.dscp = dscp;
  ipHdr.computeChecksum();
  ipHdr.ttl = ttl;
  // UDPHeader
  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());

  return makeUDPTxPacket(hw, ethHdr, ipHdr, udpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass,
    uint8_t hopLimit,
    std::optional<std::vector<uint8_t>> payload) {
  CHECK_EQ(srcIp.isV6(), dstIp.isV6());
  if (srcIp.isV6()) {
    return makeUDPTxPacket(
        hw,
        vlan,
        srcMac,
        dstMac,
        srcIp.asV6(),
        dstIp.asV6(),
        srcPort,
        dstPort,
        trafficClass,
        hopLimit,
        payload);
  }
  return makeUDPTxPacket(
      hw,
      vlan,
      srcMac,
      dstMac,
      srcIp.asV4(),
      dstIp.asV4(),
      srcPort,
      dstPort,
      trafficClass,
      hopLimit,
      payload);
}

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const TCPHeader& tcpHdr,
    const std::vector<uint8_t>& payload) {
  auto txPacket = hw->allocatePacket(
      EthHdr::SIZE + ipHdr.size() + tcpHdr.size() + payload.size());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      VlanID(ethHdr.getVlanTags()[0].vid()),
      ethHdr.getEtherType());
  ipHdr.serialize(&rwCursor);

  // write TCP header, payload and compute checksum
  rwCursor.writeBE<uint16_t>(tcpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(tcpHdr.dstPort);
  rwCursor.writeBE<uint32_t>(tcpHdr.sequenceNumber);
  rwCursor.writeBE<uint32_t>(tcpHdr.ackNumber);
  rwCursor.writeBE<uint8_t>(tcpHdr.dataOffsetAndFlags);
  rwCursor.writeBE<uint8_t>(tcpHdr.flags);
  rwCursor.writeBE<uint16_t>(tcpHdr.windowSize);
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  rwCursor.writeBE<uint16_t>(tcpHdr.urgentPointer);
  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payload.data(), payload.size());
  uint16_t csum = tcpHdr.computeChecksum(ipHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
}

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass,
    uint8_t hopLimit,
    std::optional<std::vector<uint8_t>> payload) {
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_IPV6);
  // IPv6Hdr
  IPv6Hdr ipHdr(srcIp, dstIp);
  ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  ipHdr.trafficClass = trafficClass;
  ipHdr.payloadLength = TCPHeader::size() + payloadBytes.size();
  ipHdr.hopLimit = hopLimit;
  // TCPHeader
  TCPHeader tcpHdr(srcPort, dstPort);

  return makeTCPTxPacket(hw, ethHdr, ipHdr, tcpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t dscp,
    uint8_t ttl,
    std::optional<std::vector<uint8_t>> payload) {
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_IPV4);
  // IPv4Hdr
  IPv4Hdr ipHdr(
      srcIp,
      dstIp,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP),
      payloadBytes.size());
  ipHdr.dscp = dscp;
  ipHdr.computeChecksum();
  ipHdr.ttl = ttl;
  // TCPHeader
  TCPHeader tcpHdr(srcPort, dstPort);

  return makeTCPTxPacket(hw, ethHdr, ipHdr, tcpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass,
    uint8_t hopLimit,
    std::optional<std::vector<uint8_t>> payload) {
  CHECK_EQ(srcIp.isV6(), dstIp.isV6());
  if (srcIp.isV6()) {
    return makeTCPTxPacket(
        hw,
        vlan,
        srcMac,
        dstMac,
        srcIp.asV6(),
        dstIp.asV6(),
        srcPort,
        dstPort,
        trafficClass,
        hopLimit,
        payload);
  }
  return makeTCPTxPacket(
      hw,
      vlan,
      srcMac,
      dstMac,
      srcIp.asV4(),
      dstIp.asV4(),
      srcPort,
      dstPort,
      trafficClass,
      hopLimit,
      payload);
}

std::unique_ptr<facebook::fboss::TxPacket> makeARPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    ARP_OPER type,
    std::optional<folly::MacAddress> targetMac) {
  if (!targetMac) {
    targetMac = type == ARP_OPER::ARP_OPER_REQUEST
        ? folly::MacAddress("00:00:00:00:00:00")
        : dstMac;
  }
  ArpHdr arpHdr(
      static_cast<uint16_t>(ARP_HTYPE::ARP_HTYPE_ETHERNET),
      static_cast<uint16_t>(ARP_PTYPE::ARP_PTYPE_IPV4),
      static_cast<uint8_t>(ARP_HLEN::ARP_HLEN_ETHERNET),
      static_cast<uint8_t>(ARP_PLEN::ARP_PLEN_IPV4),
      static_cast<uint16_t>(type),
      srcMac,
      srcIp.asV4(),
      *targetMac,
      dstIp.asV4());
  auto txPacket = hw->allocatePacket(
      EthHdr::SIZE + ArpHdr::size() + kDefaultPayload.size());
  // EthHdr
  auto ethHdr = makeEthHdr(srcMac, dstMac, vlan, ETHERTYPE::ETHERTYPE_ARP);
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      VlanID(ethHdr.getVlanTags()[0].vid()),
      ethHdr.getEtherType());

  // write ARP header
  rwCursor.writeBE<uint16_t>(arpHdr.htype);
  rwCursor.writeBE<uint16_t>(arpHdr.ptype);
  rwCursor.writeBE<uint8_t>(arpHdr.hlen);
  rwCursor.writeBE<uint8_t>(arpHdr.plen);
  rwCursor.writeBE<uint16_t>(arpHdr.oper);
  rwCursor.push(arpHdr.sha.bytes(), folly::MacAddress::SIZE);
  rwCursor.write<uint32_t>(arpHdr.spa.toLong());
  rwCursor.push(arpHdr.tha.bytes(), folly::MacAddress::SIZE);
  rwCursor.write<uint32_t>(arpHdr.tpa.toLong());
  rwCursor.push(kDefaultPayload.data(), kDefaultPayload.size());
  return txPacket;
}
} // namespace facebook::fboss::utility
