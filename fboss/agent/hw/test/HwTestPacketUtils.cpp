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

#include "common/logging/logging.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"

#include "fboss/agent/types.h"

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::RWPrivateCursor;

namespace {
static auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);

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

uint8_t nextHeader(const IPv6Hdr& hdr) {
  return hdr.nextHeader;
}

uint8_t nextHeader(const IPv4Hdr& hdr) {
  return hdr.protocol;
}

} // namespace

namespace facebook {
namespace fboss {
namespace utility {

std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    folly::Optional<std::vector<uint8_t>> payload) {
  static auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);
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
    folly::Optional<std::vector<uint8_t>> payload) {
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
    folly::Optional<std::vector<uint8_t>> payload) {
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
  ipHdr.computeChecksum();
  ipHdr.dscp = 0x00;
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
    folly::Optional<std::vector<uint8_t>> payload) {
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

UDPDatagram::UDPDatagram(folly::io::Cursor& cursor) {
  udpHdr_.parse(&cursor, nullptr);
  auto payLoadSize = udpHdr_.length - UDPHeader::size(); // payload length
  payload_.resize(payLoadSize);
  for (auto i = 0; i < payLoadSize; i++) {
    payload_[i] = cursor.readBE<uint8_t>(); // payload bytes
  }
}

std::unique_ptr<facebook::fboss::TxPacket> UDPDatagram::getTxPacket(
    const HwSwitch* hw) const {
  auto txPacket = hw->allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  udpHdr_.write(&rwCursor);
  for (auto byte : payload_) {
    rwCursor.writeBE<uint8_t>(byte);
  }
  return txPacket;
}

void UDPDatagram::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  udpHdr_.write(&cursor);
  for (auto byte : payload_) {
    cursor.writeBE<uint8_t>(byte);
  }
}

template <typename AddrT>
IPPacket<AddrT>::IPPacket(folly::io::Cursor& cursor) {
  hdr_ = HdrT(cursor);
  if (nextHeader(hdr_) == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
    // if proto is udp, encapsulate udp
    udpPayLoad_.assign(UDPDatagram(cursor));
  }
}

template <typename AddrT>
std::unique_ptr<facebook::fboss::TxPacket> IPPacket<AddrT>::getTxPacket(
    const HwSwitch* hw) const {
  auto txPacket = hw->allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  hdr_.serialize(&rwCursor);
  if (udpPayLoad_) {
    folly::io::Cursor cursor(udpPayLoad_->getTxPacket(hw)->buf());
    rwCursor.push(cursor, udpPayLoad_->length());
  }
  return txPacket;
}

template <typename AddrT>
void IPPacket<AddrT>::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  hdr_.serialize(&cursor);
  if (udpPayLoad_) {
    udpPayLoad_->serialize(cursor);
  }
}

MPLSPacket::MPLSPacket(folly::io::Cursor& cursor) {
  hdr_ = MPLSHdr(&cursor);
  uint8_t ipver = 0;
  if (cursor.tryReadBE<uint8_t>(ipver)) {
    cursor.retreat(1);
    ipver >>= 4; // ip version is in first four bits
    if (ipver == 4) {
      v4PayLoad_.assign(IPPacket<folly::IPAddressV4>(cursor));
    } else if (ipver == 6) {
      v6PayLoad_.assign(IPPacket<folly::IPAddressV6>(cursor));
    }
  }
}

std::unique_ptr<facebook::fboss::TxPacket> MPLSPacket::getTxPacket(
    const HwSwitch* hw) const {
  auto txPacket = hw->allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());

  hdr_.serialize(&rwCursor);
  if (v4PayLoad_) {
    auto v4Packet = v4PayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(v4Packet->buf());
    rwCursor.push(cursor, v4PayLoad_->length());
  } else if (v6PayLoad_) {
    auto v6Packet = v4PayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(v6Packet->buf());
    rwCursor.push(cursor, v4PayLoad_->length());
  }
  return txPacket;
}

void MPLSPacket::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  hdr_.serialize(&cursor);
  if (v4PayLoad_) {
    v4PayLoad_->serialize(cursor);
  } else if (v6PayLoad_) {
    v6PayLoad_->serialize(cursor);
  }
}

EthFrame::EthFrame(folly::io::Cursor& cursor) {
  hdr_ = EthHdr(cursor);
  switch (hdr_.etherType) {
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4):
      v4PayLoad_.assign(IPPacket<folly::IPAddressV4>(cursor));
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6):
      v6PayLoad_.assign(IPPacket<folly::IPAddressV6>(cursor));
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS):
      mplsPayLoad_.assign(MPLSPacket(cursor));
      break;
  }
}

std::unique_ptr<facebook::fboss::TxPacket> EthFrame::getTxPacket(
    const HwSwitch* hw) const {
  auto txPacket = hw->allocatePacket(length());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());

  rwCursor.push(hdr_.dstAddr.bytes(), folly::MacAddress::SIZE);
  rwCursor.push(hdr_.srcAddr.bytes(), folly::MacAddress::SIZE);
  if (!hdr_.vlanTags.empty()) {
    for (const auto& vlanTag : hdr_.vlanTags) {
      rwCursor.template writeBE<uint32_t>(vlanTag.value);
    }
  }
  if (v4PayLoad_) {
    rwCursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
    auto v4Packet = v4PayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(v4Packet->buf());
    rwCursor.push(cursor, v4PayLoad_->length());
  } else if (v6PayLoad_) {
    rwCursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
    auto v6Packet = v6PayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(v6Packet->buf());
    rwCursor.push(cursor, v6PayLoad_->length());
  } else if (mplsPayLoad_) {
    rwCursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS));
    auto mplsPacket = mplsPayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(mplsPacket->buf());
    rwCursor.push(cursor, mplsPayLoad_->length());
  }
  return txPacket;
}

void EthFrame::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  cursor.push(hdr_.dstAddr.bytes(), folly::MacAddress::SIZE);
  cursor.push(hdr_.srcAddr.bytes(), folly::MacAddress::SIZE);

  if (!hdr_.vlanTags.empty()) {
    for (const auto& vlanTag : hdr_.vlanTags) {
      cursor.template writeBE<uint32_t>(vlanTag.value);
    }
  }

  if (v4PayLoad_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
    v4PayLoad_->serialize(cursor);
  } else if (v6PayLoad_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
    v6PayLoad_->serialize(cursor);
  } else if (mplsPayLoad_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS));
    mplsPayLoad_->serialize(cursor);
  }
}

template class IPPacket<folly::IPAddressV4>;
template class IPPacket<folly::IPAddressV6>;
} // namespace utility
} // namespace fboss
} // namespace facebook
