// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/types.h"

#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/PTPHeader.h"

#include <optional>

namespace facebook::fboss {

class HwSwitch;

namespace utility {

using AllocatePktFn = std::function<std::unique_ptr<TxPacket>(uint32_t)>;

template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort,
    VlanID vlanId,
    size_t payloadSize = 256);

template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort,
    VlanID vlanId = VlanID(1));

EthFrame makeEthFrame(const TxPacket& txPkt, bool skipTtlDecrement = false);

EthFrame makeEthFrame(const TxPacket& txPkt, folly::MacAddress dstMac);

EthFrame
makeEthFrame(const TxPacket& txPkt, folly::MacAddress dstMac, VlanID vlan);
EthHdr makeEthHdr(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    ETHERTYPE etherType);

template <typename SwitchT>
AllocatePktFn makeAllocator(const SwitchT* sw) {
  return [sw](uint32_t size) { return sw->allocatePacket(size); };
}
std::unique_ptr<TxPacket> makeEthTxPacket(
    const AllocatePktFn& allocateTxPkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    std::optional<std::vector<uint8_t>> payload);

std::unique_ptr<facebook::fboss::TxPacket> makeARPTxPacket(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    ARP_OPER type,
    std::optional<folly::MacAddress> targetMac = std::nullopt);

std::unique_ptr<facebook::fboss::TxPacket> makeNeighborSolicitation(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& neighborIp);

std::unique_ptr<facebook::fboss::TxPacket> makeNeighborAdvertisement(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    folly::IPAddressV6 dstIp);

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    AllocatePktFn allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    AllocatePktFn allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint8_t dscp = 0,
    uint8_t ttl = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    AllocatePktFn allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeIpInIpTxPacket(
    const AllocatePktFn& allocatePkt,
    VlanID vlan,
    folly::MacAddress outerSrcMac,
    folly::MacAddress outerDstMac,
    const folly::IPAddressV6& outerSrcIp,
    const folly::IPAddressV6& outerDstIp,
    const folly::IPAddressV6& innerSrcIp,
    const folly::IPAddressV6& innerDstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t outerTrafficClass = 0,
    uint8_t innerTrafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t dscp = 0,
    uint8_t ttl = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV4& srcIp,
    const folly::IPAddressV4& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t dscp = 0,
    uint8_t ttl = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<TxPacket> makeTCPTxPacket(
    const AllocatePktFn& allocatePktSwitch,
    std::optional<VlanID> vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t trafficClass = 0,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

std::unique_ptr<TxPacket> makePTPTxPacket(
    const AllocatePktFn& allocatePktSwitch,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint8_t trafficClass,
    uint8_t hopLimit,
    PTPMessageType ptpPktType);

// Template wrappers to wrap Sw/HwSwitch allocations
template <typename SwitchT>
std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    std::optional<std::vector<uint8_t>> payload = std::nullopt) {
  return makeEthTxPacket(

      makeAllocator(switchT), vlan, srcMac, dstMac, etherType, payload);
}

template <typename SwitchT>
std::unique_ptr<facebook::fboss::TxPacket> makeARPTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    ARP_OPER type,
    std::optional<folly::MacAddress> targetMac = std::nullopt) {
  return makeARPTxPacket(
      makeAllocator(switchT),
      vlan,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      type,
      targetMac);
}

template <typename SwitchT>
std::unique_ptr<facebook::fboss::TxPacket> makeNeighborSolicitation(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& neighborIp) {
  return makeNeighborSolicitation(
      makeAllocator(switchT), vlan, srcMac, srcIp, neighborIp);
}

template <typename SwitchT>
std::unique_ptr<facebook::fboss::TxPacket> makeNeighborAdvertisement(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    folly::IPAddressV6 dstIp) {
  return makeNeighborAdvertisement(
      makeAllocator(switchT), vlan, srcMac, dstMac, srcIp, dstIp);
}

template <typename SwitchT, typename IPAddrT>
std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const IPAddrT& srcIp,
    const IPAddrT& dstIp,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>()) {
  return makeIpTxPacket(
      makeAllocator(switchT),
      vlan,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      trafficClass,
      hopLimit,
      payload);
}

template <typename SwitchT>
std::unique_ptr<facebook::fboss::TxPacket> makeIpInIpTxPacket(
    const SwitchT* switchT,
    VlanID vlan,
    folly::MacAddress outerSrcMac,
    folly::MacAddress outerDstMac,
    const folly::IPAddressV6& outerSrcIp,
    const folly::IPAddressV6& outerDstIp,
    const folly::IPAddressV6& innerSrcIp,
    const folly::IPAddressV6& innerDstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t outerTrafficClass = 0,
    uint8_t innerTrafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>()) {
  return makeIpInIpTxPacket(
      makeAllocator(switchT),
      vlan,
      outerSrcMac,
      outerDstMac,
      outerSrcIp,
      outerDstIp,
      innerSrcIp,
      innerDstIp,
      srcPort,
      dstPort,
      outerTrafficClass,
      innerTrafficClass,
      hopLimit,
      payload);
}

template <typename SwitchT, typename IPAddrT>
std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const IPAddrT& srcIp,
    const IPAddrT& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>()) {
  return makeUDPTxPacket(
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
      payload);
}

template <typename SwitchT>
std::unique_ptr<TxPacket> makePTPTxPacket(
    const SwitchT* switchT,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint8_t trafficClass,
    uint8_t hopLimit,
    PTPMessageType ptpPktType) {
  return makePTPTxPacket(
      makeAllocator(switchT),
      vlan,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      trafficClass,
      hopLimit,
      ptpPktType);
}

template <typename SwitchT, typename IPAddrT>
std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const IPAddrT& srcIp,
    const IPAddrT& dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t trafficClass = 0,
    uint8_t hopLimit = 255,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>()) {
  return makeTCPTxPacket(
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
      payload);
}

template <typename SwitchT>
std::unique_ptr<TxPacket> makeTCPTxPacket(
    const SwitchT* switchT,
    std::optional<VlanID> vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t trafficClass = 0,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>()) {
  return makeTCPTxPacket(
      makeAllocator(switchT),
      vlanId,
      dstMac,
      dstIpAddress,
      l4SrcPort,
      l4DstPort,
      trafficClass,
      payload);
}

} // namespace utility

} // namespace facebook::fboss
