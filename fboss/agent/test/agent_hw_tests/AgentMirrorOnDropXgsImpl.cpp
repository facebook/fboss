// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropXgsImpl.h"

#include <gtest/gtest.h>
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/packet/bcm/XgsPsampMod.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

namespace {

// BCM SDK drop reason codes for XGS (Tomahawk5/Tomahawk6). Sourced from BCM
// SDK headers; if the SDK rev changes these values, MoD tests will fail with
// mismatched dropReason values and the failing assertion will point here.
constexpr uint8_t kBcmDropReasonL3DstDiscard = 0x1A;
constexpr uint8_t kBcmDropReasonIngressFp = 0x10;
constexpr uint8_t kBcmDropReasonEgressPort = 0x03;

struct XgsMirrorOnDropPacketParsed {
  EthHdr ethHeader;
  IPv6Hdr ipv6Header;
  UDPHeader udpHeader;
  psamp::XgsPsampModPacket psampPacket;
};

XgsMirrorOnDropPacketParsed deserializeXgsMirrorOnDropPacket(
    const folly::IOBuf* buf) {
  XgsMirrorOnDropPacketParsed parsed;
  folly::io::Cursor cursor(buf);
  parsed.ethHeader = EthHdr(cursor);
  parsed.ipv6Header = IPv6Hdr(cursor);
  parsed.udpHeader.parse(&cursor);
  parsed.psampPacket = psamp::XgsPsampModPacket::deserialize(cursor);
  return parsed;
}

} // namespace

cfg::MirrorOnDropReport XgsMirrorOnDropImpl::makeReport(
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
  report.mirrorPort().ensure().tunnel().ensure().srcIp() = switchIp.str();
  return report;
}

MirrorOnDropPacketFields XgsMirrorOnDropImpl::parsePacket(
    const folly::IOBuf* buf) const {
  auto parsed = deserializeXgsMirrorOnDropPacket(buf);

  auto innerBuf = folly::IOBuf::copyBuffer(
      parsed.psampPacket.data.sampledPacketData.data(),
      parsed.psampPacket.data.sampledPacketData.size());
  folly::io::Cursor innerCursor(innerBuf.get());
  EthHdr innerEth(innerCursor);
  IPv6Hdr innerIpv6(innerCursor);
  UDPHeader innerUdp;
  innerUdp.parse(&innerCursor);

  return {
      .outerSrcMac = parsed.ethHeader.getSrcMac(),
      .outerDstMac = parsed.ethHeader.getDstMac(),
      .outerSrcIp = parsed.ipv6Header.srcAddr,
      .outerDstIp = parsed.ipv6Header.dstAddr,
      .outerSrcPort = parsed.udpHeader.srcPort,
      .outerDstPort = parsed.udpHeader.dstPort,
      .ingressPort = parsed.psampPacket.data.ingressPort,
      .dropReasonIngress = parsed.psampPacket.data.dropReasonIngress,
      // XgsPsampData uses dropReasonMmu; the common struct abstracts it as
      // dropReasonEgress.
      .dropReasonEgress = parsed.psampPacket.data.dropReasonMmu,
      .innerSrcMac = innerEth.getSrcMac(),
      .innerDstMac = innerEth.getDstMac(),
      .innerSrcIp = innerIpv6.srcAddr,
      .innerDstIp = innerIpv6.dstAddr,
      .innerSrcPort = innerUdp.srcPort,
      .innerDstPort = innerUdp.dstPort,
  };
}

void XgsMirrorOnDropImpl::verifyInvariants(const folly::IOBuf* buf) const {
  auto parsed = deserializeXgsMirrorOnDropPacket(buf);
  EXPECT_EQ(
      parsed.ipv6Header.nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  EXPECT_EQ(parsed.udpHeader.csum, 0);
  EXPECT_EQ(parsed.psampPacket.ipfixHeader.version, psamp::IPFIX_VERSION);
  EXPECT_EQ(
      parsed.psampPacket.templateHeader.templateId,
      psamp::XGS_PSAMP_TEMPLATE_ID);
  EXPECT_EQ(
      parsed.psampPacket.data.varLenIndicator,
      psamp::XGS_PSAMP_VAR_LEN_INDICATOR);
}

uint16_t XgsMirrorOnDropImpl::getDefaultRouteDropReason() const {
  return kBcmDropReasonL3DstDiscard;
}

uint16_t XgsMirrorOnDropImpl::getAclDropReason() const {
  return kBcmDropReasonIngressFp;
}

uint16_t XgsMirrorOnDropImpl::getMmuDropReason() const {
  return kBcmDropReasonEgressPort;
}

void XgsMirrorOnDropImpl::configureErspanMirror(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    const folly::IPAddressV6& tunnelDstIp,
    const folly::IPAddressV6& tunnelSrcIp,
    const PortID& srcPortId) const {
  cfg::Mirror mirror;
  mirror.name() = mirrorName;
  mirror.destination().ensure().tunnel().ensure().greTunnel().ensure().ip() =
      tunnelDstIp.str();
  mirror.destination()->tunnel()->srcIp() = tunnelSrcIp.str();
  mirror.truncate() = true;
  config.mirrors()->push_back(mirror);
  utility::findCfgPort(config, srcPortId)->ingressMirror() = mirrorName;
}

ProductionFeature XgsMirrorOnDropImpl::getProductionFeature() const {
  return ProductionFeature::MIRROR_ON_DROP_STATELESS;
}

} // namespace facebook::fboss
