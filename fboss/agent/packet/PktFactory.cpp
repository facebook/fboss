// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/PktFactory.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include "common/logging/logging.h"

#include "fboss/agent/HwSwitch.h"

namespace {
static auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);

using namespace facebook::fboss;
uint8_t nextHeader(const IPv6Hdr& hdr) {
  return hdr.nextHeader;
}

uint8_t nextHeader(const IPv4Hdr& hdr) {
  return hdr.protocol;
}
} // namespace

namespace facebook::fboss {

namespace utility {
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
    udpPayLoad_ = UDPDatagram(cursor);
  }
}

template <typename AddrT>
void IPPacket<AddrT>::setUDPCheckSum(folly::IOBuf* buffer) const {
  CHECK(udpPayLoad_.has_value());
  folly::io::Cursor start(buffer);
  // jump to  payloag start.
  // skip ipv4 header and udp header to get to the start of payload
  start += (hdr_.size() + udpPayLoad_->header().size());
  // compute checksum
  auto udpHdr = udpPayLoad_->header();
  auto csum = udpHdr.computeChecksum(hdr_, start);
  folly::io::RWPrivateCursor rwCursor(buffer);
  rwCursor +=
      (hdr_.size() + sizeof(udpHdr.srcPort) + sizeof(udpHdr.dstPort) +
       sizeof(udpHdr.length));
  rwCursor.writeBE<uint16_t>(csum);
}

template <typename AddrT>
std::unique_ptr<facebook::fboss::TxPacket> IPPacket<AddrT>::getTxPacket(
    const HwSwitch* hw) const {
  auto txPacket = hw->allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  hdr_.serialize(&rwCursor);
  if (udpPayLoad_) {
    auto udpPkt = udpPayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(udpPkt->buf());
    rwCursor.push(cursor, udpPayLoad_->length());
    setUDPCheckSum(txPacket->buf());
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
      v4PayLoad_ = IPPacket<folly::IPAddressV4>(cursor);
    } else if (ipver == 6) {
      v6PayLoad_ = IPPacket<folly::IPAddressV6>(cursor);
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
    auto v6Packet = v6PayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(v6Packet->buf());
    rwCursor.push(cursor, v6PayLoad_->length());
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
      v4PayLoad_ = IPPacket<folly::IPAddressV4>(cursor);
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6):
      v6PayLoad_ = IPPacket<folly::IPAddressV6>(cursor);
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS):
      mplsPayLoad_ = MPLSPacket(cursor);
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

template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort,
    uint16_t vlanId,
    size_t payloadSize) {
  constexpr auto isV4 = std::is_same_v<AddrT, folly::IPAddressV4>;
  constexpr auto etherType =
      isV4 ? ETHERTYPE::ETHERTYPE_IPV4 : ETHERTYPE::ETHERTYPE_IPV6;
  EthHdr ethHdr{srcMac,
                dstMac,
                {EthHdr::VlanTags_t{VlanTag(vlanId, 0x8100)}},
                static_cast<uint16_t>(etherType)};
  std::conditional_t<isV4, IPv4Hdr, IPv6Hdr> ipHdr;
  ipHdr.srcAddr = srcIp;
  ipHdr.dstAddr = dstIp;

  UDPHeader udpHdr;
  udpHdr.srcPort = sPort;
  udpHdr.dstPort = dPort;

  if (payloadSize == 256) {
    return utility::EthFrame(
        ethHdr,
        utility::IPPacket<AddrT>(
            ipHdr, utility::UDPDatagram(udpHdr, kDefaultPayload)));
  }
  return utility::EthFrame(
      ethHdr,
      utility::IPPacket<AddrT>(
          ipHdr,
          utility::UDPDatagram(
              udpHdr, std::vector<uint8_t>(payloadSize, 0xff))));
}

template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort) {
  constexpr auto isV4 = std::is_same_v<AddrT, folly::IPAddressV4>;
  EthHdr ethHdr{srcMac,
                dstMac,
                {EthHdr::VlanTags_t{VlanTag(1, 0x8100)}},
                static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS)};

  MPLSHdr mplsHdr{std::move(labels)};

  std::conditional_t<isV4, IPv4Hdr, IPv6Hdr> ipHdr;
  ipHdr.srcAddr = srcIp;
  ipHdr.dstAddr = dstIp;

  UDPHeader udpHdr;
  udpHdr.srcPort = sPort;
  udpHdr.dstPort = dPort;

  return utility::EthFrame(
      ethHdr,
      utility::MPLSPacket(
          mplsHdr,
          utility::IPPacket<AddrT>(
              ipHdr, utility::UDPDatagram(udpHdr, kDefaultPayload))));
}

template class IPPacket<folly::IPAddressV4>;
template class IPPacket<folly::IPAddressV6>;

template EthFrame getEthFrame<folly::IPAddressV4>(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    folly::IPAddressV4 srcIp,
    folly::IPAddressV4 dstIp,
    uint16_t sPort,
    uint16_t dPort,
    uint16_t vlanId,
    size_t payloadSize);

template EthFrame getEthFrame<folly::IPAddressV6>(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    folly::IPAddressV6 srcIp,
    folly::IPAddressV6 dstIp,
    uint16_t sPort,
    uint16_t dPort,
    uint16_t vlanId,
    size_t payloadSize);

template EthFrame getEthFrame<folly::IPAddressV4>(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    folly::IPAddressV4 srcIp,
    folly::IPAddressV4 dstIp,
    uint16_t sPort,
    uint16_t dPort);

template EthFrame getEthFrame<folly::IPAddressV6>(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    folly::IPAddressV6 srcIp,
    folly::IPAddressV6 dstIp,
    uint16_t sPort,
    uint16_t dPort);

} // namespace utility

} // namespace facebook::fboss
