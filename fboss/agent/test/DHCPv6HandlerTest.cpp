/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/DHCPv6Handler.h"
#include <fb303/ServiceData.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <algorithm>
#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/DHCPv6Packet.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using folly::IOBuf;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using std::make_unique;
using std::replace;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using ::testing::_;
using testing::Return;

namespace {

// Address constants to be used to simulate DHCPV6 client and server

// System's MAC address
const MacAddress kPlatformMac("00:02:00:ab:cd:ef");
// DHCP client's MAC Address
const string kClientMacStr{"02 00 00 93 b5 1a"};
const MacAddress kClientMac("02:00:00:93:b5:1a");
// DHCP client's VLAN
const string kClientVlanStr{"00 01"};
const VlanID kClientVlan(1);
const string kVlanInterfaceIPStr(
    "24 01 db 00 21 10 30 01 00 00 00 00 00 00 00 01");
const IPAddressV6 kVlanInterfaceIP("2401:db00:2110:3001::0001");

// DHCP client's link local IPv6 address
const string kDhcpV6ClientLocalIpStr{
    "fe 80 00 00 00 00 00 00 c6 71 fe ff fe 93 b5 1a"};
const IPAddressV6 kDhcpV6ClientLocalIp(
    "fe80:0000:0000:0000:c671:feff:fe93:b51a");

// All router's link local IPv6 address and corresponding MAC address
const string kDhcpV6AllRoutersIpStr{
    "ff 02 00 00 00 00 00 00 00 00 00 00 00 00 01 02"};
const IPAddressV6 kDhcpV6AllRoutersLLIp(
    "ff02:0000:0000:0000:0000:0000:0000:0102");
const MacAddress kDhcpV6AllRoutersMac("33:33:00:00:00:02");

// DHCP Server Address
const MacAddress kRelayNextHopMac("02:00:00:93:b5:1a");
const string kDhcpV6RelayStr{"20 01 0d b8 02 00 00 00 00 00 00 00 00 00 00 02"};
const IPAddressV6 kDhcpV6Relay("2001:0db8:0200:0000:0000:0000:0000:0002");
// DHCP Client Override MAC Address
const MacAddress kClientMacOverride("03:00:00:93:b5:1a");
// DHCP Server Override Address
const IPAddressV6 kDhcpV6RelayOverride(
    "2001:0db8:0200:0000:0000:0000:0000:0003");

// SrcIP to be used for RelayForward pkts
const string kDhcpV6RelaySrcStr{
    "20 01 0d b8 03 00 00 00 00 00 00 00 00 00 00 01"};
const IPAddressV6 kDhcpV6RelaySrc("2001:0db8:0300:0000:0000:0000:0000:0001");
// SrcIP to be used for RelayReply pkts
// Has to match an interface (interface55) IP address
const IPAddressV6 kDhcpV6ReplySrc("2401:db00:2110:3055:0000:0000:0000:0001");

// Function to setup SwState required for the tests
shared_ptr<SwitchState> testState() {
  auto state = testStateA();
  const auto& vlans = state->getVlans();
  // Configure DHCPV6 relay settings for the test VLAN
  vlans->getVlan(VlanID(1))->setDhcpV6Relay(kDhcpV6Relay);
  DhcpV6OverrideMap overrides;
  overrides[kClientMacOverride] = kDhcpV6RelayOverride;
  vlans->getVlan(VlanID(1))->setDhcpV6RelayOverrides(overrides);
  return state;
}
unique_ptr<HwTestHandle> setupTestHandle() {
  return createTestHandle(testState());
}

shared_ptr<SwitchState> testStateNAT() {
  auto state = testState();
  state->setDhcpV6RelaySrc(kDhcpV6RelaySrc);
  state->setDhcpV6ReplySrc(kDhcpV6ReplySrc);
  return state;
}

unique_ptr<HwTestHandle> setupTestHandleNAT() {
  return createTestHandle(testStateNAT());
}

// Generic function to inject a RX DHCPV6 packet to the handler
void sendDHCPV6Packet(
    HwTestHandle* handle,
    string srcMac,
    string dstMac,
    string vlan,
    string srcIp,
    string dstIp,
    string srcPort,
    string dstPort,
    uint32_t dhcpV6HdrSize,
    string dhcpV6Hdr,
    string dhcpV6RequestOptions = "") {
  constexpr auto ethHdrSize = 18; // DMAC + SMAC + VLAN + IPEthType
  constexpr auto ipV6HdrSize = 40;
  constexpr auto udpHdrSize = 8;
  auto payloadSize = dhcpV6HdrSize;
  std::string ipV6HdrPayloadLengthStr =
      folly::sformat("{0:04x}", udpHdrSize + payloadSize);
  std::string udpHdrLengthStr =
      folly::sformat("{0:04x}", udpHdrSize + payloadSize);

  auto buf = make_unique<folly::IOBuf>(PktUtil::parseHexData(
      // Ethernet header: dstMac, srcMac, EthType, vlanId
      dstMac + srcMac + "81 00" + vlan +
      // Ethernet Header: Ether type for IPv6
      "86 dd"
      // IPv6 Header: Version 6, traffic class, flow label
      "6e 00 00 00" +
      // IPV6 Header: IPv6 Payload length
      ipV6HdrPayloadLengthStr +
      // IPV6 Header: Next Header: 17 (UDP), Hop Limit (1)
      "11 01" +
      // IPv6 Header: srcIP, dstIP, srcPort, dstPort
      srcIp + dstIp +
      // UDP Header: srcPort, dstPort, length
      srcPort + dstPort + udpHdrLengthStr +
      // UDP Header: checksum (not valid)
      "2a 7e" +
      // DHCPv6 Header: msgType, txnId
      dhcpV6Hdr +
      // DHCPv6 Header: Options
      dhcpV6RequestOptions));

  // Validate the expected pkt length to actual pkt length
  ASSERT_EQ(payloadSize + udpHdrSize + ipV6HdrSize + ethHdrSize, buf->length())
      << "Don't forget to adjust the headers' length";
  // Inject the packet to the receive handler
  handle->rxPacket(std::move(buf), PortID(1), VlanID(1));
}

typedef std::function<void(Cursor* cursor, uint32_t length)>
    DHCPV6PayloadCheckFn;

// Generic validator function to validate expected packet from the DHCPV6
// handler
TxMatchFn checkDHCPV6Pkt(
    MacAddress dstMac,
    MacAddress srcMac,
    VlanID vlan,
    IPAddressV6 srcIp,
    IPAddressV6 dstIp,
    uint16_t srcPort,
    uint16_t dstPort,
    const DHCPV6PayloadCheckFn& dhcpV6CheckPayload) {
  return [=](const TxPacket* txPacket) {
    const auto* buf = txPacket->buf();
    Cursor c(buf);

    // Validate Ethernet header fields
    auto parsedDstMac = PktUtil::readMac(&c);
    checkField(dstMac, parsedDstMac, "dst mac");
    auto parsedSrcMac = PktUtil::readMac(&c);
    checkField(srcMac, parsedSrcMac, "src mac");
    auto vlanType = c.readBE<uint16_t>();
    checkField(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN),
        vlanType,
        "VLAN ethertype");
    auto vlanTag = c.readBE<uint16_t>();
    checkField(static_cast<uint16_t>(vlan), vlanTag, "VLAN tag");
    auto ethertype = c.readBE<uint16_t>();
    checkField(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
        ethertype,
        "ethertype");

    // Validate IPv6 header fields
    IPv6Hdr ipv6(c);
    checkField(
        static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
        ipv6.nextHeader,
        "IPv6 protocol");
    checkField(srcIp, ipv6.srcAddr, "src IP");
    checkField(dstIp, ipv6.dstAddr, "dst IP");

    Cursor ipv6PayloadStart(c);

    // Validate UDP header fields
    SwitchStats switchStats;
    PortStats portStats(PortID(0), "foo", &switchStats);
    UDPHeader udp;
    udp.parse(&c, &portStats);
    checkField(udp.computeChecksum(ipv6, c), udp.csum, "UDP checksum");
    checkField(srcPort, udp.srcPort, "UDP srcPort");
    checkField(dstPort, udp.dstPort, "UDP dstPort");

    // Validate DHCPv6 payload fields
    dhcpV6CheckPayload(&c, ipv6.payloadLength - UDPHeader::size());

    // Validate packet length
    if (ipv6.payloadLength != (c - ipv6PayloadStart)) {
      throw FbossError(
          "IPv6 payload length mismatch: header says ",
          ipv6.payloadLength,
          " but we used ",
          c - ipv6PayloadStart);
    }

    // This is a match
    return;
  };
}

} // unnamed   namespace
