// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTajoImpl.h"

#include <gtest/gtest.h>

#include <folly/Conv.h>
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/TCPHeader.h"
#include "fboss/agent/packet/TajoPuntHeader.h"
#include "fboss/agent/packet/UDPHeader.h"

namespace facebook::fboss {

namespace {

constexpr uint16_t kTajoDropReasonDefaultRoute = 112;
constexpr uint16_t kTajoDropReasonAcl = 107;
constexpr uint16_t kTajoDropReasonMmu = 0x12;
constexpr uint16_t kTajoDropReasonSrv6MidpointNonLastSid = 52;
constexpr uint16_t kTajoDropReasonSrv6DecapNonLastSegment = 112;
constexpr uint16_t kTajoDropReasonSrv6BindingSidNonLastSid = 161;
constexpr uint16_t kTajoDropReasonSrv6MidpointUnresolved = 161;
constexpr int16_t kTajoModTruncateSize = 128;

struct TajoMirrorOnDropPacketParsed {
  EthHdr ethHeader;
  IPv6Hdr ipv6Header;
  UDPHeader udpHeader;
  ParsedTajoPuntHeader puntHeader{};
  std::optional<EthHdr> innerEth;
  std::optional<IPv4Hdr> innerIpv4;
  std::optional<IPv6Hdr> innerIpv6;
  std::optional<UDPHeader> innerUdp;
  std::optional<TCPHeader> innerTcp;
  size_t innerBytesAfterPunt{0};
};

folly::IPAddressV6 toV6(const folly::IPAddressV6& ip) {
  return ip;
}

folly::IPAddressV6 toV6(const folly::IPAddressV4& ip) {
  return folly::IPAddressV6(folly::to<std::string>("::ffff:", ip.str()));
}

void parseInnerIpv6L4(
    folly::io::Cursor& cursor,
    const IPv6Hdr& innerV6,
    TajoMirrorOnDropPacketParsed& parsed) {
  const auto nextHeader = innerV6.nextHeader;
  if (nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
    UDPHeader udp;
    udp.parse(&cursor);
    parsed.innerUdp = udp;
  } else if (nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP)) {
    TCPHeader tcp;
    tcp.parse(&cursor);
    parsed.innerTcp = tcp;
  }
}

void parseInnerIpv4L4(
    folly::io::Cursor& cursor,
    const IPv4Hdr& innerV4,
    TajoMirrorOnDropPacketParsed& parsed) {
  const auto proto = innerV4.protocol;
  if (proto == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
    UDPHeader udp;
    udp.parse(&cursor);
    parsed.innerUdp = udp;
  } else if (proto == static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP)) {
    TCPHeader tcp;
    tcp.parse(&cursor);
    parsed.innerTcp = tcp;
  }
}

bool isInnerL3EtherType(uint16_t etherType) {
  return etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4) ||
      etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6);
}

// True when bytes 12/13 (after DA+SA) look like a real Ethernet header.
bool looksLikeInnerEthernetHdr(folly::io::Cursor cursor) {
  if (!cursor.canAdvance(EthHdr::UNTAGGED_PKT_SIZE)) {
    return false;
  }
  folly::io::Cursor peek = cursor;
  peek.skip(12);
  uint16_t etherType = static_cast<uint16_t>(peek.read<uint8_t>()) << 8;
  etherType |= static_cast<uint16_t>(peek.read<uint8_t>());
  if (isInnerL3EtherType(etherType)) {
    return true;
  }
  if (etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN) ||
      etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_QINQ)) {
    if (!peek.canAdvance(4)) {
      return false;
    }
    peek.skip(2);
    etherType = static_cast<uint16_t>(peek.read<uint8_t>()) << 8;
    etherType |= static_cast<uint16_t>(peek.read<uint8_t>());
    return isInnerL3EtherType(etherType);
  }
  return false;
}

// True when the sample starts with an IPv4/IPv6 header (no leading Eth).
bool looksLikeInnerL3Hdr(folly::io::Cursor cursor) {
  if (!cursor.canAdvance(1)) {
    return false;
  }
  const uint8_t first = cursor.read<uint8_t>();
  cursor.retreat(1);
  const auto version = first >> 4;
  return version == 4 || version == 6;
}

TajoPuntNextHeaderKind effectiveInnerLayout(
    TajoPuntNextHeaderKind puntNextHeader,
    folly::io::Cursor cursor) {
  if (tajoPuntInnerStartsAtL3(puntNextHeader)) {
    return puntNextHeader;
  }
  if (tajoPuntInnerHasEthernetHdr(puntNextHeader)) {
    if (looksLikeInnerEthernetHdr(cursor)) {
      return TajoPuntNextHeaderKind::Ethernet;
    }
    if (looksLikeInnerL3Hdr(cursor)) {
      const uint8_t first = cursor.read<uint8_t>();
      cursor.retreat(1);
      return (first >> 4) == 6 ? TajoPuntNextHeaderKind::Ipv6
                               : TajoPuntNextHeaderKind::Ipv4;
    }
    return TajoPuntNextHeaderKind::Ethernet;
  }
  if (looksLikeInnerEthernetHdr(cursor)) {
    return TajoPuntNextHeaderKind::Ethernet;
  }
  if (looksLikeInnerL3Hdr(cursor)) {
    const uint8_t first = cursor.read<uint8_t>();
    cursor.retreat(1);
    return (first >> 4) == 6 ? TajoPuntNextHeaderKind::Ipv6
                             : TajoPuntNextHeaderKind::Ipv4;
  }
  return puntNextHeader;
}

void parseInnerSampledPacket(
    folly::io::Cursor& cursor,
    TajoPuntNextHeaderKind nextHeaderKind,
    size_t innerBytesOnWire,
    TajoMirrorOnDropPacketParsed& parsed) {
  const auto innerLayout = effectiveInnerLayout(nextHeaderKind, cursor);
  if (tajoPuntInnerHasEthernetHdr(innerLayout)) {
    auto innerEth = EthHdr(cursor);
    parsed.innerEth = innerEth;
    if (innerEth.getEtherType() ==
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      auto innerV6 = IPv6Hdr(cursor);
      parsed.innerIpv6 = innerV6;
      parseInnerIpv6L4(cursor, innerV6, parsed);
    } else if (
        innerEth.getEtherType() ==
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
      auto innerV4 = IPv4Hdr(cursor);
      parsed.innerIpv4 = innerV4;
      parseInnerIpv4L4(cursor, innerV4, parsed);
    }
  } else if (innerLayout == TajoPuntNextHeaderKind::Ipv6) {
    auto innerV6 = IPv6Hdr(cursor);
    parsed.innerIpv6 = innerV6;
    parseInnerIpv6L4(cursor, innerV6, parsed);
  } else if (innerLayout == TajoPuntNextHeaderKind::Ipv4) {
    auto innerV4 = IPv4Hdr(cursor);
    parsed.innerIpv4 = innerV4;
    parseInnerIpv4L4(cursor, innerV4, parsed);
  } else {
    auto innerEth = EthHdr(cursor);
    parsed.innerEth = innerEth;
    if (innerEth.getEtherType() ==
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      auto innerV6 = IPv6Hdr(cursor);
      parsed.innerIpv6 = innerV6;
      parseInnerIpv6L4(cursor, innerV6, parsed);
    } else if (
        innerEth.getEtherType() ==
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
      auto innerV4 = IPv4Hdr(cursor);
      parsed.innerIpv4 = innerV4;
      parseInnerIpv4L4(cursor, innerV4, parsed);
    }
  }
  parsed.innerBytesAfterPunt = innerBytesOnWire;
}

TajoMirrorOnDropPacketParsed deserializeTajoMirrorOnDropPacket(
    const folly::IOBuf* buf) {
  TajoMirrorOnDropPacketParsed parsed;
  const size_t frameLen = buf->computeChainDataLength();
  folly::io::Cursor cursor(buf);
  parsed.ethHeader = EthHdr(cursor);
  parsed.ipv6Header = IPv6Hdr(cursor);
  parsed.udpHeader.parse(&cursor);
  parsed.puntHeader = parseTajoPuntHeader(cursor);
  const size_t innerBytesOnWire = tajoModInnerBytesOnWire(
      frameLen, parsed.ethHeader.size(), parsed.puntHeader.consumedBytes);
  parseInnerSampledPacket(
      cursor, parsed.puntHeader.nextHeaderKind, innerBytesOnWire, parsed);
  return parsed;
}

} // namespace

cfg::MirrorOnDropReport TajoMirrorOnDropImpl::makeReport(
    const std::string& name,
    const folly::IPAddressV6& collectorIp,
    int16_t collectorPort,
    int16_t srcPort,
    const folly::IPAddressV6& switchIp,
    std::optional<int32_t> samplingRate) const {
  cfg::MirrorOnDropReport report;
  report.name() = name;
  report.localSrcPort() = srcPort;
  report.collectorIp() = collectorIp.str();
  report.collectorPort() = collectorPort;
  report.mtu() = 1500;
  report.dscp() = 0;
  if (samplingRate.has_value()) {
    report.samplingRate() = samplingRate.value();
  }
  report.truncateSize() = kTajoModTruncateSize;
  report.mirrorPort().ensure().tunnel().ensure().srcIp() = switchIp.str();
  return report;
}

MirrorOnDropPacketFields TajoMirrorOnDropImpl::parsePacket(
    const folly::IOBuf* buf) const {
  auto parsed = deserializeTajoMirrorOnDropPacket(buf);
  folly::IPAddressV6 innerSrcIp("::");
  folly::IPAddressV6 innerDstIp("::");
  if (parsed.innerIpv6.has_value()) {
    innerSrcIp = toV6(parsed.innerIpv6->srcAddr);
    innerDstIp = toV6(parsed.innerIpv6->dstAddr);
  } else if (parsed.innerIpv4.has_value()) {
    innerSrcIp = toV6(parsed.innerIpv4->srcAddr);
    innerDstIp = toV6(parsed.innerIpv4->dstAddr);
  }

  uint16_t innerSrcPort = 0;
  uint16_t innerDstPort = 0;
  if (parsed.innerUdp.has_value()) {
    innerSrcPort = parsed.innerUdp->srcPort;
    innerDstPort = parsed.innerUdp->dstPort;
  } else if (parsed.innerTcp.has_value()) {
    innerSrcPort = parsed.innerTcp->srcPort;
    innerDstPort = parsed.innerTcp->dstPort;
  }

  return {
      .outerSrcMac = parsed.ethHeader.getSrcMac(),
      .outerDstMac = parsed.ethHeader.getDstMac(),
      .outerSrcIp = parsed.ipv6Header.srcAddr,
      .outerDstIp = parsed.ipv6Header.dstAddr,
      .outerSrcPort = parsed.udpHeader.srcPort,
      .outerDstPort = parsed.udpHeader.dstPort,
      .ingressPort = parsed.puntHeader.sourceSp != 0
          ? parsed.puntHeader.sourceSp
          : parsed.puntHeader.ingressPort,
      .dropReasonIngress = static_cast<uint16_t>(
          parsed.puntHeader.dropReason == kTajoDropReasonMmu
              ? 0
              : parsed.puntHeader.dropReason),
      .dropReasonEgress = static_cast<uint16_t>(
          parsed.puntHeader.dropReason == kTajoDropReasonMmu
              ? parsed.puntHeader.dropReason
              : 0),
      .innerSrcMac = parsed.innerEth.has_value() ? parsed.innerEth->getSrcMac()
                                                 : folly::MacAddress(),
      .innerDstMac = parsed.innerEth.has_value() ? parsed.innerEth->getDstMac()
                                                 : folly::MacAddress(),
      .innerSrcIp = innerSrcIp,
      .innerDstIp = innerDstIp,
      .innerSrcPort = innerSrcPort,
      .innerDstPort = innerDstPort,
      .asicMetadata =
          {
              {"puntCode", parsed.puntHeader.code},
              {"sourceSp", parsed.puntHeader.sourceSp},
          },
  };
}

void TajoMirrorOnDropImpl::verifyInvariants(const folly::IOBuf* buf) const {
  auto parsed = deserializeTajoMirrorOnDropPacket(buf);
  EXPECT_EQ(
      parsed.ipv6Header.nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  EXPECT_EQ(parsed.udpHeader.csum, 0);

  EXPECT_EQ(parsed.puntHeader.consumedBytes, kTajoNplPuntHeaderBytes);
  EXPECT_NE(parsed.puntHeader.nextHeaderKind, TajoPuntNextHeaderKind::Unknown);
  EXPECT_NE(parsed.puntHeader.nextHeader, 0);
  EXPECT_EQ(parsed.puntHeader.dropReason, parsed.puntHeader.code);

  const size_t frameLen = buf->computeChainDataLength();
  const size_t postOuterIpv6WireLen =
      frameLen - parsed.ethHeader.size() - kTajoModOuterIpv6Len;
  EXPECT_EQ(
      static_cast<size_t>(parsed.ipv6Header.payloadLength),
      postOuterIpv6WireLen)
      << "outer IPv6 payloadLength must match bytes after outer IPv6 header";

  EXPECT_EQ(
      postOuterIpv6WireLen,
      kTajoModOuterUdpLen + parsed.puntHeader.consumedBytes +
          parsed.innerBytesAfterPunt)
      << "outer IPv6 payload = UDP(8) + punt("
      << parsed.puntHeader.consumedBytes << ") + inner sample("
      << parsed.innerBytesAfterPunt << ")";

  folly::io::Cursor innerLayoutCursor(buf);
  innerLayoutCursor.skip(
      parsed.ethHeader.size() + kTajoModOuterIpv6Len + kTajoModOuterUdpLen +
      parsed.puntHeader.consumedBytes);
  const auto innerLayout =
      effectiveInnerLayout(parsed.puntHeader.nextHeaderKind, innerLayoutCursor);
  if (tajoPuntInnerHasEthernetHdr(innerLayout)) {
    ASSERT_TRUE(parsed.innerEth.has_value());
    EXPECT_GE(parsed.innerBytesAfterPunt, kTajoModInnerEthLen);
  } else if (tajoPuntInnerStartsAtL3(innerLayout)) {
    EXPECT_FALSE(parsed.innerEth.has_value());
    ASSERT_TRUE(parsed.innerIpv6.has_value() || parsed.innerIpv4.has_value());
    EXPECT_GE(parsed.innerBytesAfterPunt, kTajoModOuterIpv6Len);
  }
}

uint16_t TajoMirrorOnDropImpl::expectedIngressPort(
    const HwSwitch* hw,
    const PortID& portId) const {
  const auto* saiSwitch = dynamic_cast<const SaiSwitch*>(hw);
  if (!saiSwitch) {
    return static_cast<uint16_t>(portId);
  }
  if (const auto* handle =
          saiSwitch->managerTable()->portManager().getPortHandle(portId);
      handle && handle->port) {
    try {
      return static_cast<uint16_t>(
          SaiApiTable::getInstance()->portApi().getAttribute(
              handle->port->adapterKey(),
              SaiPortTraits::Attributes::SystemPortId{}));
    } catch (const SaiApiError&) {
    }
  }
  if (auto* platform = saiSwitch->getPlatform()) {
    return static_cast<uint16_t>(portId) +
        static_cast<uint16_t>(platform->getAsic()->getSflowPortIDOffset());
  }
  return static_cast<uint16_t>(portId);
}

MirrorOnDropDropReasonCodes TajoMirrorOnDropImpl::packMmuDropReason(
    uint16_t /*code*/) const {
  return {.ingressDropReason = 0, .egressDropReason = 0x12};
}

uint16_t TajoMirrorOnDropImpl::getDefaultRouteDropReason() const {
  return kTajoDropReasonDefaultRoute;
}

uint16_t TajoMirrorOnDropImpl::getAclDropReason() const {
  return kTajoDropReasonAcl;
}

uint16_t TajoMirrorOnDropImpl::getMmuDropReason() const {
  return kTajoDropReasonMmu;
}

uint16_t TajoMirrorOnDropImpl::getSrv6MidpointNonLastSidDropReason() const {
  return kTajoDropReasonSrv6MidpointNonLastSid;
}

uint16_t TajoMirrorOnDropImpl::getSrv6DecapNonLastSegmentDropReason() const {
  return kTajoDropReasonSrv6DecapNonLastSegment;
}

uint16_t TajoMirrorOnDropImpl::getSrv6BindingSidNonLastSidDropReason() const {
  return kTajoDropReasonSrv6BindingSidNonLastSid;
}

uint16_t TajoMirrorOnDropImpl::getSrv6MidpointUnresolvedDropReason() const {
  return kTajoDropReasonSrv6MidpointUnresolved;
}

void TajoMirrorOnDropImpl::configureErspanMirror(
    cfg::SwitchConfig& /*config*/,
    const std::string& /*mirrorName*/,
    const folly::IPAddressV6& /*tunnelDstIp*/,
    const folly::IPAddressV6& /*tunnelSrcIp*/,
    const PortID& /*srcPortId*/) const {
  GTEST_SKIP() << "TajoMirrorOnDropImpl::configureErspanMirror not yet "
                  "implemented; the sampling test is unsupported on Tajo";
}

ProductionFeature TajoMirrorOnDropImpl::getProductionFeature() const {
  return ProductionFeature::MIRROR_ON_DROP_STATELESS;
}

} // namespace facebook::fboss
