/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <memory>
#include <optional>
#include <random>
#include <type_traits>
#include <vector>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/ArpHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/NDP.h"
#include "fboss/agent/packet/PTPHeader.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwSwitch;
class SwitchState;
class EthHdr;
class IPv4Hdr;
class IPv6Hdr;
class TCPHeader;
class UDPHeader;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf);
folly::MacAddress getFirstInterfaceMac(const cfg::SwitchConfig& cfg);
folly::MacAddress getFirstInterfaceMac(std::shared_ptr<SwitchState> state);
std::optional<VlanID> firstVlanID(const cfg::SwitchConfig& cfg);
std::optional<VlanID> firstVlanID(const std::shared_ptr<SwitchState>& state);

std::unique_ptr<facebook::fboss::TxPacket> makeEthTxPacket(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    std::optional<std::vector<uint8_t>> payload =
        std::optional<std::vector<uint8_t>>());

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeUDPTxPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const UDPHeader& udpHdr,
    const std::vector<uint8_t>& payload);

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeIpPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const std::vector<uint8_t>& payload);

std::unique_ptr<facebook::fboss::TxPacket> makeIpTxPacket(
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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

template <typename IPHDR>
std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const HwSwitch* hw,
    const EthHdr& ethHdr,
    const IPHDR& ipHdr,
    const TCPHeader& udpHdr,
    const std::vector<uint8_t>& payload);

std::unique_ptr<facebook::fboss::TxPacket> makeTCPTxPacket(
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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
    const HwSwitch* hw,
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

std::unique_ptr<facebook::fboss::TxPacket> makeARPTxPacket(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    ARP_OPER type,
    std::optional<folly::MacAddress> targetMac = std::nullopt);

std::unique_ptr<facebook::fboss::TxPacket> makeNeighborSolicitation(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& neighborIp);

std::unique_ptr<facebook::fboss::TxPacket> makeNeighborAdvertisement(
    const HwSwitch* hw,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    folly::IPAddressV6 dstIp);

std::unique_ptr<facebook::fboss::TxPacket> makeLLDPPacket(
    const HwSwitch* hw,
    const folly::MacAddress srcMac,
    VlanID vlanid,
    const std::string& systemdescr,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities);

void sendTcpPkts(
    facebook::fboss::HwSwitch* hwSwitch,
    int numPktsToSend,
    std::optional<VlanID> vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    PortID outPort,
    uint8_t trafficClass = 0,
    std::optional<std::vector<uint8_t>> payload = std::nullopt);

std::unique_ptr<facebook::fboss::TxPacket> makePTPTxPacket(
    const HwSwitch* hw,
    VlanID vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& srcIp,
    const folly::IPAddressV6& dstIp,
    uint8_t trafficClass,
    uint8_t hopLimit,
    PTPMessageType ptpPktType);

bool isPtpEventPacket(folly::io::Cursor& cursor);
uint8_t getIpHopLimit(folly::io::Cursor& cursor);

struct RandomNumberGenerator {
  RandomNumberGenerator(int seed, uint64_t begin, uint64_t end)
      : generator(seed), distibution(begin, end) {}
  auto operator()() {
    return distibution(generator);
  }

  std::mt19937_64 generator;
  std::uniform_int_distribution<> distibution;
};

} // namespace facebook::fboss::utility
