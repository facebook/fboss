// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/CsigTestUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/sai/api/SaiApiError.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/UDPHeader.h"

#include <folly/logging/xlog.h>

#include <vector>

extern "C" {
#include <sai.h>
#include <saiportcustom.h>
#include <saiswitchcustom.h>
}

namespace facebook::fboss::utility {

namespace {

PortSaiId getPortSaiId(const HwSwitch* hw, PortID portId) {
  const auto* saiSwitch = dynamic_cast<const SaiSwitch*>(hw);
  if (!saiSwitch) {
    throw FbossError("HwSwitch is not a SaiSwitch");
  }
  auto* portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portId);
  if (!portHandle || !portHandle->port) {
    throw FbossError("No SAI port handle for port ", portId);
  }
  return portHandle->port->adapterKey();
}

SwitchSaiId getSwitchSaiId(const HwSwitch* hw) {
  const auto* saiSwitch = dynamic_cast<const SaiSwitch*>(hw);
  if (!saiSwitch) {
    throw FbossError("HwSwitch is not a SaiSwitch");
  }
  return saiSwitch->getSaiSwitchId();
}

#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)

sai_switch_api_t* getSwitchApi() {
  sai_switch_api_t* switchApi = nullptr;
  auto status =
      sai_api_query(SAI_API_SWITCH, reinterpret_cast<void**>(&switchApi));
  saiApiCheckError(status, SAI_API_SWITCH, "query switch api");
  if (!switchApi) {
    throw FbossError("Failed to query SAI switch API");
  }
  return switchApi;
}

sai_port_api_t* getPortApi() {
  sai_port_api_t* portApi = nullptr;
  auto status = sai_api_query(SAI_API_PORT, reinterpret_cast<void**>(&portApi));
  saiApiCheckError(status, SAI_API_PORT, "query port api");
  if (!portApi) {
    throw FbossError("Failed to query SAI port API");
  }
  return portApi;
}

void setSwitchRawAttr(
    SwitchSaiId switchId,
    sai_attr_id_t attrId,
    const sai_attribute_value_t& value) {
  sai_attribute_t attr{};
  attr.id = attrId;
  attr.value = value;
  auto status = getSwitchApi()->set_switch_attribute(switchId, &attr);
  saiApiCheckError(status, SAI_API_SWITCH, "set switch attr ", attrId);
}

void setPortRawAttr(PortSaiId portId, sai_attr_id_t attrId, int32_t value) {
  sai_attribute_t attr{};
  attr.id = attrId;
  attr.value.s32 = value;
  auto status = getPortApi()->set_port_attribute(portId, &attr);
  saiApiCheckError(status, SAI_API_PORT, "set port attr ", attrId);
}

void setPortRawAttrU8(PortSaiId portId, sai_attr_id_t attrId, uint8_t value) {
  sai_attribute_t attr{};
  attr.id = attrId;
  attr.value.u8 = value;
  auto status = getPortApi()->set_port_attribute(portId, &attr);
  saiApiCheckError(status, SAI_API_PORT, "set port attr ", attrId);
}

#endif // SAI_API_VERSION >= SAI_VERSION(1, 13, 0)

} // namespace

bool isCsigSupportedAsic(const HwAsic* asic) {
#if SAI_API_VERSION < SAI_VERSION(1, 13, 0)
  return false;
#else
  if (!asic) {
    return false;
  }
  switch (asic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_G202X:
      return true;
    default:
      return false;
  }
#endif
}

void configureCsigTpid(const HwSwitch* hw) {
#if SAI_API_VERSION < SAI_VERSION(1, 13, 0)
  throw FbossError("CSIG requires SAI API >= 1.13");
#else
  static const std::vector<uint16_t> kTpids = {kCsigTpid};
  sai_u16_list_t tpidList;
  tpidList.count = kTpids.size();
  tpidList.list = const_cast<uint16_t*>(kTpids.data());

  auto switchId = getSwitchSaiId(hw);
  sai_attribute_value_t value{};
  value.u16list = tpidList;
  setSwitchRawAttr(switchId, SAI_SWITCH_ATTR_CSIG_TPID, value);
#endif
}

void configureCsigHistograms(const HwSwitch* hw) {
#if SAI_API_VERSION < SAI_VERSION(1, 13, 0)
  throw FbossError("CSIG requires SAI API >= 1.13");
#else
  static const std::vector<uint32_t> kAbwHistogram = {
      40000, 80000, 120000, 160000, 180000, 190000, 198000};
  static const std::vector<uint8_t> kAbwcHistogram = {
      5, 10, 25, 40, 55, 70, 85};

  sai_u32_list_t abwList;
  abwList.count = kAbwHistogram.size();
  abwList.list = const_cast<uint32_t*>(kAbwHistogram.data());

  sai_u8_list_t abwcList;
  abwcList.count = kAbwcHistogram.size();
  abwcList.list = const_cast<uint8_t*>(kAbwcHistogram.data());

  auto switchId = getSwitchSaiId(hw);
  sai_attribute_value_t value{};
  value.u32list = abwList;
  setSwitchRawAttr(switchId, SAI_SWITCH_ATTR_CSIG_ABW_HISTOGRAM_SPEC, value);

  value.u8list = abwcList;
  setSwitchRawAttr(switchId, SAI_SWITCH_ATTR_CSIG_ABWC_HISTOGRAM_SPEC, value);

  configureCsigTpid(hw);
#endif
}

void configureCsigTgenHistograms(const HwSwitch* hw) {
#if SAI_API_VERSION < SAI_VERSION(1, 13, 0)
  throw FbossError("CSIG requires SAI API >= 1.13");
#else
  // Matches sai/test/python/hw/csig/test_csig_basic_v4_v6.py
  // csig_tgen_abwc_histogram_spec at 100G: abw = percent * speed_Gbps * 10.
  static const std::vector<uint32_t> kAbwHistogram = {
      15000, 25000, 35000, 45000, 55000, 65000, 85000};
  static const std::vector<uint8_t> kAbwcHistogram = {
      15, 25, 35, 45, 55, 65, 85};

  sai_u32_list_t abwList;
  abwList.count = kAbwHistogram.size();
  abwList.list = const_cast<uint32_t*>(kAbwHistogram.data());

  sai_u8_list_t abwcList;
  abwcList.count = kAbwcHistogram.size();
  abwcList.list = const_cast<uint8_t*>(kAbwcHistogram.data());

  auto switchId = getSwitchSaiId(hw);
  sai_attribute_value_t value{};
  value.u32list = abwList;
  setSwitchRawAttr(switchId, SAI_SWITCH_ATTR_CSIG_ABW_HISTOGRAM_SPEC, value);

  value.u8list = abwcList;
  setSwitchRawAttr(switchId, SAI_SWITCH_ATTR_CSIG_ABWC_HISTOGRAM_SPEC, value);

  configureCsigTpid(hw);
#endif
}

void configureCsigForTgenStyleTest(
    const HwSwitch* hw,
    PortID floodPort,
    PortID egressPort) {
  configureCsigTgenHistograms(hw);
  configureCsigIngressPort(hw, floodPort);
  configureCsigEgressPort(hw, egressPort, kDefaultCsigLinkLocator);
}

void configureCsigPort(
    const HwSwitch* hw,
    PortID port,
    int32_t tagAction,
    std::optional<uint8_t> linkLocator) {
#if SAI_API_VERSION < SAI_VERSION(1, 13, 0)
  throw FbossError("CSIG requires SAI API >= 1.13");
#else
  auto portId = getPortSaiId(hw, port);
  setPortRawAttr(portId, SAI_PORT_ATTR_CSIG_TAG_ACTION, tagAction);
  if (linkLocator.has_value()) {
    setPortRawAttrU8(
        portId, SAI_PORT_ATTR_CSIG_LINK_LOCATOR, linkLocator.value());
  }
#endif
}

void configureCsigEgressPort(
    const HwSwitch* hw,
    PortID egressPort,
    uint8_t linkLocator) {
  configureCsigPort(
      hw, egressPort, SAI_PORT_CSIG_TAG_ACTION_UPDATE, linkLocator);
}

void configureCsigIngressPort(const HwSwitch* hw, PortID ingressPort) {
  configureCsigPort(
      hw, ingressPort, SAI_PORT_CSIG_TAG_ACTION_UPDATE, std::nullopt);
}

void configureCsigPassthroughPort(const HwSwitch* hw, PortID port) {
  configureCsigPort(
      hw, port, SAI_PORT_CSIG_TAG_ACTION_PASSTHROUGH, std::nullopt);
}

void configureCsigPassthroughPorts(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort) {
  configureCsigHistograms(hw);
  configureCsigPassthroughPort(hw, ingressPort);
  configureCsigPassthroughPort(hw, egressPort);
  setPortRawAttrU8(
      getPortSaiId(hw, egressPort),
      SAI_PORT_ATTR_CSIG_LINK_LOCATOR,
      kDefaultCsigLinkLocator);
}

void configureCsigStripPort(const HwSwitch* hw, PortID port) {
  configureCsigPort(hw, port, SAI_PORT_CSIG_TAG_ACTION_STRIP, std::nullopt);
}

void configureCsigStripPorts(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort) {
  configureCsigHistograms(hw);
  configureCsigStripPort(hw, ingressPort);
  configureCsigStripPort(hw, egressPort);
}

void configureCsigForSrv6EncapTest(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort) {
  configureCsigHistograms(hw);
  // Congested UPDATE: ingress PASSTHROUGH (probe CSIG unchanged on inject),
  // egress UPDATE + histogram drives ARC ABWC stamp (same ingress model as
  // quiet-clamp; differs only by flood + dynamic expect 0x10-0x16).
  configureCsigPassthroughPort(hw, ingressPort);
  configureCsigEgressPort(hw, egressPort, kDefaultCsigLinkLocator);
}

void configureCsigForSrv6QuietClampTest(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort) {
  // Match SDK srv6_fpp_base / csig_base quiet UPDATE: ingress untouched,
  // egress sys_port UPDATE only (no switch histogram). Toggle PASS->UPDATE so
  // la_system_port_fpp::set_csig_tag_action reprograms csig_dsp_attr (UPDATE
  // when already UPDATE returns early without refreshing abwc/LM). Egress LM is
  // stamped on the wire from kCsigQuietClampEgressLinkLocator (SDK behavior).
  configureCsigTpid(hw);
  configureCsigPassthroughPort(hw, ingressPort);
  configureCsigPort(
      hw, egressPort, SAI_PORT_CSIG_TAG_ACTION_PASSTHROUGH, std::nullopt);
  configureCsigPort(
      hw,
      egressPort,
      SAI_PORT_CSIG_TAG_ACTION_UPDATE,
      kCsigQuietClampEgressLinkLocator);
}

void configureCsigForSrv6EncapTest(const HwSwitch* hw, PortID egressPort) {
  configureCsigForSrv6EncapTest(hw, std::vector<PortID>{egressPort});
}

void configureCsigForSrv6EncapTest(
    const HwSwitch* hw,
    const std::vector<PortID>& ports) {
  configureCsigHistograms(hw);
  for (auto port : ports) {
    configureCsigEgressPort(hw, port, kDefaultCsigLinkLocator);
  }
}

std::optional<CsigTag> parseCsigTag(folly::io::Cursor& cursor) {
  if (cursor.isAtEnd() || cursor.length() < 4) {
    return std::nullopt;
  }
  auto word1 = cursor.readBE<uint16_t>();
  auto word2 = cursor.readBE<uint16_t>();

  CsigTag tag;
  tag.signalType = (word1 >> 13) & 0x7;
  tag.reserved = (word1 >> 12) & 0x1;
  tag.signal = (word1 >> 7) & 0x1f;
  tag.locatorMetadata = word1 & 0x7f;
  tag.nextEtherType = word2;
  return tag;
}

namespace {

uint16_t encodeCsigTagWord1(
    uint8_t signalType,
    uint8_t reserved,
    uint8_t signal,
    uint8_t locatorMetadata) {
  return static_cast<uint16_t>(
      ((signalType & 0x7) << 13) | ((reserved & 0x1) << 12) |
      ((signal & 0x1f) << 7) | (locatorMetadata & 0x7f));
}

std::vector<uint8_t> defaultCsigTestPayload() {
  return std::vector<uint8_t>(kCsigBasicTestPadLen, 0);
}

} // namespace

void writeCsigShim(
    folly::io::RWPrivateCursor& cursor,
    uint16_t nextEtherType,
    uint8_t signalType,
    uint8_t signal,
    uint8_t locatorMetadata,
    uint8_t reserved) {
  cursor.writeBE<uint16_t>(
      encodeCsigTagWord1(signalType, reserved, signal, locatorMetadata));
  cursor.writeBE<uint16_t>(nextEtherType);
}

template <typename IPHDR>
std::unique_ptr<TxPacket> makeCsigUdpTxPacketImpl(
    const AllocatePktFn& allocatePacket,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    uint16_t nextEtherType,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    const std::vector<uint8_t>& payloadBytes,
    uint8_t csigSignalType,
    uint8_t csigSignal,
    uint8_t csigLocatorMetadata) {
  CHECK(!vlan.has_value())
      << "CSIG ingress must be untagged L2: [dst|src|0x9900|CSIG|IP|...]";
  constexpr auto kCsigShimSize = 4;
  auto txPacket = allocatePacket(
      EthHdr::SIZE + kCsigShimSize + ipHdr.size() + udpHdr.size() +
      payloadBytes.size());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  auto ethHdr =
      makeEthHdr(srcMac, dstMac, vlan, static_cast<ETHERTYPE>(kCsigTpid));
  if (!ethHdr.getVlanTags().empty()) {
    const auto& vlanTag = ethHdr.getVlanTags()[0];
    rwCursor.push(ethHdr.getDstMac().bytes(), folly::MacAddress::SIZE);
    rwCursor.push(ethHdr.getSrcMac().bytes(), folly::MacAddress::SIZE);
    rwCursor.writeBE<uint16_t>(vlanTag.tpid());
    rwCursor.writeBE<uint16_t>(static_cast<uint16_t>(vlanTag.value));
    rwCursor.writeBE<uint16_t>(ethHdr.getEtherType());
  } else {
    txPacket->writeEthHeader(
        &rwCursor,
        ethHdr.getDstMac(),
        ethHdr.getSrcMac(),
        ethHdr.getEtherType());
  }
  writeCsigShim(
      rwCursor, nextEtherType, csigSignalType, csigSignal, csigLocatorMetadata);
  ipHdr.serialize(&rwCursor);

  rwCursor.writeBE<uint16_t>(udpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(udpHdr.dstPort);
  rwCursor.writeBE<uint16_t>(udpHdr.length);
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payloadBytes.data(), payloadBytes.size());
  uint16_t csum = udpHdr.computeChecksum(ipHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
}

std::unique_ptr<TxPacket> makeCsigUdpTxPacket(
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
    std::optional<std::vector<uint8_t>> payload,
    uint8_t csigSignalType,
    uint8_t csigSignal,
    uint8_t csigLocatorMetadata) {
  if (!payload) {
    payload = defaultCsigTestPayload();
  }
  const auto& payloadBytes = payload.value();
  IPv4Hdr ipHdr(
      srcIp,
      dstIp,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      payloadBytes.size() + UDPHeader::size());
  ipHdr.dscp = dscp >> 2;
  ipHdr.ecn = dscp & 0x3;
  ipHdr.ttl = ttl;
  ipHdr.computeChecksum();
  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());
  return makeCsigUdpTxPacketImpl(
      allocatePacket,
      vlan,
      srcMac,
      dstMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4),
      ipHdr,
      udpHdr,
      payloadBytes,
      csigSignalType,
      csigSignal,
      csigLocatorMetadata);
}

std::unique_ptr<TxPacket> makeCsigUdpTxPacket(
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
    std::optional<std::vector<uint8_t>> payload,
    uint8_t csigSignalType,
    uint8_t csigSignal,
    uint8_t csigLocatorMetadata) {
  if (!payload) {
    payload = defaultCsigTestPayload();
  }
  const auto& payloadBytes = payload.value();
  IPv6Hdr ipHdr(srcIp, dstIp);
  ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  ipHdr.trafficClass = trafficClass;
  ipHdr.payloadLength = UDPHeader::size() + payloadBytes.size();
  ipHdr.hopLimit = hopLimit;
  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());
  return makeCsigUdpTxPacketImpl(
      allocatePacket,
      vlan,
      srcMac,
      dstMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      ipHdr,
      udpHdr,
      payloadBytes,
      csigSignalType,
      csigSignal,
      csigLocatorMetadata);
}

std::unique_ptr<TxPacket> makeCsigIpInIpTxPacket(
    const AllocatePktFn& allocatePacket,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& outerSrcIp,
    const folly::IPAddressV6& outerDstIp,
    const folly::IPAddressV6& innerSrcIp,
    const folly::IPAddressV6& innerDstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t outerTrafficClass,
    uint8_t innerTrafficClass,
    uint8_t outerHopLimit,
    std::optional<uint8_t> innerHopLimit,
    uint32_t outerFlowLabel,
    std::optional<std::vector<uint8_t>> payload,
    uint8_t csigSignalType,
    uint8_t csigSignal,
    uint8_t csigLocatorMetadata) {
  CHECK(!vlan.has_value())
      << "CSIG ingress must be untagged L2: [dst|src|0x9900|CSIG|outer IP|...]";
  if (!payload) {
    payload = defaultCsigTestPayload();
  }
  const auto& payloadBytes = payload.value();

  IPv6Hdr outerIpHdr(outerSrcIp, outerDstIp);
  outerIpHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6);
  outerIpHdr.trafficClass = outerTrafficClass;
  outerIpHdr.flowLabel = outerFlowLabel;
  outerIpHdr.payloadLength =
      IPv6Hdr::size() + UDPHeader::size() + payloadBytes.size();
  outerIpHdr.hopLimit = outerHopLimit;

  IPv6Hdr innerIpHdr(innerSrcIp, innerDstIp);
  innerIpHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  innerIpHdr.trafficClass = innerTrafficClass;
  innerIpHdr.payloadLength = UDPHeader::size() + payloadBytes.size();
  innerIpHdr.hopLimit = innerHopLimit.value_or(outerHopLimit);

  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());

  constexpr auto kCsigShimSize = 4;
  auto txPacket = allocatePacket(
      EthHdr::SIZE + kCsigShimSize + outerIpHdr.size() + innerIpHdr.size() +
      UDPHeader::size() + payloadBytes.size());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  txPacket->writeEthHeader(
      &rwCursor, dstMac, srcMac, static_cast<uint16_t>(kCsigTpid));
  writeCsigShim(
      rwCursor,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      csigSignalType,
      csigSignal,
      csigLocatorMetadata);
  outerIpHdr.serialize(&rwCursor);
  innerIpHdr.serialize(&rwCursor);

  rwCursor.writeBE<uint16_t>(udpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(udpHdr.dstPort);
  rwCursor.writeBE<uint16_t>(udpHdr.length);
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payloadBytes.data(), payloadBytes.size());
  uint16_t csum = udpHdr.computeChecksum(innerIpHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
}

std::unique_ptr<TxPacket> makeCsigIpInIpTxPacket(
    const AllocatePktFn& allocatePacket,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& outerSrcIp,
    const folly::IPAddressV6& outerDstIp,
    const folly::IPAddressV4& innerSrcIp,
    const folly::IPAddressV4& innerDstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t outerTrafficClass,
    uint8_t innerDscp,
    uint8_t outerHopLimit,
    std::optional<uint8_t> innerHopLimit,
    uint32_t outerFlowLabel,
    std::optional<std::vector<uint8_t>> payload,
    uint8_t csigSignalType,
    uint8_t csigSignal,
    uint8_t csigLocatorMetadata) {
  CHECK(!vlan.has_value())
      << "CSIG ingress must be untagged L2: [dst|src|0x9900|CSIG|outer IP|...]";
  if (!payload) {
    payload = defaultCsigTestPayload();
  }
  const auto& payloadBytes = payload.value();

  IPv6Hdr outerIpHdr(outerSrcIp, outerDstIp);
  outerIpHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4);
  outerIpHdr.trafficClass = outerTrafficClass;
  outerIpHdr.flowLabel = outerFlowLabel;
  outerIpHdr.payloadLength =
      IPv4Hdr::minSize() + UDPHeader::size() + payloadBytes.size();
  outerIpHdr.hopLimit = outerHopLimit;

  IPv4Hdr innerIpHdr(
      innerSrcIp,
      innerDstIp,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      UDPHeader::size() + payloadBytes.size());
  innerIpHdr.dscp = innerDscp;
  innerIpHdr.ttl = innerHopLimit.value_or(outerHopLimit);
  innerIpHdr.computeChecksum();

  UDPHeader udpHdr(srcPort, dstPort, UDPHeader::size() + payloadBytes.size());

  constexpr auto kCsigShimSize = 4;
  auto txPacket = allocatePacket(
      EthHdr::SIZE + kCsigShimSize + outerIpHdr.size() + innerIpHdr.size() +
      UDPHeader::size() + payloadBytes.size());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  txPacket->writeEthHeader(
      &rwCursor, dstMac, srcMac, static_cast<uint16_t>(kCsigTpid));
  writeCsigShim(
      rwCursor,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      csigSignalType,
      csigSignal,
      csigLocatorMetadata);
  outerIpHdr.serialize(&rwCursor);
  innerIpHdr.serialize(&rwCursor);

  rwCursor.writeBE<uint16_t>(udpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(udpHdr.dstPort);
  rwCursor.writeBE<uint16_t>(udpHdr.length);
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  folly::io::Cursor payloadStart(rwCursor);
  rwCursor.push(payloadBytes.data(), payloadBytes.size());
  uint16_t csum = udpHdr.computeChecksum(innerIpHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(csum);
  return txPacket;
}

} // namespace facebook::fboss::utility
