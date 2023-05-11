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
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/TCPHeader.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include "fboss/agent/test/ResourceLibUtil.h"

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::RWPrivateCursor;

namespace {
auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);

EthHdr makeEthHdr(
    MacAddress srcMac,
    MacAddress dstMac,
    std::optional<VlanID> vlan,
    ETHERTYPE etherType) {
  EthHdr::VlanTags_t vlanTags;

  if (vlan.has_value()) {
    vlanTags.push_back(VlanTag(
        vlan.value(), static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN)));
  }

  EthHdr ethHdr(dstMac, srcMac, vlanTags, static_cast<uint16_t>(etherType));
  return ethHdr;
}

template <typename CursorType>
void writeEthHeader(
    const std::unique_ptr<TxPacket>& txPacket,
    CursorType* cursor,
    folly::MacAddress dst,
    folly::MacAddress src,
    std::vector<VlanTag> vlanTags,
    uint16_t protocol) {
  if (vlanTags.size() != 0) {
    txPacket->writeEthHeader(
        cursor, dst, src, VlanID(vlanTags[0].vid()), protocol);
  } else {
    txPacket->writeEthHeader(cursor, dst, src, protocol);
  }
}

} // namespace

namespace facebook::fboss::utility {

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan) {
  return state->getMultiSwitchInterfaces()->getInterfaceInVlan(vlan)->getMac();
}
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf) {
  return state->getMultiSwitchInterfaces()->getNode(intf)->getMac();
}

folly::MacAddress getFirstInterfaceMac(const cfg::SwitchConfig& cfg) {
  auto intfCfg = cfg.interfaces()[0];

  if (!intfCfg.mac().has_value()) {
    throw FbossError(
        "No MAC address set for interface ", InterfaceID(*intfCfg.intfID()));
  }

  return folly::MacAddress(*intfCfg.mac());
}

folly::MacAddress getFirstInterfaceMac(std::shared_ptr<SwitchState> state) {
  const auto& intfMap = state->getMultiSwitchInterfaces()->cbegin()->second;
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
  if (state->getVlans()->size()) {
    firstVlanId = (*state->getVlans()->cbegin()).second->getID();
  }
  return firstVlanId;
}

std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    const HwSwitch* hw,
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
  writeEthHeader(
      txPacket,
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      ethHdr.getVlanTags(),
      ethHdr.getEtherType());
  ipHdr.serialize(&rwCursor);

  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payload.data(), payload.size());
  return txPacket;
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
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
    std::optional<VlanID> vlan,
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
  ipHdr.ttl = ttl;
  ipHdr.computeChecksum();

  return makeIpTxPacket(hw, ethHdr, ipHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
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

std::unique_ptr<facebook::fboss::TxPacket> makeIpInIpTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress outerSrcMac,
    folly::MacAddress outerDstMac,
    const folly::IPAddressV6& outerSrcIp,
    const folly::IPAddressV6& outerDstIp,
    const folly::IPAddressV6& innerSrcIp,
    const folly::IPAddressV6& innerDstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t outerTrafficClass,
    uint8_t innerTrafficClass,
    uint8_t hopLimit,
    std::optional<std::vector<uint8_t>> payload) {
  if (!payload) {
    payload = kDefaultPayload;
  }
  const auto& payloadBytes = payload.value();
  auto ethHdr =
      makeEthHdr(outerSrcMac, outerDstMac, vlan, ETHERTYPE::ETHERTYPE_IPV6);
  // IPv6Hdr -- outer
  IPv6Hdr outerIpHdr(outerSrcIp, outerDstIp);
  outerIpHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6);
  outerIpHdr.trafficClass = outerTrafficClass;
  outerIpHdr.payloadLength =
      IPv6Hdr::size() + UDPHeader::size() + payloadBytes.size();
  outerIpHdr.hopLimit = hopLimit;
  // IPv6Hdr -- inner
  IPv6Hdr innerIpHdr(innerSrcIp, innerDstIp);
  innerIpHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  innerIpHdr.trafficClass = innerTrafficClass;
  innerIpHdr.payloadLength = UDPHeader::size() + payloadBytes.size();
  innerIpHdr.hopLimit = hopLimit;

  auto txPacket = hw->allocatePacket(
      EthHdr::SIZE + 2 * outerIpHdr.size() + UDPHeader::size() +
      payloadBytes.size());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      vlan,
      ethHdr.getEtherType());
  outerIpHdr.serialize(&rwCursor);
  innerIpHdr.serialize(&rwCursor);

  rwCursor.writeBE<uint16_t>(srcPort);
  rwCursor.writeBE<uint16_t>(dstPort);
  rwCursor.writeBE<uint16_t>(UDPHeader::size() + payloadBytes.size());
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payloadBytes.data(), payloadBytes.size());
  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());
  uint16_t csum = udpHdr.computeChecksum(innerIpHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
}

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    const std::vector<uint8_t>& payload) {
  auto txPacket = hw->allocatePacket(
      ethHdr.size() + ipHdr.size() + udpHdr.size() + payload.size());

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
  rwCursor.push(payload.data(), payload.size());
  uint16_t csum = udpHdr.computeChecksum(ipHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
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

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
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
    std::optional<VlanID> vlan,
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
  // IPv4Hdr - total_length field includes the payload + UDP hdr + ip hdr
  IPv4Hdr ipHdr(
      srcIp,
      dstIp,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      payloadBytes.size() + UDPHeader::size());
  ipHdr.dscp = dscp;
  ipHdr.ttl = ttl;
  ipHdr.computeChecksum();
  // UDPHeader
  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());

  return makeUDPTxPacket(hw, ethHdr, ipHdr, udpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
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
      ethHdr.size() + ipHdr.size() + tcpHdr.size() + payload.size());

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

  // write TCP header, payload and compute checksum
  rwCursor.writeBE<uint16_t>(tcpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(tcpHdr.dstPort);
  rwCursor.writeBE<uint32_t>(tcpHdr.sequenceNumber);
  rwCursor.writeBE<uint32_t>(tcpHdr.ackNumber);
  rwCursor.writeBE<uint8_t>(tcpHdr.dataOffsetAndReserved);
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
    std::optional<VlanID> vlan,
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
    std::optional<VlanID> vlan,
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
  ipHdr.ttl = ttl;
  ipHdr.computeChecksum();
  // TCPHeader
  TCPHeader tcpHdr(srcPort, dstPort);

  return makeTCPTxPacket(hw, ethHdr, ipHdr, tcpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
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
    std::optional<VlanID> vlan,
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
  writeEthHeader(
      txPacket,
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      ethHdr.getVlanTags(),
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

std::unique_ptr<facebook::fboss::TxPacket> makeNeighborSolicitation(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& neighborIp) {
  folly::IPAddressV6 solicitedNodeAddr = neighborIp.getSolicitedNodeAddress();
  MacAddress dstMac = MacAddress::createMulticast(solicitedNodeAddr);

  NDPOptions ndpOptions;
  ndpOptions.sourceLinkLayerAddress.emplace(srcMac);

  uint32_t bodyLength = ICMPHdr::ICMPV6_UNUSED_LEN +
      folly::IPAddressV6::byteCount() + ndpOptions.computeTotalLength();

  IPv6Hdr ipv6(srcIp, solicitedNodeAddr);
  ipv6.trafficClass = 0xe0; // CS7 precedence (network control)
  ipv6.payloadLength = ICMPHdr::SIZE + bodyLength;
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 255;

  ICMPHdr icmp6(
      static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION),
      static_cast<uint8_t>(ICMPv6Code::ICMPV6_CODE_NDP_MESSAGE_CODE),
      0);

  uint32_t pktLen = icmp6.computeTotalLengthV6(bodyLength);
  auto pkt = hw->allocatePacket(pktLen);
  RWPrivateCursor cursor(pkt->buf());
  icmp6.serializeFullPacket(
      &cursor,
      dstMac,
      srcMac,
      vlan,
      ipv6,
      bodyLength,
      [neighborIp, ndpOptions](RWPrivateCursor* cursor) {
        cursor->writeBE<uint32_t>(0); // reserved
        cursor->push(neighborIp.bytes(), folly::IPAddressV6::byteCount());
        ndpOptions.serialize(cursor);
      });
  return pkt;
}

std::unique_ptr<facebook::fboss::TxPacket> makeNeighborAdvertisement(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    folly::IPAddressV6 dstIp) {
  uint32_t flags =
      NeighborAdvertisementFlags::ROUTER | NeighborAdvertisementFlags::OVERRIDE;
  if (dstIp.isZero()) {
    dstIp = folly::IPAddressV6("ff01::1");
  } else {
    flags |= NeighborAdvertisementFlags::SOLICITED;
  }
  NDPOptions ndpOptions;
  ndpOptions.targetLinkLayerAddress.emplace(srcMac);

  uint32_t bodyLength = ICMPHdr::ICMPV6_UNUSED_LEN +
      folly::IPAddressV6::byteCount() + ndpOptions.computeTotalLength();

  IPv6Hdr ipv6(srcIp, dstIp);
  ipv6.trafficClass = 0xe0; // CS7 precedence (network control)
  ipv6.payloadLength = ICMPHdr::SIZE + bodyLength;
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 255;

  ICMPHdr icmp6(
      static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT),
      static_cast<uint8_t>(ICMPv6Code::ICMPV6_CODE_NDP_MESSAGE_CODE),
      0);

  uint32_t pktLen = icmp6.computeTotalLengthV6(bodyLength);
  auto pkt = hw->allocatePacket(pktLen);
  RWPrivateCursor cursor(pkt->buf());

  icmp6.serializeFullPacket(
      &cursor,
      dstMac,
      srcMac,
      vlan,
      ipv6,
      bodyLength,
      [srcIp, ndpOptions, flags](RWPrivateCursor* cursor) {
        cursor->writeBE<uint32_t>(flags);
        cursor->push(srcIp.bytes(), folly::IPAddressV6::byteCount());
        ndpOptions.serialize(cursor);
      });
  return pkt;
}

std::unique_ptr<facebook::fboss::TxPacket> makeLLDPPacket(
    const HwSwitch* hw,
    const folly::MacAddress srcMac,
    std::optional<VlanID> vlanid,
    const std::string& systemdescr,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities) {
  uint32_t frameLen =
      LldpManager::LldpPktSize(hostname, portname, portdesc, systemdescr);

  VlanID vlan(vlanid.value_or(VlanID(0)));

  auto pkt = hw->allocatePacket(frameLen);
  LldpManager::fillLldpTlv(
      pkt.get(),
      srcMac,
      vlan,
      systemdescr,
      hostname,
      portname,
      portdesc,
      ttl,
      capabilities);
  return pkt;
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
  folly::MacAddress srcMac;

  if (!dstMac.isUnicast()) {
    // some arbitrary mac
    srcMac = folly::MacAddress("00:00:01:02:03:04");
  } else {
    srcMac = utility::MacAddressGenerator().get(dstMac.u64NBO() + 1);
  }

  // arbit
  const auto srcIp =
      folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::10");
  for (int i = 0; i < numPktsToSend; i++) {
    auto txPacket = utility::makeTCPTxPacket(
        hwSwitch,
        vlanId,
        srcMac,
        dstMac,
        srcIp,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        dstIpAddress.isV4()
            ? trafficClass
            : trafficClass << 2, // v6 header takes entire TC byte with
                                 // trailing 2 bits for ECN. V4 header OTOH
                                 // expects only dscp value.
        255,
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
