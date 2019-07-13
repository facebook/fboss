/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/IPv4Hdr.h"

/*
 * References:
 *   https://code.google.com/p/googletest/wiki/Primer
 *   https://code.google.com/p/googletest/wiki/AdvancedGuide
 */
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

using namespace facebook::fboss;
using folly::IOBuf;
using folly::IPAddressV4;
using folly::io::Appender;
using folly::io::Cursor;

TEST(IPv4HdrTest, default_constructor) {
  IPv4Hdr ipv4Hdr;
  EXPECT_EQ(0, ipv4Hdr.version);
  EXPECT_EQ(0, ipv4Hdr.ihl);
  EXPECT_EQ(0, ipv4Hdr.dscp);
  EXPECT_EQ(0, ipv4Hdr.ecn);
  EXPECT_EQ(0, ipv4Hdr.length);
  EXPECT_EQ(0, ipv4Hdr.id);
  EXPECT_FALSE(ipv4Hdr.dontFragment);
  EXPECT_FALSE(ipv4Hdr.moreFragments);
  EXPECT_EQ(0, ipv4Hdr.fragmentOffset);
  EXPECT_EQ(0, ipv4Hdr.ttl);
  EXPECT_EQ(0, ipv4Hdr.protocol);
  EXPECT_EQ(0, ipv4Hdr.csum);
  EXPECT_EQ(IPAddressV4("0.0.0.0"), ipv4Hdr.srcAddr);
  EXPECT_EQ(IPAddressV4("0.0.0.0"), ipv4Hdr.dstAddr);
}

TEST(IPv4HdrTest, copy_constructor) {
  uint8_t version = IPV4_VERSION;
  uint8_t ihl = 5;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t length = 20;
  uint16_t id = 0;
  bool dontFragment = false;
  bool moreFragments = false;
  uint16_t fragmentOffset = 0;
  uint8_t ttl = 1;
  uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  IPv4Hdr lhs(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  IPv4Hdr rhs(lhs);
  EXPECT_EQ(lhs, rhs);
}

TEST(IPv4HdrTest, parameterized_data_constructor) {
  uint8_t version = IPV4_VERSION;
  uint8_t ihl = 5;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t length = 20;
  uint16_t id = 0;
  bool dontFragment = false;
  bool moreFragments = false;
  uint16_t fragmentOffset = 0;
  uint8_t ttl = 1;
  uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  IPv4Hdr ipv4Hdr(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  EXPECT_EQ(version, ipv4Hdr.version);
  EXPECT_EQ(ihl, ipv4Hdr.ihl);
  EXPECT_EQ(dscp, ipv4Hdr.dscp);
  EXPECT_EQ(ecn, ipv4Hdr.ecn);
  EXPECT_EQ(length, ipv4Hdr.length);
  EXPECT_EQ(id, ipv4Hdr.id);
  EXPECT_EQ(dontFragment, ipv4Hdr.dontFragment);
  EXPECT_EQ(moreFragments, ipv4Hdr.moreFragments);
  EXPECT_EQ(fragmentOffset, ipv4Hdr.fragmentOffset);
  EXPECT_EQ(ttl, ipv4Hdr.ttl);
  EXPECT_EQ(protocol, ipv4Hdr.protocol);
  EXPECT_EQ(csum, ipv4Hdr.csum);
  EXPECT_EQ(srcAddr, ipv4Hdr.srcAddr);
  EXPECT_EQ(dstAddr, ipv4Hdr.dstAddr);
}

TEST(IPv4HdrTest, cursor_data_constructor) {
  uint8_t version = IPV4_VERSION;
  uint8_t ihl = 5;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t length = 20;
  uint16_t id = 0;
  bool dontFragment = false;
  bool moreFragments = false;
  uint16_t fragmentOffset = 0;
  uint8_t ttl = 1;
  uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  auto pkt = MockRxPacket::fromHex(
      // IPv4 Header
      "4" // version: 4
      "5" // IHL: 5
      "00" // DSCP, ECN
      "00 14" // length: 20
      "00 00" // id: 0
      "00 00" // flags and fragment offset
      "01" // TTL: 1
      "06" // protocol: TCP
      "00 00" // checksum
      "0a 00 00 0f" // Source Address: 10.0.0.15
      "0a 00 00 01" // Destination Address: 10.0.0.1
  );
  Cursor cursor(pkt->buf());
  IPv4Hdr ipv4Hdr(cursor);
  EXPECT_EQ(version, ipv4Hdr.version);
  EXPECT_EQ(ihl, ipv4Hdr.ihl);
  EXPECT_EQ(dscp, ipv4Hdr.dscp);
  EXPECT_EQ(ecn, ipv4Hdr.ecn);
  EXPECT_EQ(length, ipv4Hdr.length);
  EXPECT_EQ(id, ipv4Hdr.id);
  EXPECT_EQ(dontFragment, ipv4Hdr.dontFragment);
  EXPECT_EQ(moreFragments, ipv4Hdr.moreFragments);
  EXPECT_EQ(fragmentOffset, ipv4Hdr.fragmentOffset);
  EXPECT_EQ(ttl, ipv4Hdr.ttl);
  EXPECT_EQ(protocol, ipv4Hdr.protocol);
  EXPECT_EQ(csum, ipv4Hdr.csum);
  EXPECT_EQ(srcAddr, ipv4Hdr.srcAddr);
  EXPECT_EQ(dstAddr, ipv4Hdr.dstAddr);
}

TEST(IPv4HdrTest, cursor_data_constructor_too_small) {
  auto pkt = MockRxPacket::fromHex(
      // IPv4 Header
      "4" // version: 4
      "5" // IHL: 5
      "00" // DSCP, ECN
      "00 14" // length: 20
      "00 00" // id: 0
      "00 00" // flags and fragment offset
      "01" // TTL: 1
      "06" // protocol: TCP
      "00 00" // checksum
      "0a 00 00 0f" // Source Address: 10.0.0.15
      "0a 00 00   " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv4Hdr ipv4Hdr(cursor); }, HdrParseError);
}

TEST(IPv4HdrTest, cursor_data_constructor_bad_version) {
  auto pkt = MockRxPacket::fromHex(
      // IPv4 Header
      "6" // version: 6
      "5" // IHL: 5
      "00" // DSCP, ECN
      "00 14" // length: 20
      "00 00" // id: 0
      "00 00" // flags and fragment offset
      "01" // TTL: 1
      "06" // protocol: TCP
      "00 00" // checksum
      "0a 00 00 0f" // Source Address: 10.0.0.15
      "0a 00 00 01" // Destination Address: 10.0.0.1
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv4Hdr ipv4Hdr(cursor); }, HdrParseError);
}

TEST(IPv4HdrTest, cursor_data_constructor_bad_ihl) {
  auto pkt = MockRxPacket::fromHex(
      // IPv4 Header
      "4" // version: 4
      "4" // IHL: 4
      "00" // DSCP, ECN
      "00 14" // length: 20
      "00 00" // id: 0
      "00 00" // flags and fragment offset
      "01" // TTL: 1
      "06" // protocol: TCP
      "00 00" // checksum
      "0a 00 00 0f" // Source Address: 10.0.0.15
      "0a 00 00 01" // Destination Address: 10.0.0.1
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv4Hdr ipv4Hdr(cursor); }, HdrParseError);
}

TEST(IPv4HdrTest, cursor_data_constructor_bad_length) {
  auto pkt = MockRxPacket::fromHex(
      // IPv4 Header
      "4" // version: 4
      "5" // IHL: 5
      "00" // DSCP, ECN
      "00 13" // length: 19
      "00 00" // id: 0
      "00 00" // flags and fragment offset
      "01" // TTL: 1
      "06" // protocol: TCP
      "00 00" // checksum
      "0a 00 00 0f" // Source Address: 10.0.0.15
      "0a 00 00 01" // Destination Address: 10.0.0.1
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv4Hdr ipv4Hdr(cursor); }, HdrParseError);
}

TEST(IPv4HdrTest, cursor_data_constructor_zero_ttl) {
  auto pkt = MockRxPacket::fromHex(
      // IPv4 Header
      "4" // version: 4
      "5" // IHL: 5
      "00" // DSCP, ECN
      "00 14" // length: 10
      "00 00" // id: 0
      "00 00" // flags and fragment offset
      "00" // TTL: 0
      "06" // protocol: TCP
      "00 00" // checksum
      "0a 00 00 0f" // Source Address: 10.0.0.15
      "0a 00 00 01" // Destination Address: 10.0.0.1
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ IPv4Hdr ipv4Hdr(cursor); }, HdrParseError);
}

TEST(IPv4HdrTest, write_to_buffer) {
  uint8_t version = IPV4_VERSION;
  uint8_t ihl = 5;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t length = 20;
  uint16_t id = 0;
  bool dontFragment = false;
  bool moreFragments = false;
  uint16_t fragmentOffset = 0;
  uint8_t ttl = 1;
  uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  IPv4Hdr lhs(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  auto buf = IOBuf::create(IPv4Hdr::minSize());
  Appender appender(buf.get(), 100);
  lhs.write(&appender);
  EXPECT_EQ(buf->computeChainDataLength(), IPv4Hdr::minSize());
  Cursor cursor(buf.get());
  IPv4Hdr rhs(cursor);
  EXPECT_TRUE(lhs == rhs);
}

TEST(IPv4HdrTest, assignment_operator) {
  uint8_t version = IPV4_VERSION;
  uint8_t ihl = 5;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t length = 20;
  uint16_t id = 0;
  bool dontFragment = false;
  bool moreFragments = false;
  uint16_t fragmentOffset = 0;
  uint8_t ttl = 1;
  uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  IPv4Hdr lhs(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  IPv4Hdr rhs = lhs;
  EXPECT_EQ(lhs, rhs);
}

TEST(IPv4HdrTest, equality_operator) {
  uint8_t version = IPV4_VERSION;
  uint8_t ihl = 5;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t length = 20;
  uint16_t id = 0;
  bool dontFragment = false;
  bool moreFragments = false;
  uint16_t fragmentOffset = 0;
  uint8_t ttl = 1;
  uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  IPv4Hdr lhs(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  IPv4Hdr rhs(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  EXPECT_EQ(lhs, rhs);
}

TEST(IPv4HdrTest, inequality_operator) {
  uint8_t version = IPV4_VERSION;
  uint8_t ihl = 5;
  uint8_t dscp = 0;
  uint8_t ecn = 0;
  uint16_t length = 20;
  uint16_t id = 0;
  bool dontFragment = false;
  bool moreFragments = false;
  uint16_t fragmentOffset = 0;
  uint8_t ttl1 = 1;
  uint8_t ttl2 = 2;
  uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP);
  uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  IPv4Hdr lhs(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl1,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  IPv4Hdr rhs(
      version,
      ihl,
      dscp,
      ecn,
      length,
      id,
      dontFragment,
      moreFragments,
      fragmentOffset,
      ttl2,
      protocol,
      csum,
      srcAddr,
      dstAddr);
  EXPECT_NE(lhs, rhs);
}
