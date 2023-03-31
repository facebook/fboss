/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossError.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/DHCPv4Packet.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/cast.hpp>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using std::make_shared;
using std::string;
using std::unique_ptr;

using ::testing::_;
using testing::Return;

namespace {

const IPAddressV4 kVlanInterfaceIP("10.0.0.1");

unique_ptr<HwTestHandle> setupTestHandle(bool noIpv4VlanIntf = false) {
  // Setup a default state object
  auto state =
      noIpv4VlanIntf ? testStateAWithoutIpv4VlanIntf(VlanID(1)) : testStateA();
  const auto& vlans = state->getVlans();
  // Set up an arp response entry for VLAN 1, 10.0.0.1,
  // so that we can detect the packet to 10.0.0.1 is for myself
  auto respTable1 = make_shared<ArpResponseTable>();
  respTable1->setEntry(
      kVlanInterfaceIP, MockPlatform::getMockLocalMac(), InterfaceID(1));
  vlans->getVlan(VlanID(1))->setArpResponseTable(respTable1);
  return createTestHandle(state);
}

std::string genICMPv6EchoRequest(int hopLimit, size_t payloadSize) {
  auto hopLimitStr = folly::sformat("{0:02x}", hopLimit);
  auto payloadLenStr = folly::sformat("{0:04x}", payloadSize + 8);
  auto payload = std::string(payloadSize * 2, 'f');
  return std::string(
      // Version 6, traffic class, flow label
      "60 00 00 00" +
      // Payload length
      payloadLenStr +
      // Next Header: 58 (ICMPv6), Hop Limit (1)
      "3a " + hopLimitStr +
      // src addr (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a"
      // dst addr (fe02::1:ff00:000a) (Non multicast address)
      "fe 02 00 00 00 00 00 00 00 00 00 01 ff 00 00 0a"
      // type: echo request
      "80"
      // code
      "00"
      // checksum (faked)
      "42 42"
      // identifier(2004), sequence (01)
      "20 04 00 01" +
      payload);
}

typedef std::function<void(Cursor* cursor, uint32_t length)> PayloadCheckFn;

TxMatchFn checkICMPv4Pkt(
    MacAddress srcMac,
    IPAddressV4 srcIP,
    MacAddress dstMac,
    IPAddressV4 dstIP,
    VlanID vlan,
    ICMPv4Type type,
    ICMPv4Code code,
    const PayloadCheckFn& checkPayload) {
  return [=](const TxPacket* pkt) {
    Cursor c(pkt->buf());
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
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4),
        ethertype,
        "ethertype");

    Cursor ipv4HdrStart(c);
    IPv4Hdr ipv4(c);
    checkField(
        static_cast<uint8_t>(IP_PROTO::IP_PROTO_ICMP),
        ipv4.protocol,
        "IPv4 protocol");
    checkField(srcIP, ipv4.srcAddr, "src IP");
    checkField(dstIP, ipv4.dstAddr, "dst IP");

    ICMPHdr icmp4(c);
    checkField(
        icmp4.computeChecksum(c, c.length()), icmp4.csum, "ICMPv4 checksum");
    checkField(static_cast<uint8_t>(type), icmp4.type, "ICMPv4 type");
    checkField(static_cast<uint8_t>(code), icmp4.code, "ICMPv4 code");

    checkPayload(&c, ipv4.length - IPv4Hdr::minSize() - ICMPHdr::SIZE);

    if (ipv4.length != (c - ipv4HdrStart)) {
      throw FbossError(
          "IPv4 payload length mismatch: header says ",
          ipv4.length,
          " but we used ",
          c - ipv4HdrStart);
    }

    // This is a match
    return;
  };
}

TxMatchFn checkICMPv6Pkt(
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    VlanID vlan,
    ICMPv6Type type,
    ICMPv6Code code,
    const PayloadCheckFn& checkPayload) {
  return [=](const TxPacket* pkt) {
    Cursor c(pkt->buf());
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
    IPv6Hdr ipv6(c);
    checkField(
        static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP),
        ipv6.nextHeader,
        "IPv6 protocol");
    checkField(srcIP, ipv6.srcAddr, "src IP");
    checkField(dstIP, ipv6.dstAddr, "dst IP");

    Cursor ipv6PayloadStart(c);
    ICMPHdr icmp6(c);
    checkField(icmp6.computeChecksum(ipv6, c), icmp6.csum, "ICMPv6 checksum");
    checkField(static_cast<uint8_t>(type), icmp6.type, "ICMPv6 type");
    checkField(static_cast<uint8_t>(code), icmp6.code, "ICMPv6 code");

    checkPayload(&c, ipv6.payloadLength - ICMPHdr::SIZE);

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

TxMatchFn checkICMPv4TTLExceeded(
    MacAddress srcMac,
    IPAddressV4 srcIP,
    MacAddress dstMac,
    IPAddressV4 dstIP,
    VlanID vlan,
    const uint8_t* payloadData,
    size_t payloadLengthLimit) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    if (length > payloadLengthLimit) {
      throw FbossError(
          "ICMPv4 TTL Exceeded payload too long, cursor length ",
          length,
          "but payloadLengthLimit is ",
          payloadLengthLimit);
    }
    const uint8_t* cursorData = cursor->data();
    for (int i = 0; i < length; i++) {
      EXPECT_EQ(cursorData[i], payloadData[i]);
    }
    cursor->skip(length);
    // This is a match
    return;
  };
  return checkICMPv4Pkt(
      srcMac,
      srcIP,
      dstMac,
      dstIP,
      vlan,
      ICMPv4Type::ICMPV4_TYPE_TIME_EXCEEDED,
      ICMPv4Code::ICMPV4_CODE_TIME_EXCEEDED_TTL_EXCEEDED,
      checkPayload);
}

TxMatchFn checkICMPv6TTLExceeded(
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    VlanID vlan,
    const uint8_t* payloadData,
    size_t payloadLengthLimit) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    if (length > payloadLengthLimit) {
      throw FbossError(
          "ICMPv6 TTL Exceeded payload too long, cursor length ",
          length,
          "but payloadLengthLimit is ",
          payloadLengthLimit);
    }
    const uint8_t* cursorData = cursor->data();
    for (int i = 0; i < length; i++) {
      EXPECT_EQ(cursorData[i], payloadData[i]);
    }
    cursor->skip(length);
    // This is a match
    return;
  };
  return checkICMPv6Pkt(
      srcMac,
      srcIP,
      dstMac,
      dstIP,
      vlan,
      ICMPv6Type::ICMPV6_TYPE_TIME_EXCEEDED,
      ICMPv6Code::ICMPV6_CODE_TIME_EXCEEDED_HOPLIMIT_EXCEEDED,
      checkPayload);
}

TxMatchFn checkICMPv6PacketTooBig(
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    VlanID vlan,
    const uint8_t* payloadData,
    size_t payloadLengthLimit) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    if (length > payloadLengthLimit) {
      throw FbossError(
          "ICMPv6 Packet Too Big payload length too long, cursor length ",
          length,
          " but payloadLengthLimit is ",
          payloadLengthLimit);
    }
    EXPECT_EQ(length, payloadLengthLimit);
    const uint8_t* cursorData = cursor->data();
    for (int i = 0; i < length; i++) {
      EXPECT_EQ(cursorData[i], payloadData[i]);
    }
    cursor->skip(length);
    return;
  };

  return checkICMPv6Pkt(
      srcMac,
      srcIP,
      dstMac,
      dstIP,
      vlan,
      ICMPv6Type::ICMPV6_TYPE_PACKET_TOO_BIG,
      ICMPv6Code::ICMPV6_CODE_PACKET_TOO_BIG,
      checkPayload);
}
} // unnamed namespace

TEST(ICMPTest, TTLExceededV4) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  PortID portID(1);
  VlanID vlanID(1);

  const std::string ipHdr =
      // Version(4), IHL(5), DSCP(7), ECN(1), Total Length(20+8+8=36)
      "45 1d 00 24"
      // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
      "34 56 53 45"
      // TTL(1), Protocol(11), Checksum (0x1234, fake)
      "01 11 12 34"
      // Source IP (1.2.3.4)
      "01 02 03 04"
      // Destination IP (10.1.0.10)
      "0a 01 00 0a";

  const std::string udpHdr =
      // UDP
      // Source port (69), destination port (70)
      "00 45 00 46"
      // Length (16), checksum (0x1234, faked)
      "00 10 12 34";

  const std::string payload = "01 02 03 04 05 06 07 08";

  // create an IPv4 packet with TTL = 1
  auto pkt = PktUtil::parseHexData(
      // dst mac, src mac
      "00 02 00 00 00 01  02 00 02 01 02 03"
      // 802.1q, VLAN 1
      "81 00 00 01"
      // IPv4
      "08 00" +
      ipHdr + udpHdr + payload);

  auto icmpPayload = PktUtil::parseHexData(
      // icmp padding for unused field
      "00 00 00 00" + ipHdr + udpHdr + payload);

  // Cache the current stats
  CounterCache counters(sw);

  EXPECT_HW_CALL(sw, stateChangedImpl(_)).Times(0);

  // We should get a ICMPv4 TTL exceeded back
  EXPECT_SWITCHED_PKT(
      sw,
      "ICMP TTL Exceeded",
      checkICMPv4TTLExceeded(
          MockPlatform::getMockLocalMac(),
          IPAddressV4("10.0.0.1"),
          MockPlatform::getMockLocalMac(),
          IPAddressV4("1.2.3.4"),
          VlanID(1),
          icmpPayload.data(),
          32));

  handle->rxPacket(std::make_unique<folly::IOBuf>(pkt), portID, vlanID);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.ttl_exceeded.sum", 1);
}

// Force ICMP TTL expiration to test that serialize(unserialize(x)) = x
// even if we have extra IPv4 options
TEST(ICMPTest, TTLExceededV4IPExtraOptions) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  PortID portID(1);
  VlanID vlanID(1);

  // TODO: If actual parsing of IPv4 options is implemented at some point
  //       the bogus option bytes will need to be replaced with valid options
  //
  const std::string ethHdr =
      // dst mac, src mac
      "00 02 00 00 00 01  02 00 02 01 02 03"
      // 802.1q, VLAN 1
      "81 00 00 01"
      // IPv4
      "08 00";

  // create an IPv4 packet with TTL = 1 and ihl != 5
  const std::string ipHdr =
      // Version(4), IHL(15), DSCP(7), ECN(1), Total Length(60 + 8 + * = 76)
      "4f 1d 00 4c"
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
      "aa bb cc dd aa bb cc dd aa bb cc dd aa bb cc dd aa bb cc dd";

  const std::string udpHdr =
      // UDP
      // Source port (69), destination port (70)
      "00 45 00 46"
      // Length (16), checksum (0x1234, faked)
      "00 10 12 34";

  const std::string payload = "01 02 03 04 05 06 07 08";

  auto pkt = PktUtil::parseHexData(ethHdr + ipHdr + udpHdr + payload);
  auto icmpPayload = PktUtil::parseHexData(
      // icmp padding for unused field
      "00 00 00 00" + ipHdr + udpHdr + payload);

  EXPECT_HW_CALL(sw, stateChangedImpl(_)).Times(0);

  // We should get a ICMPv4 TTL exceeded back
  EXPECT_SWITCHED_PKT(
      sw,
      "ICMP TTL Exceeded",
      checkICMPv4TTLExceeded(
          MockPlatform::getMockLocalMac(),
          IPAddressV4("10.0.0.1"),
          MockPlatform::getMockLocalMac(),
          IPAddressV4("1.2.3.4"),
          VlanID(1),
          icmpPayload.data(),
          72));

  handle->rxPacket(std::make_unique<folly::IOBuf>(pkt), portID, vlanID);
}

TEST(ICMPTest, ExtraFrameCheckSequenceAtEnd) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  PortID portID(1);
  VlanID vlanID(1);

  std::string msg =
      // Destination mac
      "00 02 00 00 00 01"
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
      "24 01 db 00 21 10 30 01 00 00 00 00 00 00 00 01"
      // ICMPv6 type (1) / code (1) / checksum (2).
      // 0x88 = Neighbor Advertisement
      "88 00 98 fe"
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

  EXPECT_NO_THROW(
      sw->packetReceivedThrowExceptionOnError(std::move(pktWithChecksum)));
  EXPECT_NO_THROW(
      sw->packetReceivedThrowExceptionOnError(std::move(pktWithoutChecksum)));
}

TEST(ICMPTest, TTLExceededV4WithoutV4Interface) {
  auto handle = setupTestHandle(true);
  auto sw = handle->getSw();

  PortID portID(1);
  VlanID vlanID(1);

  const std::string ipHdr =
      // Version(4), IHL(5), DSCP(7), ECN(1), Total Length(20+8+8=36)
      "45 1d 00 24"
      // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
      "34 56 53 45"
      // TTL(1), Protocol(11), Checksum (0x1234, fake)
      "01 11 12 34"
      // Source IP (1.2.3.4)
      "01 02 03 04"
      // Destination IP (10.1.0.10)
      "0a 01 00 0a";

  const std::string udpHdr =
      // UDP
      // Source port (69), destination port (70)
      "00 45 00 46"
      // Length (16), checksum (0x1234, faked)
      "00 10 12 34";

  const std::string payload = "01 02 03 04 05 06 07 08";

  // create an IPv4 packet with TTL = 1
  auto pkt = PktUtil::parseHexData(
      // dst mac, src mac
      "00 02 00 00 00 01  02 00 02 01 02 03"
      // 802.1q, VLAN 1
      "81 00 00 01"
      // IPv4
      "08 00" +
      ipHdr + udpHdr + payload);

  auto icmpPayload = PktUtil::parseHexData(
      // icmp padding for unused field
      "00 00 00 00" + ipHdr + udpHdr + payload);

  // Cache the current stats
  CounterCache counters(sw);

  EXPECT_HW_CALL(sw, stateChangedImpl(_)).Times(0);

  // We should get a ICMPv4 TTL exceeded back with src IP set to
  // VLAN55 IPv4 addres. Because VLAN 1 does not have an IPv4 address.
  EXPECT_SWITCHED_PKT(
      sw,
      "ICMP TTL Exceeded",
      checkICMPv4TTLExceeded(
          MockPlatform::getMockLocalMac(),
          IPAddressV4("10.0.55.1"),
          MockPlatform::getMockLocalMac(),
          IPAddressV4("1.2.3.4"),
          VlanID(1),
          icmpPayload.data(),
          32));

  handle->rxPacket(std::make_unique<folly::IOBuf>(pkt), portID, vlanID);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.ttl_exceeded.sum", 1);
}

void runTTLExceededV6Test(size_t requestedPayloadSize) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  PortID portID(1);
  VlanID vlanID(1);

  auto icmp6Pkt = genICMPv6EchoRequest(1, requestedPayloadSize);
  constexpr size_t kMaxPayloadSize = IPv6Handler::IPV6_MIN_MTU - IPv6Hdr::SIZE -
      ICMPHdr::SIZE - // IPv6 ICMP TTL Exceed
      IPv6Hdr::SIZE - ICMPHdr::SIZE; // IPv6 ICMP Echo Request
  size_t expectedPayloadSize = std::min(requestedPayloadSize, kMaxPayloadSize);

  auto pkt = PktUtil::parseHexData(
      // dst mac, src mac
      "33 33 ff 00 00 0a  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd" +
      icmp6Pkt);
  auto expectedPayload = PktUtil::parseHexData(
      "00 00 00 00" + // icmp6 padding for unused field
      icmp6Pkt);
  expectedPayload.trimEnd(requestedPayloadSize - expectedPayloadSize);

  // Cache the current stats
  CounterCache counters(sw);

  EXPECT_HW_CALL(sw, stateChangedImpl(_)).Times(0);

  // We should get a ICMPv6 TTL exceeded back
  EXPECT_SWITCHED_PKT(
      sw,
      "ICMP TTL Exceeded",
      checkICMPv6TTLExceeded(
          MockPlatform::getMockLocalMac(),
          IPAddressV6("2401:db00:2110:3001::0001"),
          MockPlatform::getMockLocalMac(),
          IPAddressV6("2401:db00:2110:3004::a"),
          VlanID(1),
          expectedPayload.data(),
          expectedPayload.length()));

  handle->rxPacket(std::make_unique<folly::IOBuf>(pkt), portID, vlanID);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv6.hop_exceeded.sum", 1);
}

TEST(ICMPTest, TTLExceededV6) {
  runTTLExceededV6Test(8);
}

TEST(ICMPTest, TTLExceededV6MaxSize) {
  runTTLExceededV6Test(1184);
}

TEST(ICMPTest, TTLExceededV6Truncated) {
  runTTLExceededV6Test(1300);
}

TEST(ICMPTest, PacketTooBigV6) {
  cfg::SwitchConfig config = testConfigA();
  /*
   * sender is on interface with MTU 9000
   * receiver is on interface with MTU 1500
   */
  config.interfaces()[0].mtu() = 9000;
  config.interfaces()[1].mtu() = 1500;
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();

  /* ICMPv6 exchange on vlan 1 */
  PortID portID(1);
  VlanID vlanID(1);

  /*
   * sender sends a packet of size 9000, excluding 802.1q ethernet header
   * this makes input frame size as 9018 bytes
   */
  constexpr uint16_t inputFrameSize = 9018;
  std::array<uint8_t, inputFrameSize> zeros; // maximum required padding
  zeros.fill(0);

  // ICMPv6 echo request of size 9000
  auto header = PktUtil::parseHexData(
      // dst mac (intf1 of switch), src mac
      "00 02 00 00 00 01  02 03 04 05 06 07"
      // 802.1q header, VLAN 1
      "81 00 00 01"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "60 00 00 00"
      // Payload length: 8960 (MTU of 9000 - IPv6 header),
      // this has to be greater that MTU of interface on which
      // receiver is reachable to trigger PTB
      "23 00"
      // Next Header: 58 (ICMPv6), Hop Limit (127)
      "3a 7F"
      // src addr (2401:db00:2110:3001::a) (unicast in subnet of intf1)
      "24 01 db 00 21 10 30 01 00 00 00 00 00 00 00 0a"
      // dst addr (2401:db00:2110:3055::a) (unicast in subnet intf2)
      "24 01 db 00 21 10 30 55 00 00 00 00 00 00 00 0a"
      // type: echo request
      "80"
      // code
      "00"
      // checksum (does matter if validateChecksum is called)
      "9c 31"
      // identifier(2004), sequence (01)
      "20 04 00 01");

  // pad zero bytes to make packet fill entire mtu
  header.appendChain(
      folly::IOBuf::wrapBuffer(zeros.data(), inputFrameSize - header.length()));

  // create a single input packet
  IOBuf pkt(IOBuf::WRAP_BUFFER, header.coalesce());

  ASSERT_EQ(pkt.length(), inputFrameSize);

  // header without type (ICMPv6), code (PTB) & checksum
  auto icmpHeader = PktUtil::parseHexData(
      // MTU 1500 of a link that goes to receiver
      "00 00 05 dc"
      // subsequently it is an original IPv6 packet that triggered the response
      // Version 6, traffic class, flow label
      "60 00 00 00"
      // Payload length: 9001 (larger than MTU)
      "23 00"
      // Next Header: 58 (ICMPv6), Hop Limit (127)
      "3a 7F"
      // src addr (2401:db00:2110:3001::a) (unicast in subnet of intf1)
      "24 01 db 00 21 10 30 01 00 00 00 00 00 00 00 0a"
      // dst addr (2401:db00:2110:3055::a) (unicast in subnet intf2)
      "24 01 db 00 21 10 30 55 00 00 00 00 00 00 00 0a"
      // type: echo request
      "80"
      // code
      "00"
      // checksum
      "9c 31"
      // identifier(2004), sequence (01)
      "20 04 00 01");

  // Minumum IPv6 MTU (1280)
  // IPv6 header (40) +  Ethernet header(18) = 58
  // Expected payload size = 1280 - 58 = 1222
  // pad the icmpHeader with zeros
  icmpHeader.appendChain(
      IOBuf::wrapBuffer(zeros.data(), 1222 - icmpHeader.length()));

  // create a single ICMPv6 PTB response buffer
  IOBuf icmp6Payload(IOBuf::WRAP_BUFFER, icmpHeader.coalesce());

  // Cache the current stats
  CounterCache counters(sw);

  EXPECT_HW_CALL(sw, stateChangedImpl(_)).Times(0);
  EXPECT_SWITCHED_PKT(
      sw,
      "ICMPv6 Packet Too Big",
      checkICMPv6PacketTooBig(
          folly::MacAddress("00:02:00:00:00:01"), /* switch's interface1 mac */
          IPAddressV6("2401:db00:2110:3001::1"), /* switch's interface1 addr */
          folly::MacAddress("02:03:04:05:06:07"), /* sender's mac */
          IPAddressV6("2401:db00:2110:3001::a"), /* sender's addr */
          VlanID(1), /* on vlan */
          icmp6Payload.data(), /* payload to check excluding Type/Code/Cksum */
          1218)); /* 1222 - 4 bytes of ICMPv6 header (Type/Code/Cksum ) */

  /* receive a big packet */
  handle->rxPacket(std::make_unique<folly::IOBuf>(pkt), portID, vlanID);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "trapped.packet_too_big.sum", 1);
}
