// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTajoImpl.h"

#include <gtest/gtest.h>

#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"

namespace facebook::fboss {

namespace {

// TODO(Tajo): replace these placeholder drop-reason codes with the actual
// values from Tajo's published drop-reason enumeration. The MoD sample
// packet capture only contained MMU/egress-class drops with dropReason
// values 0x0001-0x0004; ingress-pipeline drops (ACL, default route) were
// not present, so the correct codes for those are still unknown. We keep
// all three at 0x00 placeholders rather than guessing at MMU=0x01 based on
// the capture, because the capture meanings were inferred from ASCII
// strings inside the inner payloads, not from a Tajo spec. While these are
// 0x00, every reason check in tests degenerates to 0x00 == 0x00 (always
// true). The test class is gated off Tajo via configerator
// (MIRROR_ON_DROP_STATELESS opt-in), so this path is unreachable in CI
// today — but a manual run will silently pass on bogus reasons.
constexpr uint16_t kTajoDropReasonDefaultRoute = 0x00;
constexpr uint16_t kTajoDropReasonAcl = 0x00;
constexpr uint16_t kTajoDropReasonMmu = 0x00;

// Tajo proprietary header, observed in the MoD sample packet capture: a
// fixed-size 28-byte punt header sitting between the outer UDP and the
// inner Ethernet frame. All multi-byte fields are big-endian on the wire.
struct TajoModPuntHeader {
  uint8_t version{};
  uint8_t flags{};
  uint16_t sequence{};
  uint16_t dropReason{};
  uint8_t dropFlags{};
  uint8_t reasonExt{};
  uint16_t subType{};
  uint16_t dropCategory{};
  uint16_t reserved1{};
  uint16_t reserved2{};
  uint16_t observationDomain{};
  uint16_t flowId{};
  uint16_t ingressPort{};
  uint16_t ingressInfo{};
  uint16_t egressPort{};
  uint16_t egressInfo{};
} __attribute__((packed));
static_assert(sizeof(TajoModPuntHeader) == 28);

struct TajoMirrorOnDropPacketParsed {
  EthHdr ethHeader;
  IPv6Hdr ipv6Header;
  UDPHeader udpHeader;
  TajoModPuntHeader puntHeader{};
  EthHdr innerEth;
  IPv6Hdr innerIpv6;
  UDPHeader innerUdp;
};

TajoMirrorOnDropPacketParsed deserializeTajoMirrorOnDropPacket(
    const folly::IOBuf* buf) {
  TajoMirrorOnDropPacketParsed parsed;
  folly::io::Cursor cursor(buf);
  parsed.ethHeader = EthHdr(cursor);
  parsed.ipv6Header = IPv6Hdr(cursor);
  parsed.udpHeader.parse(&cursor);
  // Read each TajoModPuntHeader field with explicit byte-order. A raw
  // cursor.pull() of the packed struct would leave multi-byte fields in
  // network byte order on the host, breaking any value >= 256.
  parsed.puntHeader.version = cursor.read<uint8_t>();
  parsed.puntHeader.flags = cursor.read<uint8_t>();
  parsed.puntHeader.sequence = cursor.readBE<uint16_t>();
  parsed.puntHeader.dropReason = cursor.readBE<uint16_t>();
  parsed.puntHeader.dropFlags = cursor.read<uint8_t>();
  parsed.puntHeader.reasonExt = cursor.read<uint8_t>();
  parsed.puntHeader.subType = cursor.readBE<uint16_t>();
  parsed.puntHeader.dropCategory = cursor.readBE<uint16_t>();
  parsed.puntHeader.reserved1 = cursor.readBE<uint16_t>();
  parsed.puntHeader.reserved2 = cursor.readBE<uint16_t>();
  parsed.puntHeader.observationDomain = cursor.readBE<uint16_t>();
  parsed.puntHeader.flowId = cursor.readBE<uint16_t>();
  parsed.puntHeader.ingressPort = cursor.readBE<uint16_t>();
  parsed.puntHeader.ingressInfo = cursor.readBE<uint16_t>();
  parsed.puntHeader.egressPort = cursor.readBE<uint16_t>();
  parsed.puntHeader.egressInfo = cursor.readBE<uint16_t>();
  parsed.innerEth = EthHdr(cursor);
  parsed.innerIpv6 = IPv6Hdr(cursor);
  parsed.innerUdp.parse(&cursor);
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
      .innerSrcIp = parsed.innerIpv6.srcAddr,
      .innerDstIp = parsed.innerIpv6.dstAddr,
      .innerSrcPort = parsed.innerUdp.srcPort,
      .innerDstPort = parsed.innerUdp.dstPort,
  };
}

void TajoMirrorOnDropImpl::verifyInvariants(const folly::IOBuf* buf) const {
  auto parsed = deserializeTajoMirrorOnDropPacket(buf);
  EXPECT_EQ(
      parsed.ipv6Header.nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  // Constant fields observed across all packets in the sample pcap. If any
  // of these fail in the future, either the Tajo header format has changed
  // or the assumed field layout is wrong.
  EXPECT_EQ(parsed.puntHeader.version, 0x09);
  EXPECT_EQ(parsed.puntHeader.subType, 0xFF10);
  EXPECT_EQ(parsed.puntHeader.reasonExt, 0xFF);
  EXPECT_EQ(parsed.puntHeader.reserved1, 0xFFFF);
  EXPECT_EQ(parsed.puntHeader.reserved2, 0x0000);
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
