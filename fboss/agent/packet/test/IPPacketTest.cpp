// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/IPPacket.h"

#include <gtest/gtest.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/PktUtil.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

TEST(IPPacketTest, parseV6WithInnerV6) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header (40 bytes)
      // version=6, traffic class=0, flow label=0
      "60 00 00 00"
      // payload length = 40 (inner IPv6 header only)
      "00 28"
      // next header = 0x29 (IPv6)
      "29"
      // hop limit = 64
      "40"
      // src addr: 2001:db8::1
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      // dst addr: 2001:db8::2
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 02"
      // Inner IPv6 header (40 bytes)
      // version=6, traffic class=0, flow label=0
      "60 00 00 00"
      // payload length = 0
      "00 00"
      // next header = 0x3B (No Next Header)
      "3B"
      // hop limit = 63
      "3F"
      // src addr: fd00::1
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      // dst addr: fd00::2
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 02");

  folly::io::Cursor cursor(&buf);
  IPPacket<folly::IPAddressV6> pkt(cursor);

  // Outer header
  EXPECT_EQ(
      pkt.header().nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6));
  EXPECT_EQ(pkt.header().srcAddr, folly::IPAddressV6("2001:db8::1"));
  EXPECT_EQ(pkt.header().dstAddr, folly::IPAddressV6("2001:db8::2"));
  EXPECT_EQ(pkt.header().hopLimit, 64);

  // Inner v6 payload should be parsed
  EXPECT_NE(pkt.v6PayLoad(), nullptr);
  EXPECT_EQ(pkt.v4PayLoad(), nullptr);
  EXPECT_FALSE(pkt.udpPayload().has_value());
  EXPECT_FALSE(pkt.tcpPayload().has_value());

  // Inner header fields
  auto* innerV6 = pkt.v6PayLoad();
  EXPECT_EQ(innerV6->header().srcAddr, folly::IPAddressV6("fd00::1"));
  EXPECT_EQ(innerV6->header().dstAddr, folly::IPAddressV6("fd00::2"));
  EXPECT_EQ(innerV6->header().hopLimit, 63);
  EXPECT_EQ(
      innerV6->header().nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_NONXT));

  // Total length: outer (40) + inner (40)
  EXPECT_EQ(pkt.length(), 80);
}

TEST(IPPacketTest, parseV6WithInnerV4) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header (40 bytes)
      // version=6, traffic class=0, flow label=0
      "60 00 00 00"
      // payload length = 20 (inner IPv4 header only)
      "00 14"
      // next header = 0x04 (IPv4)
      "04"
      // hop limit = 64
      "40"
      // src addr: 2001:db8::1
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      // dst addr: 2001:db8::2
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 02"
      // Inner IPv4 header (20 bytes)
      // version=4, IHL=5, DSCP=0, ECN=0
      "45 00"
      // total length = 20
      "00 14"
      // identification
      "00 00"
      // flags + fragment offset
      "40 00"
      // TTL=64
      "40"
      // protocol=0 (no payload)
      "00"
      // checksum (placeholder)
      "00 00"
      // src: 10.0.0.1
      "0a 00 00 01"
      // dst: 10.0.0.2
      "0a 00 00 02");

  folly::io::Cursor cursor(&buf);
  IPPacket<folly::IPAddressV6> pkt(cursor);

  // Outer header
  EXPECT_EQ(
      pkt.header().nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4));
  EXPECT_EQ(pkt.header().srcAddr, folly::IPAddressV6("2001:db8::1"));

  // Inner v4 payload should be parsed
  EXPECT_NE(pkt.v4PayLoad(), nullptr);
  EXPECT_EQ(pkt.v6PayLoad(), nullptr);
  EXPECT_FALSE(pkt.udpPayload().has_value());

  // Inner header fields
  auto* innerV4 = pkt.v4PayLoad();
  EXPECT_EQ(innerV4->header().srcAddr, folly::IPAddressV4("10.0.0.1"));
  EXPECT_EQ(innerV4->header().dstAddr, folly::IPAddressV4("10.0.0.2"));
  EXPECT_EQ(innerV4->header().ttl, 64);

  // Total length: outer (40) + inner (20)
  EXPECT_EQ(pkt.length(), 60);
}

TEST(IPPacketTest, parseV4WithInnerV4) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv4 header (20 bytes)
      // version=4, IHL=5, DSCP=0, ECN=0
      "45 00"
      // total length = 40 (outer 20 + inner 20)
      "00 28"
      // identification
      "00 00"
      // flags + fragment offset
      "40 00"
      // TTL=64
      "40"
      // protocol = 4 (IPv4)
      "04"
      // checksum (placeholder)
      "00 00"
      // src: 192.168.1.1
      "c0 a8 01 01"
      // dst: 192.168.1.2
      "c0 a8 01 02"
      // Inner IPv4 header (20 bytes)
      // version=4, IHL=5
      "45 00"
      // total length = 20
      "00 14"
      // identification
      "00 00"
      // flags + fragment offset
      "40 00"
      // TTL=63
      "3F"
      // protocol=0
      "00"
      // checksum
      "00 00"
      // src: 10.0.0.1
      "0a 00 00 01"
      // dst: 10.0.0.2
      "0a 00 00 02");

  folly::io::Cursor cursor(&buf);
  IPPacket<folly::IPAddressV4> pkt(cursor);

  EXPECT_EQ(
      pkt.header().protocol, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4));
  EXPECT_EQ(pkt.header().srcAddr, folly::IPAddressV4("192.168.1.1"));

  EXPECT_NE(pkt.v4PayLoad(), nullptr);
  EXPECT_EQ(pkt.v6PayLoad(), nullptr);

  auto* innerV4 = pkt.v4PayLoad();
  EXPECT_EQ(innerV4->header().srcAddr, folly::IPAddressV4("10.0.0.1"));
  EXPECT_EQ(innerV4->header().dstAddr, folly::IPAddressV4("10.0.0.2"));
  EXPECT_EQ(innerV4->header().ttl, 63);
}

TEST(IPPacketTest, parseV4WithInnerV6) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv4 header (20 bytes)
      // version=4, IHL=5
      "45 00"
      // total length = 60 (outer 20 + inner 40)
      "00 3c"
      // identification
      "00 00"
      // flags + fragment offset
      "40 00"
      // TTL=64
      "40"
      // protocol = 41 (0x29, IPv6)
      "29"
      // checksum
      "00 00"
      // src: 192.168.1.1
      "c0 a8 01 01"
      // dst: 192.168.1.2
      "c0 a8 01 02"
      // Inner IPv6 header (40 bytes)
      "60 00 00 00"
      // payload length = 0
      "00 00"
      // next header = 0x3B (No Next Header)
      "3B"
      // hop limit = 63
      "3F"
      // src addr: fd00::1
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      // dst addr: fd00::2
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 02");

  folly::io::Cursor cursor(&buf);
  IPPacket<folly::IPAddressV4> pkt(cursor);

  EXPECT_EQ(
      pkt.header().protocol, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6));
  EXPECT_EQ(pkt.header().srcAddr, folly::IPAddressV4("192.168.1.1"));

  EXPECT_EQ(pkt.v4PayLoad(), nullptr);
  EXPECT_NE(pkt.v6PayLoad(), nullptr);

  auto* innerV6 = pkt.v6PayLoad();
  EXPECT_EQ(innerV6->header().srcAddr, folly::IPAddressV6("fd00::1"));
  EXPECT_EQ(innerV6->header().dstAddr, folly::IPAddressV6("fd00::2"));
  EXPECT_EQ(innerV6->header().hopLimit, 63);
}

TEST(IPPacketTest, copyConstructorWithInnerPayload) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header
      "60 00 00 00"
      "00 28"
      "29"
      "40"
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 02"
      // Inner IPv6 header
      "60 00 00 00"
      "00 00"
      "3B"
      "3F"
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 02");

  folly::io::Cursor cursor(&buf);
  IPPacket<folly::IPAddressV6> pkt(cursor);

  // Copy
  auto pktCopy = pkt;
  EXPECT_EQ(pkt, pktCopy);
  EXPECT_NE(pktCopy.v6PayLoad(), nullptr);
  EXPECT_EQ(
      pktCopy.v6PayLoad()->header().srcAddr, folly::IPAddressV6("fd00::1"));

  // Verify deep copy (different pointers)
  EXPECT_NE(pkt.v6PayLoad(), pktCopy.v6PayLoad());
}

TEST(IPPacketTest, noInnerPayloadWhenUDP) {
  auto buf = PktUtil::parseHexData(
      // IPv6 header
      "60 00 00 00"
      // payload length = 8 (UDP header only)
      "00 08"
      // next header = 17 (UDP)
      "11"
      // hop limit = 64
      "40"
      // src addr
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      // dst addr
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 02"
      // UDP header (8 bytes)
      // src port
      "1f 40"
      // dst port
      "1f 41"
      // length = 8
      "00 08"
      // checksum
      "00 00");

  folly::io::Cursor cursor(&buf);
  IPPacket<folly::IPAddressV6> pkt(cursor);

  EXPECT_TRUE(pkt.udpPayload().has_value());
  EXPECT_EQ(pkt.v4PayLoad(), nullptr);
  EXPECT_EQ(pkt.v6PayLoad(), nullptr);
}

TEST(IPPacketTest, noInnerPayloadWhenTCP) {
  auto buf = PktUtil::parseHexData(
      // IPv6 header
      "60 00 00 00"
      // payload length = 20 (TCP header only)
      "00 14"
      // next header = 6 (TCP)
      "06"
      // hop limit = 64
      "40"
      // src addr
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      // dst addr
      "20 01 0d b8 00 00 00 00"
      "00 00 00 00 00 00 00 02"
      // TCP header (20 bytes)
      // src port = 8000
      "1f 40"
      // dst port = 8001
      "1f 41"
      // sequence number
      "00 00 00 01"
      // ack number
      "00 00 00 00"
      // data offset (5 << 4) + reserved
      "50"
      // flags (SYN)
      "02"
      // window size
      "ff ff"
      // checksum
      "00 00"
      // urgent pointer
      "00 00");

  folly::io::Cursor cursor(&buf);
  IPPacket<folly::IPAddressV6> pkt(cursor);

  EXPECT_TRUE(pkt.tcpPayload().has_value());
  EXPECT_FALSE(pkt.udpPayload().has_value());
  EXPECT_EQ(pkt.v4PayLoad(), nullptr);
  EXPECT_EQ(pkt.v6PayLoad(), nullptr);
}
