/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <algorithm>
#include <string>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/DHCPv4Packet.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/cast.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::make_shared;
using folly::io::Cursor;
using std::unique_ptr;
using std::string;

using ::testing::_;
using testing::Return;

namespace {

const MacAddress kPlatformMac("02:01:02:03:04:05");
const IPAddressV4 kVlanInterfaceIP("10.0.0.1");

namespace {

unique_ptr<SwSwitch> setupSwitch() {
  // Setup a default state object
  auto state = testStateA();
  const auto& vlans = state->getVlans();
  // Set up an arp response entry for VLAN 1, 10.0.0.1,
  // so that we can detect the packet to 10.0.0.1 is for myself
  auto respTable1 = make_shared<ArpResponseTable>();
  respTable1->setEntry(kVlanInterfaceIP, kPlatformMac, InterfaceID(1));
  vlans->getVlan(VlanID(1))->setArpResponseTable(respTable1);
  return createMockSw(state);
}

} // unnamed namespace

typedef std::function<void(Cursor* cursor, uint32_t length)> PayloadCheckFn;


TxMatchFn checkICMPv4Pkt(MacAddress srcMac, IPAddressV4 srcIP,
                         MacAddress dstMac, IPAddressV4 dstIP,
                         VlanID vlan, ICMPv4Type type, ICMPv4Code code,
                         const PayloadCheckFn& checkPayload) {
  return [=](const TxPacket* pkt) {
    Cursor c(pkt->buf());
    auto parsedDstMac = PktUtil::readMac(&c);
    checkField(dstMac, parsedDstMac, "dst mac");
    auto parsedSrcMac = PktUtil::readMac(&c);
    checkField(srcMac, parsedSrcMac, "src mac");
    auto vlanType = c.readBE<uint16_t>();
    checkField(ETHERTYPE_VLAN, vlanType, "VLAN ethertype");
    auto vlanTag = c.readBE<uint16_t>();
    checkField(static_cast<uint16_t>(vlan), vlanTag, "VLAN tag");
    auto ethertype = c.readBE<uint16_t>();
    checkField(ETHERTYPE_IPV4, ethertype, "ethertype");

    Cursor ipv4HdrStart(c);
    IPv4Hdr ipv4(c);
    checkField(IP_PROTO_ICMP, ipv4.protocol, "IPv4 protocol");
    checkField(srcIP, ipv4.srcAddr, "src IP");
    checkField(dstIP, ipv4.dstAddr, "dst IP");

    ICMPHdr icmp4(c);
    checkField(icmp4.computeChecksum(c, c.length()), icmp4.csum,
        "ICMPv4 checksum");
    checkField(type, icmp4.type, "ICMPv4 type");
    checkField(code, icmp4.code, "ICMPv4 code");

    checkPayload(&c, ipv4.length - IPv4Hdr::minSize() - ICMPHdr::SIZE);

    if (ipv4.length != (c - ipv4HdrStart)) {
      throw FbossError("IPv4 payload length mismatch: header says ",
                       ipv4.length, " but we used ",
                       c - ipv4HdrStart);
    }

    // This is a match
    return;
  };
}

TxMatchFn checkICMPv6Pkt(MacAddress srcMac, IPAddressV6 srcIP,
                         MacAddress dstMac, IPAddressV6 dstIP,
                         VlanID vlan, ICMPv6Type type, ICMPv6Code code,
                         const PayloadCheckFn& checkPayload) {
  return [=](const TxPacket* pkt) {
    Cursor c(pkt->buf());
    auto parsedDstMac = PktUtil::readMac(&c);
    checkField(dstMac, parsedDstMac, "dst mac");
    auto parsedSrcMac = PktUtil::readMac(&c);
    checkField(srcMac, parsedSrcMac, "src mac");
    auto vlanType = c.readBE<uint16_t>();
    checkField(ETHERTYPE_VLAN, vlanType, "VLAN ethertype");
    auto vlanTag = c.readBE<uint16_t>();
    checkField(static_cast<uint16_t>(vlan), vlanTag, "VLAN tag");
    auto ethertype = c.readBE<uint16_t>();
    checkField(ETHERTYPE_IPV6, ethertype, "ethertype");
    IPv6Hdr ipv6(c);
    checkField(IP_PROTO_IPV6_ICMP, ipv6.nextHeader, "IPv6 protocol");
    checkField(srcIP, ipv6.srcAddr, "src IP");
    checkField(dstIP, ipv6.dstAddr, "dst IP");

    Cursor ipv6PayloadStart(c);
    ICMPHdr icmp6(c);
    checkField(icmp6.computeChecksum(ipv6, c), icmp6.csum, "ICMPv6 checksum");
    checkField(type, icmp6.type, "ICMPv6 type");
    checkField(code, icmp6.code, "ICMPv6 code");

    checkPayload(&c, ipv6.payloadLength - ICMPHdr::SIZE);

    if (ipv6.payloadLength != (c - ipv6PayloadStart)) {
      throw FbossError("IPv6 payload length mismatch: header says ",
                       ipv6.payloadLength, " but we used ",
                       c - ipv6PayloadStart);
    }

    // This is a match
    return;
  };
}

TxMatchFn checkICMPv4TTLExceeded(MacAddress srcMac, IPAddressV4 srcIP,
                              MacAddress dstMac, IPAddressV4 dstIP,
                              VlanID vlan, const uint8_t* payloadData,
                              size_t payloadLengthLimit) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    if (length > payloadLengthLimit) {
      throw FbossError("ICMPv4 TTL Exceeded payload too long, cursor length ",
                       length, "but payloadLengthLimit is ",
                       payloadLengthLimit);
    }
    const uint8_t* cursorData = cursor->data();
    for(int i = 0; i < length; i++) {
      EXPECT_EQ(cursorData[i], payloadData[i]);
    }
    cursor->skip(length);
    // This is a match
    return;
  };
  return checkICMPv4Pkt(srcMac, srcIP, dstMac, dstIP, vlan,
                        ICMPV4_TYPE_TIME_EXCEEDED,
                        ICMPV4_CODE_TIME_EXCEEDED_TTL_EXCEEDED,
                        checkPayload);
}

TxMatchFn checkICMPv6TTLExceeded(MacAddress srcMac, IPAddressV6 srcIP,
                              MacAddress dstMac, IPAddressV6 dstIP,
                              VlanID vlan, const uint8_t* payloadData,
                              size_t payloadLengthLimit) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    if (length > payloadLengthLimit) {
      throw FbossError("ICMPv4 TTL Exceeded payload too long, cursor length ",
                       length, "but payloadLengthLimit is ",
                       payloadLengthLimit);
    }
    const uint8_t* cursorData = cursor->data();
    for(int i = 0; i < length; i++) {
      EXPECT_EQ(cursorData[i], payloadData[i]);
    }
    cursor->skip(length);
    // This is a match
    return;
  };
  return checkICMPv6Pkt(srcMac, srcIP, dstMac, dstIP, vlan,
                        ICMPV6_TYPE_TIME_EXCEEDED,
                        ICMPV6_CODE_TIME_EXCEEDED_HOPLIMIT_EXCEEDED,
                        checkPayload);
}

TEST(ICMPTest, TTLExceededV4) {
  auto sw = setupSwitch();
  PortID portID(1);
  VlanID vlanID(1);

  // create an IPv4 packet with TTL = 1
  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "00 02 00 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(5), DSCP(7), ECN(1), Total Length(20)
    "45 1d 00 14"
    // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
    "34 56 53 45"
    // TTL(1), Protocol(11), Checksum (0x1234, fake)
    "01 11 12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.1.0.10)
    "0a 01 00 0a"
    // Source port (69), destination port (70)
    "00 45 00 46"
    // Length (8), checksum (0x1234, faked)
    "00 08 12 34"
  );

  // a copy of origin packet to comparison (only IP packet included)
  auto icmpPayload = MockRxPacket::fromHex(
    // icmp padding for unused field
    "00 00 00 00"
    // Version(4), IHL(5), DSCP(7), ECN(1), Total Length(20)
    "45 1d 00 14"
    // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
    "34 56 53 45"
    // TTL(1), Protocol(11), Checksum (0x1234, fake)
    "01 11 12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.1.0.10)
    "0a 01 00 0a"
    // Source port (69), destination port (70)
    "00 45 00 46"
    // Length (8), checksum (0x1234, faked)
    "00 08 12 34"
  );

  pkt->padToLength(68);
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  // Cache the current stats
  CounterCache counters(sw.get());

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PLATFORM_CALL(sw, getLocalMac()).
    WillRepeatedly(Return(kPlatformMac));

  // We should get a ICMPv4 TTL exceeded back
  EXPECT_PKT(sw, "ICMP TTL Exceeded",
             checkICMPv4TTLExceeded(kPlatformMac,
                                 IPAddressV4("10.0.0.1"),
                                 kPlatformMac,
                                 IPAddressV4("1.2.3.4"),
                                 VlanID(1), icmpPayload->buf()->data(), 32));

  sw->packetReceived(pkt->clone());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.ttl_exceeded.sum", 1);
}

// Force ICMP TTL expiration to test that serialize(unserialize(x)) = x
// even if we have extra IPv4 options
TEST(ICMPTest, TTLExceededV4IPExtraOptions) {
  auto sw = setupSwitch();
  PortID portID(1);
  VlanID vlanID(1);

  // TODO: If actual parsing of IPv4 options is implemented at some point
  //       the bogus option bytes will need to be replaced with valid options

  // create an IPv4 packet with TTL = 1 and ihl != 5
  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "00 02 00 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(15), DSCP(7), ECN(1), Total Length(60) <------ N.B. IHL
    "4f 1d 00 3c"
    // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
    "34 56 53 45"
    // TTL(1), Protocol(11), Checksum (0x1234, fake)
    "01 11 12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.1.0.10)
    "0a 01 00 0a"
    // Maximum amount of options, bogus values here
    "11 22 33 44 11 22 33 44 11 22 33 44 11 22 33 44 11 22 33 44"
    "aa bb cc dd aa bb cc dd aa bb cc dd aa bb cc dd aa bb cc dd"
    // Source port (69), destination port (70)
    "00 45 00 46"
    // Length (8), checksum (0x1234, faked)
    "00 08 12 34"
  );

  // a copy of origin packet to comparison (only IP packet included)
  auto icmpPayload = MockRxPacket::fromHex(
    // icmp padding for unused field
    "00 00 00 00"
    // Version(4), IHL(15), DSCP(7), ECN(1), Total Length(60) <------ N.B. IHL
    "4f 1d 00 3c"
    // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
    "34 56 53 45"
    // TTL(1), Protocol(11), Checksum (0x1234, fake)
    "01 11 12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.1.0.10)
    "0a 01 00 0a"
    // Maximum amount of options, bogus values here
    "11 22 33 44 11 22 33 44 11 22 33 44 11 22 33 44 11 22 33 44"
    "aa bb cc dd aa bb cc dd aa bb cc dd aa bb cc dd aa bb cc dd"
    // Source port (69), destination port (70)
    "00 45 00 46"
    // Length (8), checksum (0x1234, faked)
    "00 08 12 34"

  );

  pkt->padToLength(108);
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PLATFORM_CALL(sw, getLocalMac()).
    WillRepeatedly(Return(kPlatformMac));

  // We should get a ICMPv4 TTL exceeded back
  EXPECT_PKT(sw, "ICMP TTL Exceeded",
             checkICMPv4TTLExceeded(kPlatformMac,
                                 IPAddressV4("10.0.0.1"),
                                 kPlatformMac,
                                 IPAddressV4("1.2.3.4"),
                                 VlanID(1), icmpPayload->buf()->data(), 72));

  sw->packetReceived(pkt->clone());
}

TEST(ICMPTest, ExtraFrameCheckSequenceAtEnd) {
  auto sw = setupSwitch();
  PortID portID(1);
  VlanID vlanID(1);

  std::string msg =
      // Destination mac
      "00 02 c9 bc 26 f0"
      // Source mac
      "00 02 c9 bb 5e 0e"
      // VLAN type
      "81 00"
      // Unused
      "07 dd"
      // Ethertype
      "86 dd"
      // Version, Traffic class, Flow label
      "60 00 00 00"
      // Payload length (2), next header (1), hop limit (1)
      "00 18 3a ff"
      // Src IPv6 address
      "fe 80 00 00 00 00 00 00 02 02 c9 ff fe bb 5e 0e"
      // Dst IPv6 address
      "fe 80 00 00 00 00 00 00 02 02 c9 ff fe bc 26 f0"
      // ICMPv6 type (1) / code (1) / checksum (2).
      // 0x88 = Neighbor Advertisement
      "88 00 f8 e2"
      // flags for ICMPv6 (4)
      "40 00 00 00"
      // IPv6 address of the source
      "fe 80 00 00 00 00 00 00 02 02 c9 ff fe bb 5e 0e";
  std::string checksum =
      // Ethernet frame check sequence
      "45 5f 16 58";
  auto pktWithoutChecksum = MockRxPacket::fromHex(msg);
  auto pktWithChecksum = MockRxPacket::fromHex(msg + checksum);

  pktWithoutChecksum->setSrcPort(portID);
  pktWithoutChecksum->setSrcVlan(vlanID);

  pktWithChecksum->setSrcPort(portID);
  pktWithChecksum->setSrcVlan(vlanID);

  EXPECT_THROW(
      sw->packetReceivedThrowExceptionOnError(pktWithChecksum->clone()),
      std::out_of_range);
  EXPECT_NO_THROW(
      sw->packetReceivedThrowExceptionOnError(pktWithoutChecksum->clone()));
}

TEST(ICMPTest, TTLExceededV6) {
  auto sw = setupSwitch();
  PortID portID(1);
  VlanID vlanID(1);

  // a testing IPv6 packet with hop limit 1
  auto pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "33 33 ff 00 00 0a  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 58 (ICMPv6), Hop Limit (1)
      "3a 01"
      // src addr (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a"
      // dst addr (ff02::1:ff00:000a)
      "ff 02 00 00 00 00 00 00 00 00 00 01 ff 00 00 0a"
      // type: neighbor solicitation
      "87"
      // code
      "00"
      // checksum
      "2a 7e"
      // reserved
      "00 00 00 00"
      // target address (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a");

  // keep a copy of the ip packet for icmp payload comparison
  auto icmp6Payload = MockRxPacket::fromHex(
      // icmp6 padding for unused field
      "00 00 00 00"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 58 (ICMPv6), Hop Limit (1)
      "3a 01"
      // src addr (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a"
      // dst addr (ff02::1:ff00:000a)
      "ff 02 00 00 00 00 00 00 00 00 00 01 ff 00 00 0a"
      // type: neighbor solicitation
      "87"
      // code
      "00"
      // checksum
      "2a 7e"
      // reserved
      "00 00 00 00"
      // target address (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a");

  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);
  // Cache the current stats
  CounterCache counters(sw.get());

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PLATFORM_CALL(sw, getLocalMac()).
    WillRepeatedly(Return(kPlatformMac));

  // We should get a ICMPv6 TTL exceeded back
  EXPECT_PKT(sw, "ICMP TTL Exceeded",
             checkICMPv6TTLExceeded(kPlatformMac,
                                 IPAddressV6("2401:db00:2110:3001::0001"),
                                 kPlatformMac,
                                 IPAddressV6("2401:db00:2110:3004::a"),
                                 VlanID(1), icmp6Payload->buf()->data(), 68));

  sw->packetReceived(pkt->clone());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv6.hop_exceeded.sum", 1);
}


} // namespace
