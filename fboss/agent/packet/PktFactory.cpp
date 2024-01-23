// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/PktFactory.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include "common/logging/logging.h"

#include "fboss/agent/HwSwitch.h"

namespace facebook::fboss::utility {
namespace {
static auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);

uint8_t nextHeader(const IPv6Hdr& hdr) {
  return hdr.nextHeader;
}

uint8_t nextHeader(const IPv4Hdr& hdr) {
  return hdr.protocol;
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

template <typename IPHDR>
void makeIpPacket(
    std::unique_ptr<TxPacket>& txPacket,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const std::vector<uint8_t>& payload) {
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
  rwCursor.push(payload.data(), payload.size());
}

template <typename IPHDR>
std::unique_ptr<TxPacket> makeUDPTxPacket(
    const AllocatePktFn& allocatePacket,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    const std::vector<uint8_t>& payload) {
  auto txPacket = allocatePacket(
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
    const AllocatePktFn& allocatePacket,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    PTPMessageType ptpPktType) {
  int payloadSize = PTPHeader::getPayloadSize(ptpPktType);
  auto txPacket = allocatePacket(
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

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePacket,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const TCPHeader& tcpHdr,
    const std::vector<uint8_t>& payload) {
  auto txPacket = allocatePacket(
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

} // namespace

template <typename AddrT>
IPPacket<AddrT>::IPPacket(folly::io::Cursor& cursor) {
  hdr_ = HdrT(cursor);
  if (nextHeader(hdr_) == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
    // if proto is udp, encapsulate udp
    udpPayLoad_ = UDPDatagram(cursor);
  } else if (nextHeader(hdr_) == static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP)) {
    tcpPayLoad_ = TCPPacket(cursor, hdr_.payloadSize());
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

template <typename AddrT>
std::string IPPacket<AddrT>::toString() const {
  std::stringstream ss;
  ss << "IP hdr: " << hdr_
     << " UDP : " << (udpPayLoad_.has_value() ? udpPayLoad_->toString() : "")
     << " TCP: " << (tcpPayLoad_.has_value() ? tcpPayLoad_->toString() : "");
  return ss.str();
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

std::string MPLSPacket::toString() const {
  std::stringstream ss;
  ss << "MPLS hdr: " << hdr_.toString()
     << " v6 : " << (v6PayLoad_.has_value() ? v6PayLoad_->toString() : "")
     << " v4 : " << (v4PayLoad_.has_value() ? v4PayLoad_->toString() : "");
  return ss.str();
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
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_ARP):
      arpHdr_ = ArpHdr(cursor);
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_LLDP):
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS):
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_EAPOL):
      // TODO - add support for parsing respective protocol PDUs
      break;
    default:
      throw FbossError("Unhandled etherType: ", hdr_.etherType);
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
  } else if (arpHdr_) {
    arpHdr_->serialize(&rwCursor);
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
  } else if (arpHdr_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_ARP));
    arpHdr_->serialize(&cursor);
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
    VlanID vlanId,
    size_t payloadSize) {
  constexpr auto isV4 = std::is_same_v<AddrT, folly::IPAddressV4>;
  constexpr auto etherType =
      isV4 ? ETHERTYPE::ETHERTYPE_IPV4 : ETHERTYPE::ETHERTYPE_IPV6;
  auto tags = EthHdr::VlanTags_t{VlanTag(vlanId, 0x8100)};
  EthHdr ethHdr{srcMac, dstMac, {tags}, static_cast<uint16_t>(etherType)};
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
    uint16_t dPort,
    VlanID vlanId) {
  constexpr auto isV4 = std::is_same_v<AddrT, folly::IPAddressV4>;
  auto tags = EthHdr::VlanTags_t{VlanTag(vlanId, 0x8100)};
  EthHdr ethHdr{
      srcMac, dstMac, {tags}, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS)};

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

std::string utility::EthFrame::toString() const {
  std::stringstream ss;
  ss << "Eth hdr: " << hdr_.toString()
     << " arp: " << (arpHdr_.has_value() ? arpHdr_->toString() : "")
     << " mpls: " << (mplsPayLoad_.has_value() ? mplsPayLoad_->toString() : "")
     << " v6 : " << (v6PayLoad_.has_value() ? v6PayLoad_->toString() : "")
     << " v4 : " << (v4PayLoad_.has_value() ? v4PayLoad_->toString() : "");
  return ss.str();
}

EthFrame makeEthFrame(const TxPacket& txPkt, bool skipTtlDecrement) {
  folly::io::Cursor cursor{txPkt.buf()};
  utility::EthFrame frame(cursor);
  if (skipTtlDecrement) {
    return frame;
  }
  frame.decrementTTL();
  return frame;
}

EthFrame makeEthFrame(const TxPacket& txPkt, folly::MacAddress dstMac) {
  auto frame = makeEthFrame(txPkt);
  frame.setDstMac(dstMac);
  return frame;
}

EthFrame
makeEthFrame(const TxPacket& txPkt, folly::MacAddress dstMac, VlanID vlan) {
  auto frame = makeEthFrame(txPkt, dstMac);
  frame.setVlan(vlan);
  return frame;
}

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

std::unique_ptr<TxPacket> makeEthTxPacket(
    const AllocatePktFn& allocatePacket,
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
  auto txPacket = allocatePacket(EthHdr::SIZE + payloadBytes.size());

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

std::unique_ptr<facebook::fboss::TxPacket> makeARPTxPacket(
    const AllocatePktFn& allocatePacket,
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
  auto txPacket =
      allocatePacket(EthHdr::SIZE + ArpHdr::size() + kDefaultPayload.size());
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
    const AllocatePktFn& allocatePacket,
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
  ipv6.trafficClass =
      kGetNetworkControlTrafficClass(); // CS6 precedence (network control)
  ipv6.payloadLength = ICMPHdr::SIZE + bodyLength;
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 255;

  ICMPHdr icmp6(
      static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION),
      static_cast<uint8_t>(ICMPv6Code::ICMPV6_CODE_NDP_MESSAGE_CODE),
      0);

  uint32_t pktLen = icmp6.computeTotalLengthV6(bodyLength);
  auto pkt = allocatePacket(pktLen);
  folly::io::RWPrivateCursor cursor(pkt->buf());
  icmp6.serializeFullPacket(
      &cursor,
      dstMac,
      srcMac,
      vlan,
      ipv6,
      bodyLength,
      [neighborIp, ndpOptions](folly::io::RWPrivateCursor* cursor) {
        cursor->writeBE<uint32_t>(0); // reserved
        cursor->push(neighborIp.bytes(), folly::IPAddressV6::byteCount());
        ndpOptions.serialize(cursor);
      });
  return pkt;
}

std::unique_ptr<facebook::fboss::TxPacket> makeNeighborAdvertisement(
    const AllocatePktFn& allocatePacket,
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
  ipv6.trafficClass =
      kGetNetworkControlTrafficClass(); // CS6 precedence (network control)
  ipv6.payloadLength = ICMPHdr::SIZE + bodyLength;
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 255;

  ICMPHdr icmp6(
      static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT),
      static_cast<uint8_t>(ICMPv6Code::ICMPV6_CODE_NDP_MESSAGE_CODE),
      0);

  uint32_t pktLen = icmp6.computeTotalLengthV6(bodyLength);
  auto pkt = allocatePacket(pktLen);
  folly::io::RWPrivateCursor cursor(pkt->buf());

  icmp6.serializeFullPacket(
      &cursor,
      dstMac,
      srcMac,
      vlan,
      ipv6,
      bodyLength,
      [srcIp, ndpOptions, flags](folly::io::RWPrivateCursor* cursor) {
        cursor->writeBE<uint32_t>(flags);
        cursor->push(srcIp.bytes(), folly::IPAddressV6::byteCount());
        ndpOptions.serialize(cursor);
      });
  return pkt;
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    AllocatePktFn allocatePkt,
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

  auto txPacket =
      allocatePkt(EthHdr::SIZE + ipHdr.size() + payloadBytes.size());
  makeIpPacket(txPacket, ethHdr, ipHdr, payloadBytes);
  return txPacket;
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    AllocatePktFn allocatePkt,
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

  auto txPacket =
      allocatePkt(EthHdr::SIZE + ipHdr.size() + payloadBytes.size());
  makeIpPacket(txPacket, ethHdr, ipHdr, payloadBytes);
  return txPacket;
}

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    AllocatePktFn allocatePkt,
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
                            allocatePkt,
                            vlan,
                            srcMac,
                            dstMac,
                            srcIp.asV6(),
                            dstIp.asV6(),
                            trafficClass,
                            hopLimit,
                            payload)
                      : makeIpTxPacket(
                            allocatePkt,
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
    const AllocatePktFn& allocatePacket,
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

  auto txPacket = allocatePacket(
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

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const AllocatePktFn& allocator,
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

  return makeUDPTxPacket(allocator, ethHdr, ipHdr, udpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const AllocatePktFn& allocator,
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

  return makeUDPTxPacket(allocator, ethHdr, ipHdr, udpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const AllocatePktFn& allocator,
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
        allocator,
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
      allocator,
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

std::unique_ptr<facebook::fboss::TxPacket> makePTPTxPacket(
    const AllocatePktFn& allocatePacket,
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

  return makePTPUDPTxPacket(allocatePacket, ethHdr, ipHdr, udpHdr, ptpPktType);
}
std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePacket,
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

  return makeTCPTxPacket(allocatePacket, ethHdr, ipHdr, tcpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePacket,
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

  return makeTCPTxPacket(allocatePacket, ethHdr, ipHdr, tcpHdr, payloadBytes);
}

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePacket,
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
        allocatePacket,
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
      allocatePacket,
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

std::unique_ptr<TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePacket,
    std::optional<VlanID> vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t trafficClass,
    std::optional<std::vector<uint8_t>> payload) {
  folly::MacAddress srcMac;

  if (!dstMac.isUnicast()) {
    // some arbitrary mac
    srcMac = folly::MacAddress("00:00:01:02:03:04");
  } else {
    srcMac = folly::MacAddress::fromNBO(dstMac.u64NBO() + 1);
  }

  // arbit
  const auto srcIp =
      folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::10");

  return utility::makeTCPTxPacket(
      allocatePacket,
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
    VlanID vlanId,
    size_t payloadSize);

template EthFrame getEthFrame<folly::IPAddressV6>(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    folly::IPAddressV6 srcIp,
    folly::IPAddressV6 dstIp,
    uint16_t sPort,
    uint16_t dPort,
    VlanID vlanId,
    size_t payloadSize);

template EthFrame getEthFrame<folly::IPAddressV4>(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    folly::IPAddressV4 srcIp,
    folly::IPAddressV4 dstIp,
    uint16_t sPort,
    uint16_t dPort,
    VlanID vlanId);

template EthFrame getEthFrame<folly::IPAddressV6>(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    folly::IPAddressV6 srcIp,
    folly::IPAddressV6 dstIp,
    uint16_t sPort,
    uint16_t dPort,
    VlanID vlanId);

} // namespace facebook::fboss::utility
