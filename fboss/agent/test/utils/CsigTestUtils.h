// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include <memory>
#include <optional>
#include <vector>

namespace facebook::fboss::utility {

inline constexpr uint16_t kCsigTpid = 0x9900;
inline constexpr uint8_t kCsigAbwcSignalType = 1;
inline constexpr uint8_t kDefaultCsigLinkLocator = 0x42;
// Matches sai/test/python/hw/csig/test_csig_basic_v4_v6.py ingress CSIG tag.
inline constexpr uint8_t kCsigIngressPktLinkLocator = 0x5;
// Logical quiet-clamp ABWC signal (csig_base / srv6_fpp_base namedtuple
// signal=7).
inline constexpr uint8_t kCsigQuietClampIngressSignal = 0x7;
// SDK scapy CSIG(signal=7) default signal_buf=0x2 -> wire 99 00 2b 85 (5-bit
// signal 0x17).
inline constexpr uint8_t kCsigSdkScapyQuietClampIngressSignal = 0x17;
// SDK CsigTag(signal=0) quiet-clamp egress: signal_buf=0x2 preserved -> wire
// 5-bit 0x10, shim word1 0x2800 | lm (e.g. 99 00 28 42 when lm=0x42). Logical
// SDK signal=0.
inline constexpr uint8_t kCsigSdkScapyQuietClampEgressSignal = 0x10;
inline constexpr uint8_t kCsigQuietClampEgressSignal =
    kCsigSdkScapyQuietClampEgressSignal;
// Program a non-zero egress LM so quiet-clamp tests prove port LM stamping (SDK
// get_csig_tag_locator_metadata()), distinct from ingress probe lm=0x5.
inline constexpr uint8_t kCsigQuietClampEgressLinkLocator =
    kDefaultCsigLinkLocator;
inline constexpr uint16_t kCsigQuietClampEgressShimWord1 =
    static_cast<uint16_t>(0x2800 | kCsigQuietClampEgressLinkLocator);
inline constexpr uint8_t kCsigIngressAbwcSignal = 0x17; // signal 7 | MSB (0x10)
inline constexpr uint8_t kCsigEgressDebugAbwcSignal = 0x12; // abwc 2 | MSB
inline constexpr size_t kCsigBasicTestPadLen = 50;
// ABWC buckets 0-3 indicate congested available bandwidth (<= 40%).
inline constexpr uint8_t kMaxCongestedAbwcBucket = 3;
// Histogram spec has 7 thresholds -> buckets 0-6.
inline constexpr uint8_t kMaxAbwcHistogramBucket = 6;

struct CsigTag {
  uint8_t signalType{0};
  uint8_t reserved{0};
  uint8_t signal{0};
  uint8_t locatorMetadata{0};
  uint16_t nextEtherType{0};
};

inline bool isCongestedAbwcSignal(uint8_t signal) {
  if ((signal & 0x10) == 0) {
    return false;
  }
  return (signal & 0x0f) <= kMaxCongestedAbwcBucket;
}

// UPDATE egress: ARC rewrites ingress ABWC (0x17) to a measured bucket 0-6.
// Line-rate flood on MAC loopback often lands in buckets 4-5 (55-70% ABWC), not
// strictly congested buckets 0-3.
inline bool isEgressUpdatedAbwcSignal(uint8_t signal) {
  if ((signal & 0x10) == 0) {
    return false;
  }
  const uint8_t bucket = signal & 0x0f;
  return bucket <= kMaxAbwcHistogramBucket && signal != kCsigIngressAbwcSignal;
}

// SDK quiet-path UPDATE: egress ABWC bucket 0 (wire signal 0x10), lm from port.
inline bool isQuietClampEgressCsigSignal(uint8_t signal) {
  return signal == kCsigQuietClampEgressSignal;
}

bool isCsigSupportedAsic(const HwAsic* asic);

void configureCsigHistograms(const HwSwitch* hw);

// SDK device init: enable CSIG TPID only (no ABW/ABWC histogram).
void configureCsigTpid(const HwSwitch* hw);

void configureCsigPort(
    const HwSwitch* hw,
    PortID port,
    int32_t tagAction,
    std::optional<uint8_t> linkLocator = std::nullopt);

void configureCsigEgressPort(
    const HwSwitch* hw,
    PortID egressPort,
    uint8_t linkLocator = kDefaultCsigLinkLocator);

void configureCsigIngressPort(const HwSwitch* hw, PortID ingressPort);

void configureCsigPassthroughPort(const HwSwitch* hw, PortID port);

void configureCsigPassthroughPorts(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort);

void configureCsigStripPort(const HwSwitch* hw, PortID port);

void configureCsigStripPorts(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort);

// Congested UPDATE baseline: histogram + ingress PASSTHROUGH + egress UPDATE.
void configureCsigForSrv6EncapTest(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort);

// SDK test_csig_tgen_v4_v6 histogram and dual-port UPDATE (flood + egress).
void configureCsigTgenHistograms(const HwSwitch* hw);

void configureCsigForTgenStyleTest(
    const HwSwitch* hw,
    PortID floodPort,
    PortID egressPort);

// SDK srv6_fpp_base quiet-clamp: ingress PASSTHROUGH, egress UPDATE only;
// PASS then UPDATE to force csig_dsp_attr reprogram (UPDATE no-op otherwise).
void configureCsigForSrv6QuietClampTest(
    const HwSwitch* hw,
    PortID ingressPort,
    PortID egressPort);

void configureCsigForSrv6EncapTest(const HwSwitch* hw, PortID egressPort);

void configureCsigForSrv6EncapTest(
    const HwSwitch* hw,
    const std::vector<PortID>& ports);

std::optional<CsigTag> parseCsigTag(folly::io::Cursor& cursor);

void writeCsigShim(
    folly::io::RWPrivateCursor& cursor,
    uint16_t nextEtherType,
    uint8_t signalType = kCsigAbwcSignalType,
    uint8_t signal = kCsigIngressAbwcSignal,
    uint8_t locatorMetadata = kCsigIngressPktLinkLocator,
    uint8_t reserved = 0);

std::unique_ptr<TxPacket> makeCsigUdpTxPacket(
    const AllocatePktFn& allocatePacket,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t dscp = 0,
    uint8_t ttl = 64,
    std::optional<std::vector<uint8_t>> payload = std::nullopt,
    uint8_t csigSignalType = kCsigAbwcSignalType,
    uint8_t csigSignal = kCsigIngressAbwcSignal,
    uint8_t csigLocatorMetadata = kCsigIngressPktLinkLocator);

std::unique_ptr<TxPacket> makeCsigUdpTxPacket(
    const AllocatePktFn& allocatePacket,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 64,
    std::optional<std::vector<uint8_t>> payload = std::nullopt,
    uint8_t csigSignalType = kCsigAbwcSignalType,
    uint8_t csigSignal = kCsigIngressAbwcSignal,
    uint8_t csigLocatorMetadata = kCsigIngressPktLinkLocator);

template <typename SwitchT>
std::unique_ptr<TxPacket> makeCsigUdpTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t dscp = 0,
    uint8_t ttl = 64,
    std::optional<std::vector<uint8_t>> payload = std::nullopt,
    uint8_t csigSignalType = kCsigAbwcSignalType,
    uint8_t csigSignal = kCsigIngressAbwcSignal,
    uint8_t csigLocatorMetadata = kCsigIngressPktLinkLocator) {
  return makeCsigUdpTxPacket(
      makeAllocator(switchT),
      vlan,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      srcPort,
      dstPort,
      dscp,
      ttl,
      payload,
      csigSignalType,
      csigSignal,
      csigLocatorMetadata);
}

template <typename SwitchT>
std::unique_ptr<TxPacket> makeCsigUdpTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 64,
    std::optional<std::vector<uint8_t>> payload = std::nullopt,
    uint8_t csigSignalType = kCsigAbwcSignalType,
    uint8_t csigSignal = kCsigIngressAbwcSignal,
    uint8_t csigLocatorMetadata = kCsigIngressPktLinkLocator) {
  return makeCsigUdpTxPacket(
      makeAllocator(switchT),
      vlan,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      srcPort,
      dstPort,
      trafficClass,
      hopLimit,
      payload,
      csigSignalType,
      csigSignal,
      csigLocatorMetadata);
}

// Untagged L2: [dst|src|0x9900|CSIG|outer IPv6|inner IP|UDP|payload].
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
    uint32_t outerFlowLabel = 0,
    std::optional<std::vector<uint8_t>> payload = std::nullopt,
    uint8_t csigSignalType = kCsigAbwcSignalType,
    uint8_t csigSignal = kCsigIngressAbwcSignal,
    uint8_t csigLocatorMetadata = kCsigIngressPktLinkLocator);

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
    uint32_t outerFlowLabel = 0,
    std::optional<std::vector<uint8_t>> payload = std::nullopt,
    uint8_t csigSignalType = kCsigAbwcSignalType,
    uint8_t csigSignal = kCsigIngressAbwcSignal,
    uint8_t csigLocatorMetadata = kCsigIngressPktLinkLocator);

template <typename SwitchT>
std::unique_ptr<TxPacket> makeCsigIpInIpTxPacket(
    const SwitchT* switchT,
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
    uint32_t outerFlowLabel = 0,
    std::optional<std::vector<uint8_t>> payload = std::nullopt,
    uint8_t csigSignalType = kCsigAbwcSignalType,
    uint8_t csigSignal = kCsigIngressAbwcSignal,
    uint8_t csigLocatorMetadata = kCsigIngressPktLinkLocator) {
  return makeCsigIpInIpTxPacket(
      makeAllocator(switchT),
      vlan,
      srcMac,
      dstMac,
      outerSrcIp,
      outerDstIp,
      innerSrcIp,
      innerDstIp,
      srcPort,
      dstPort,
      outerTrafficClass,
      innerTrafficClass,
      outerHopLimit,
      innerHopLimit,
      outerFlowLabel,
      payload,
      csigSignalType,
      csigSignal,
      csigLocatorMetadata);
}

} // namespace facebook::fboss::utility
