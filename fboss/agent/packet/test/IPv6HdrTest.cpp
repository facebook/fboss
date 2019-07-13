/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/IPv6Hdr.h"

/*
 * References:
 *   https://code.google.com/p/googletest/wiki/Primer
 *   https://code.google.com/p/googletest/wiki/AdvancedGuide
 */
#include <gtest/gtest.h>

#include <folly/IPAddress.h>
#include <folly/IPAddressV6.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV6;
using folly::io::Cursor;
using std::string;

TEST(IPv6HdrTest, default_constructor) {
  IPv6Hdr ipv6Hdr;
  EXPECT_EQ(6, ipv6Hdr.version);
  EXPECT_EQ(0, ipv6Hdr.trafficClass);
  EXPECT_EQ(0, ipv6Hdr.flowLabel);
  EXPECT_EQ(0, ipv6Hdr.payloadLength);
  EXPECT_EQ(0, ipv6Hdr.nextHeader);
  EXPECT_EQ(255, ipv6Hdr.hopLimit);
  EXPECT_EQ(IPAddressV6("::0"), ipv6Hdr.srcAddr);
  EXPECT_EQ(IPAddressV6("::0"), ipv6Hdr.dstAddr);
}

TEST(IPv6HdrTest, copy_constructor) {
  uint8_t version = IPV6_VERSION;
  uint8_t trafficClass = 0;
  uint32_t flowLabel = 0;
  uint16_t payloadLength = 0;
  uint8_t nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT);
  uint8_t hopLimit = 1;
  IPAddressV6 srcAddr("2620:0:1cfe:face:b00c::3");
  IPAddressV6 dstAddr("2620:0:1cfe:face:b00c::4");
  IPv6Hdr lhs(
      version,
      trafficClass,
      flowLabel,
      payloadLength,
      nextHeader,
      hopLimit,
      srcAddr,
      dstAddr);
  IPv6Hdr rhs(lhs);
  EXPECT_EQ(lhs, rhs);
}

TEST(IPv6HdrTest, parameterized_data_constructor) {
  uint8_t version = IPV6_VERSION;
  uint8_t trafficClass = 0;
  uint32_t flowLabel = 0;
  uint16_t payloadLength = 0;
  uint8_t nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT);
  uint8_t hopLimit = 1;
  IPAddressV6 srcAddr("2620:0:1cfe:face:b00c::3");
  IPAddressV6 dstAddr("2620:0:1cfe:face:b00c::4");
  IPv6Hdr ipv6Hdr(
      version,
      trafficClass,
      flowLabel,
      payloadLength,
      nextHeader,
      hopLimit,
      srcAddr,
      dstAddr);
  EXPECT_EQ(version, ipv6Hdr.version);
  EXPECT_EQ(trafficClass, ipv6Hdr.trafficClass);
  EXPECT_EQ(flowLabel, ipv6Hdr.flowLabel);
  EXPECT_EQ(payloadLength, ipv6Hdr.payloadLength);
  EXPECT_EQ(nextHeader, ipv6Hdr.nextHeader);
  EXPECT_EQ(hopLimit, ipv6Hdr.hopLimit);
  EXPECT_EQ(srcAddr, ipv6Hdr.srcAddr);
  EXPECT_EQ(dstAddr, ipv6Hdr.dstAddr);
}

TEST(IPv6HdrTest, cursor_data_constructor) {
  uint8_t version = IPV6_VERSION;
  uint8_t trafficClass = 0xff;
  uint32_t flowLabel = 0xfffff;
  uint16_t payloadLength = 0;
  uint8_t nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT);
  uint8_t hopLimit = 1;
  IPAddressV6 srcAddr("2620:0:1cfe:face:b00c::3");
  IPAddressV6 dstAddr("2620:0:1cfe:face:b00c::4");
  auto pkt = MockRxPacket::fromHex(
      // IPv6 Header
      "6" // VERSION: 6
      "ff" // Traffic Class
      "fffff" // Flow Label
      "00 00" // Payload Length
      "3B" // Next Header: No Next Header
      "01" // Hop Limit: 1
      // IPv6 Source Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // IPv6 Destination Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04");
  Cursor cursor(pkt->buf());
  IPv6Hdr ipv6Hdr(cursor);
  EXPECT_EQ(version, ipv6Hdr.version);
  EXPECT_EQ(trafficClass, ipv6Hdr.trafficClass);
  EXPECT_EQ(flowLabel, ipv6Hdr.flowLabel);
  EXPECT_EQ(payloadLength, ipv6Hdr.payloadLength);
  EXPECT_EQ(nextHeader, ipv6Hdr.nextHeader);
  EXPECT_EQ(hopLimit, ipv6Hdr.hopLimit);
  EXPECT_EQ(srcAddr, ipv6Hdr.srcAddr);
  EXPECT_EQ(dstAddr, ipv6Hdr.dstAddr);
}

TEST(IPv6HdrTest, parseBroadcast) {
  // Test a broadcast packet from a node with no IP to all nodes
  auto pkt = MockRxPacket::fromHex(
      // IPv6 Header
      "6" // VERSION: 6
      "00" // Traffic Class
      "00000" // Flow Label
      "00 00" // Payload Length
      "3B" // Next Header: No Next Header
      "ff" // Hop Limit
      // IPv6 Source Address
      "00 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 00"
      // IPv6 Destination Address
      "ff 02 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 01");
  Cursor cursor(pkt->buf());
  IPv6Hdr hdr(cursor);
  EXPECT_EQ(IPV6_VERSION, hdr.version);
  EXPECT_EQ(0, hdr.trafficClass);
  EXPECT_EQ(0, hdr.flowLabel);
  EXPECT_EQ(0, hdr.payloadLength);
  EXPECT_EQ(
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT), hdr.nextHeader);
  EXPECT_EQ(0xff, hdr.hopLimit);
  EXPECT_EQ(IPAddressV6("::0"), hdr.srcAddr);
  EXPECT_EQ(IPAddressV6("ff02::1"), hdr.dstAddr);
}

TEST(IPv6HdrTest, cursor_data_constructor_too_small) {
  auto pkt = MockRxPacket::fromHex(
      // IPv6 Header
      "6" // VERSION: 6
      "00" // Traffic Class
      "00000" // Flow Label
      "00 00" // Payload Length
      "3B" // Next Header: No Next Header
      "01" // Hop Limit: 1
      // IPv6 Source Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // IPv6 Destination Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00   " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv6Hdr ipv6Hdr(cursor); }, HdrParseError);
}

TEST(IPv6HdrTest, cursor_data_constructor_bad_version) {
  auto pkt = MockRxPacket::fromHex(
      // IPv6 Header
      "4" // VERSION: 4
      "00" // Traffic Class
      "00000" // Flow Label
      "00 00" // Payload Length
      "3B" // Next Header: No Next Header
      "01" // Hop Limit: 1
      // IPv6 Source Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // IPv6 Destination Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04");
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv6Hdr ipv6Hdr(cursor); }, HdrParseError);
}

TEST(IPv6HdrTest, cursor_data_constructor_zero_hop_limit) {
  auto pkt = MockRxPacket::fromHex(
      // IPv6 Header
      "6" // VERSION: 6
      "00" // Traffic Class
      "00000" // Flow Label
      "00 00" // Payload Length
      "3B" // Next Header: No Next Header
      "00" // Hop Limit: 0
      // IPv6 Source Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // IPv6 Destination Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04");
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv6Hdr ipv6Hdr(cursor); }, HdrParseError);
}

TEST(IPv6HdrTest, assignment_operator) {
  uint8_t version = IPV6_VERSION;
  uint8_t trafficClass = 0;
  uint32_t flowLabel = 0;
  uint16_t payloadLength = 0;
  uint8_t nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT);
  uint8_t hopLimit = 1;
  IPAddressV6 srcAddr("2620:0:1cfe:face:b00c::3");
  IPAddressV6 dstAddr("2620:0:1cfe:face:b00c::4");
  IPv6Hdr lhs(
      version,
      trafficClass,
      flowLabel,
      payloadLength,
      nextHeader,
      hopLimit,
      srcAddr,
      dstAddr);
  IPv6Hdr rhs = lhs;
  EXPECT_EQ(lhs, rhs);
}

TEST(IPv6HdrTest, equality_operator) {
  uint8_t version = IPV6_VERSION;
  uint8_t trafficClass = 0;
  uint32_t flowLabel = 0;
  uint16_t payloadLength = 0;
  uint8_t nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT);
  uint8_t hopLimit = 1;
  IPAddressV6 srcAddr("2620:0:1cfe:face:b00c::3");
  IPAddressV6 dstAddr("2620:0:1cfe:face:b00c::4");
  IPv6Hdr lhs(
      version,
      trafficClass,
      flowLabel,
      payloadLength,
      nextHeader,
      hopLimit,
      srcAddr,
      dstAddr);
  IPv6Hdr rhs(
      version,
      trafficClass,
      flowLabel,
      payloadLength,
      nextHeader,
      hopLimit,
      srcAddr,
      dstAddr);
  EXPECT_EQ(lhs, rhs);
}

TEST(IPv6HdrTest, inequality_operator) {
  uint8_t version = IPV6_VERSION;
  uint8_t trafficClass = 0;
  uint32_t flowLabel = 0;
  uint16_t payloadLength = 0;
  uint8_t nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT);
  uint8_t hopLimit = 1;
  IPAddressV6 srcAddr("2620:0:1cfe:face:b00c::3");
  IPAddressV6 dstAddr1("2620:0:1cfe:face:b00c::4");
  IPAddressV6 dstAddr2("2620:0:1cfe:face:b00c::5");
  IPv6Hdr lhs(
      version,
      trafficClass,
      flowLabel,
      payloadLength,
      nextHeader,
      hopLimit,
      srcAddr,
      dstAddr1);
  IPv6Hdr rhs(
      version,
      trafficClass,
      flowLabel,
      payloadLength,
      nextHeader,
      hopLimit,
      srcAddr,
      dstAddr2);
  EXPECT_NE(lhs, rhs);
}
