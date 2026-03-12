// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/Srv6Packet.h"

#include <gtest/gtest.h>

#include <sstream>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/PktUtil.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

TEST(Srv6PacketTest, headerOnlyConstructor) {
  IPv6Hdr hdr;
  hdr.srcAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
  hdr.dstAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  hdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4);
  hdr.hopLimit = 64;

  Srv6Packet pkt(hdr);
  EXPECT_EQ(pkt.header().srcAddr, hdr.srcAddr);
  EXPECT_EQ(pkt.header().dstAddr, hdr.dstAddr);
  EXPECT_EQ(pkt.header().nextHeader, hdr.nextHeader);
  EXPECT_FALSE(pkt.v4PayLoad().has_value());
  EXPECT_FALSE(pkt.v6PayLoad().has_value());
  EXPECT_EQ(pkt.length(), IPv6Hdr::SIZE);
}

TEST(Srv6PacketTest, constructWithV4Payload) {
  IPv6Hdr outerHdr;
  outerHdr.srcAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
  outerHdr.dstAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  outerHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4);
  outerHdr.hopLimit = 64;

  IPv4Hdr innerHdr(
      folly::IPAddressV4("10.0.0.1"), folly::IPAddressV4("10.0.0.2"), 0, 0);
  IPPacket<folly::IPAddressV4> innerPkt(innerHdr);

  Srv6Packet pkt(outerHdr, innerPkt);
  EXPECT_TRUE(pkt.v4PayLoad().has_value());
  EXPECT_FALSE(pkt.v6PayLoad().has_value());
  EXPECT_EQ(pkt.length(), IPv6Hdr::SIZE + innerPkt.length());
}

TEST(Srv6PacketTest, constructWithV6Payload) {
  IPv6Hdr outerHdr;
  outerHdr.srcAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
  outerHdr.dstAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  outerHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6);
  outerHdr.hopLimit = 64;

  IPv6Hdr innerHdr(
      folly::IPAddressV6("fd00::1"), folly::IPAddressV6("fd00::2"));
  innerHdr.nextHeader = 0x3B; // No Next Header
  IPPacket<folly::IPAddressV6> innerPkt(innerHdr);

  Srv6Packet pkt(outerHdr, innerPkt);
  EXPECT_FALSE(pkt.v4PayLoad().has_value());
  EXPECT_TRUE(pkt.v6PayLoad().has_value());
  EXPECT_EQ(pkt.length(), IPv6Hdr::SIZE + innerPkt.length());
}

TEST(Srv6PacketTest, parseFromCursorV4Inner) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header (40 bytes)
      // version=6, traffic class=0, flow label=0
      "60 00 00 00"
      // payload length = 20 (inner IPv4 header, no payload)
      "00 14"
      // next header = 4 (IPv4)
      "04"
      // hop limit = 64
      "40"
      // src addr: 2620:0:1cfe:face:b00c::3
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // dst addr: 2620:0:1cfe:face:b00c::4
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04"
      // Inner IPv4 header (20 bytes, no options)
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
  Srv6Packet pkt(cursor);
  EXPECT_EQ(
      pkt.header().nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4));
  EXPECT_TRUE(pkt.v4PayLoad().has_value());
  EXPECT_FALSE(pkt.v6PayLoad().has_value());
  EXPECT_EQ(pkt.v4PayLoad()->header().srcAddr, folly::IPAddressV4("10.0.0.1"));
  EXPECT_EQ(pkt.v4PayLoad()->header().dstAddr, folly::IPAddressV4("10.0.0.2"));
}

TEST(Srv6PacketTest, parseFromCursorV6Inner) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header (40 bytes)
      // version=6, traffic class=0, flow label=0
      "60 00 00 00"
      // payload length = 40 (inner IPv6 header, no payload)
      "00 28"
      // next header = 41 (0x29, IPv6)
      "29"
      // hop limit = 64
      "40"
      // src addr: 2620:0:1cfe:face:b00c::3
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // dst addr: 2620:0:1cfe:face:b00c::4
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04"
      // Inner IPv6 header (40 bytes)
      // version=6, traffic class=0, flow label=0
      "60 00 00 00"
      // payload length = 0
      "00 00"
      // next header = 0x3B (No Next Header)
      "3B"
      // hop limit = 64
      "40"
      // src addr: fd00::1
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      // dst addr: fd00::2
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 02");

  folly::io::Cursor cursor(&buf);
  Srv6Packet pkt(cursor);
  EXPECT_EQ(
      pkt.header().nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6));
  EXPECT_FALSE(pkt.v4PayLoad().has_value());
  EXPECT_TRUE(pkt.v6PayLoad().has_value());
  EXPECT_EQ(pkt.v6PayLoad()->header().srcAddr, folly::IPAddressV6("fd00::1"));
  EXPECT_EQ(pkt.v6PayLoad()->header().dstAddr, folly::IPAddressV6("fd00::2"));
}

TEST(Srv6PacketTest, parseUnsupportedNextHeaderThrows) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header with nextHeader = 6 (TCP, unsupported)
      "60 00 00 00"
      "00 00"
      // next header = 6 (TCP)
      "06"
      "40"
      // src addr
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // dst addr
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04");

  folly::io::Cursor cursor(&buf);
  EXPECT_THROW(Srv6Packet pkt(cursor), FbossError);
}

TEST(Srv6PacketTest, serializeRoundTripV4) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header
      "60 00 00 00"
      "00 14"
      "04"
      "40"
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04"
      // Inner IPv4 header
      "45 00"
      "00 14"
      "00 00"
      "40 00"
      "40"
      "00"
      "00 00"
      "0a 00 00 01"
      "0a 00 00 02");

  folly::io::Cursor cursor(&buf);
  Srv6Packet pkt(cursor);

  auto buf2 = folly::IOBuf::create(pkt.length());
  buf2->append(pkt.length());
  folly::io::RWPrivateCursor writeCursor(buf2.get());
  pkt.serialize(writeCursor);

  folly::io::Cursor cursor2(buf2.get());
  Srv6Packet pkt2(cursor2);
  EXPECT_EQ(pkt, pkt2);
}

TEST(Srv6PacketTest, serializeRoundTripV6) {
  auto buf = PktUtil::parseHexData(
      // Outer IPv6 header
      "60 00 00 00"
      "00 28"
      "29"
      "40"
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04"
      // Inner IPv6 header
      "60 00 00 00"
      "00 00"
      "3B"
      "40"
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 01"
      "fd 00 00 00 00 00 00 00"
      "00 00 00 00 00 00 00 02");

  folly::io::Cursor cursor(&buf);
  Srv6Packet pkt(cursor);

  auto buf2 = folly::IOBuf::create(pkt.length());
  buf2->append(pkt.length());
  folly::io::RWPrivateCursor writeCursor(buf2.get());
  pkt.serialize(writeCursor);

  folly::io::Cursor cursor2(buf2.get());
  Srv6Packet pkt2(cursor2);
  EXPECT_EQ(pkt, pkt2);
}

TEST(Srv6PacketTest, equality) {
  IPv6Hdr hdr;
  hdr.srcAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
  hdr.dstAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  hdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4);
  hdr.hopLimit = 64;

  IPv4Hdr innerHdr(
      folly::IPAddressV4("10.0.0.1"), folly::IPAddressV4("10.0.0.2"), 0, 0);
  IPPacket<folly::IPAddressV4> innerPkt(innerHdr);

  Srv6Packet pkt1(hdr, innerPkt);
  Srv6Packet pkt2(hdr, innerPkt);
  EXPECT_EQ(pkt1, pkt2);

  // Different outer header
  IPv6Hdr hdr2 = hdr;
  hdr2.hopLimit = 32;
  Srv6Packet pkt3(hdr2, innerPkt);
  EXPECT_FALSE(pkt1 == pkt3);
}

TEST(Srv6PacketTest, toStringContainsSrv6) {
  IPv6Hdr hdr;
  hdr.srcAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
  hdr.dstAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  Srv6Packet pkt(hdr);
  auto str = pkt.toString();
  EXPECT_NE(str.find("SRv6"), std::string::npos);
}

TEST(Srv6PacketTest, ostreamOperator) {
  IPv6Hdr hdr;
  hdr.srcAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
  hdr.dstAddr = folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  Srv6Packet pkt(hdr);
  std::ostringstream os;
  os << pkt;
  EXPECT_EQ(os.str(), pkt.toString());
}
