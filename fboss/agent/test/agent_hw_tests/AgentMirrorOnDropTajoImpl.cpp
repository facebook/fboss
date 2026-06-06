// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTajoImpl.h"

#include <gtest/gtest.h>

#include <folly/Conv.h>
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/TCPHeader.h"
#include "fboss/agent/packet/TajoPuntHeader.h"
#include "fboss/agent/packet/UDPHeader.h"

namespace facebook::fboss {

namespace {

// GR2/Tajo drop-reason expectations used by stateless MoD tests.
//
// Source references used for these constants:
// - L3 default-route miss and L3 ACL drop trap codes are defined in
//   datacenter/npl/fpp_app/traps/trap_code.npl:
//     TRAP_CODE_L3_LPM_DROP = 0x8c
//     TRAP_CODE_L3_ACL_DROP = 0x6b
//
constexpr uint16_t kTajoDropReasonDefaultRoute = 0x8c;
constexpr uint16_t kTajoDropReasonAcl = 0x6b;
constexpr uint16_t kTajoDropReasonMmu = 0x10;
// TODO: replace with actual Tajo drop-reason codes for each SRv6 scenario
// once available from the vendor SDK. These are placeholders.
constexpr uint16_t kTajoDropReasonSrv6MidpointNonLastSid = 0x00;
constexpr uint16_t kTajoDropReasonSrv6DecapNonLastSegment = 0x01;
constexpr uint16_t kTajoDropReasonSrv6BindingSidNonLastSid = 0x02;
constexpr uint16_t kTajoDropReasonSrv6MidpointUnresolved = 0x03;

struct TajoMirrorOnDropPacketParsed {
  EthHdr ethHeader;
  IPv6Hdr ipv6Header;
  UDPHeader udpHeader;
  ParsedTajoPuntHeader puntHeader{};
  EthHdr innerEth;
  std::optional<IPv4Hdr> innerIpv4;
  std::optional<IPv6Hdr> innerIpv6;
  std::optional<UDPHeader> innerUdp;
  std::optional<TCPHeader> innerTcp;
};

folly::IPAddressV6 toV6(const folly::IPAddressV6& ip) {
  return ip;
}

folly::IPAddressV6 toV6(const folly::IPAddressV4& ip) {
  return folly::IPAddressV6(folly::to<std::string>("::ffff:", ip.str()));
}

TajoMirrorOnDropPacketParsed deserializeTajoMirrorOnDropPacket(
    const folly::IOBuf* buf) {
  TajoMirrorOnDropPacketParsed parsed;
  folly::io::Cursor cursor(buf);
  parsed.ethHeader = EthHdr(cursor);
  parsed.ipv6Header = IPv6Hdr(cursor);
  parsed.udpHeader.parse(&cursor);
  parsed.puntHeader = parseTajoPuntHeader(cursor);
  parsed.innerEth = EthHdr(cursor);

  if (parsed.innerEth.getEtherType() ==
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
    auto innerV6 = IPv6Hdr(cursor);
    const auto nextHeader = innerV6.nextHeader;
    parsed.innerIpv6 = innerV6;
    if (nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
      UDPHeader udp;
      udp.parse(&cursor);
      parsed.innerUdp = udp;
    } else if (nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP)) {
      TCPHeader tcp;
      tcp.parse(&cursor);
      parsed.innerTcp = tcp;
    }
  } else if (
      parsed.innerEth.getEtherType() ==
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
    auto innerV4 = IPv4Hdr(cursor);
    const auto proto = innerV4.protocol;
    parsed.innerIpv4 = innerV4;
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
  // Tajo MoD is routed out a front-panel port; mirrorPort tunnel src IP
  // mirrors XGS so resolution picks the correct egress port.
  report.mirrorPort().ensure().tunnel().ensure().srcIp() = switchIp.str();
  return report;
}

MirrorOnDropPacketFields TajoMirrorOnDropImpl::parsePacket(
    const folly::IOBuf* buf) const {
  auto parsed = deserializeTajoMirrorOnDropPacket(buf);
  // Tajo's punt header carries a single `dropReason` field — there is no
  // separate ingress vs. MMU vs. egress reason like XGS. We surface the
  // value as `dropReasonIngress` and leave `dropReasonEgress` zero.
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
      .ingressPort = parsed.puntHeader.ingressPort,
      .dropReasonIngress = parsed.puntHeader.dropReason,
      .dropReasonEgress = 0,
      .innerSrcMac = parsed.innerEth.getSrcMac(),
      .innerDstMac = parsed.innerEth.getDstMac(),
      .innerSrcIp = innerSrcIp,
      .innerDstIp = innerDstIp,
      .innerSrcPort = innerSrcPort,
      .innerDstPort = innerDstPort,
  };
}

void TajoMirrorOnDropImpl::verifyInvariants(const folly::IOBuf* buf) const {
  auto parsed = deserializeTajoMirrorOnDropPacket(buf);
  // Outer transport for MoD export must be UDP over IPv6.
  EXPECT_EQ(
      parsed.ipv6Header.nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  // Match XGS invariant behavior: exported outer UDP checksum is expected to be
  // zero for this encapsulation path.
  EXPECT_EQ(parsed.udpHeader.csum, 0);

  // Legacy fallback parser validates known fixed/sentinel bytes observed in
  // the Tajo punt wire format.
  if (!parsed.puntHeader.parsedBySdk) {
    EXPECT_EQ(parsed.puntHeader.version, 0x09);
    EXPECT_EQ(parsed.puntHeader.subType, 0xFF10);
    EXPECT_EQ(parsed.puntHeader.reasonExt, 0xFF);
    EXPECT_EQ(parsed.puntHeader.reserved1, 0xFFFF);
    EXPECT_EQ(parsed.puntHeader.reserved2, 0x0000);
  }
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
  // The high-drop-rate ERSPAN sampling test (`MirrorOnDropWithSampling`) is
  // not yet validated on Tajo. Once Tajo's mirror programming model is
  // confirmed, this should mirror the XGS impl: set up a GRE tunnel mirror on
  // srcPortId. Until then, the test gate (MIRROR_ON_DROP_STATELESS not in
  // Tajo's per-ASIC feature list) keeps this path off Tajo hardware. If
  // someone manually runs the sampling test on Tajo, skip cleanly rather
  // than crash the whole test fixture mid-setup.
  GTEST_SKIP() << "TajoMirrorOnDropImpl::configureErspanMirror not yet "
                  "implemented; the sampling test is unsupported on Tajo";
}

ProductionFeature TajoMirrorOnDropImpl::getProductionFeature() const {
  return ProductionFeature::MIRROR_ON_DROP_STATELESS;
}

} // namespace facebook::fboss
