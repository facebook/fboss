// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/MPLSPacket.h"

#include <gtest/gtest.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/IPPacket.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/UDPDatagram.h"
#include "fboss/agent/packet/UDPHeader.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace {
const auto kSrcIpV4 = folly::IPAddressV4("10.0.0.1");
const auto kDstIpV4 = folly::IPAddressV4("10.0.0.2");
const auto kSrcIpV6 = folly::IPAddressV6("2001:db8::1");
const auto kDstIpV6 = folly::IPAddressV6("2001:db8::2");

IPPacket<folly::IPAddressV4> makeV4Packet() {
  IPv4Hdr ipHdr(
      kSrcIpV4, kDstIpV4, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP), 0);
  UDPHeader udpHdr(8000, 8001, 0);
  std::vector<uint8_t> payload = {0xaa, 0xbb, 0xcc, 0xdd};
  return IPPacket<folly::IPAddressV4>(ipHdr, UDPDatagram(udpHdr, payload));
}

IPPacket<folly::IPAddressV6> makeV6Packet() {
  IPv6Hdr ipHdr;
  ipHdr.srcAddr = kSrcIpV6;
  ipHdr.dstAddr = kDstIpV6;
  ipHdr.setProtocol(static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  UDPHeader udpHdr(9000, 9001, 0);
  std::vector<uint8_t> payload = {0x11, 0x22, 0x33};
  return IPPacket<folly::IPAddressV6>(ipHdr, UDPDatagram(udpHdr, payload));
}
} // namespace

// --- Constructor tests ---

TEST(MPLSPacketTest, constructFromHdrOnly) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  MPLSPacket pkt(hdr);

  EXPECT_EQ(pkt.header().size(), 4); // single label
  EXPECT_FALSE(pkt.v4PayLoad().has_value());
  EXPECT_FALSE(pkt.v6PayLoad().has_value());
}

TEST(MPLSPacketTest, constructWithV4Payload) {
  MPLSHdr hdr(MPLSHdr::Label{200, 0, true, 128});
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt(hdr, v4Pkt);

  EXPECT_TRUE(pkt.v4PayLoad().has_value());
  EXPECT_FALSE(pkt.v6PayLoad().has_value());
  EXPECT_EQ(pkt.v4PayLoad()->header().srcAddr, kSrcIpV4);
  EXPECT_EQ(pkt.v4PayLoad()->header().dstAddr, kDstIpV4);
}

TEST(MPLSPacketTest, constructWithV6Payload) {
  MPLSHdr hdr(MPLSHdr::Label{300, 0, true, 64});
  auto v6Pkt = makeV6Packet();
  MPLSPacket pkt(hdr, v6Pkt);

  EXPECT_FALSE(pkt.v4PayLoad().has_value());
  EXPECT_TRUE(pkt.v6PayLoad().has_value());
  EXPECT_EQ(pkt.v6PayLoad()->header().srcAddr, kSrcIpV6);
  EXPECT_EQ(pkt.v6PayLoad()->header().dstAddr, kDstIpV6);
}

// --- length() tests ---

TEST(MPLSPacketTest, lengthHeaderOnly) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  MPLSPacket pkt(hdr);
  EXPECT_EQ(pkt.length(), hdr.size());
}

TEST(MPLSPacketTest, lengthWithV4Payload) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt(hdr, v4Pkt);
  EXPECT_EQ(pkt.length(), hdr.size() + v4Pkt.length());
}

TEST(MPLSPacketTest, lengthWithV6Payload) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  auto v6Pkt = makeV6Packet();
  MPLSPacket pkt(hdr, v6Pkt);
  EXPECT_EQ(pkt.length(), hdr.size() + v6Pkt.length());
}

// --- serialize and parse round-trip ---

TEST(MPLSPacketTest, serializeRoundTripV4) {
  MPLSHdr hdr(MPLSHdr::Label{500, 3, true, 255});
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt(hdr, v4Pkt);

  auto buf = folly::IOBuf::create(pkt.length());
  buf->append(pkt.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  pkt.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  MPLSPacket pkt2(readCursor);

  EXPECT_TRUE(pkt2.v4PayLoad().has_value());
  EXPECT_FALSE(pkt2.v6PayLoad().has_value());
  EXPECT_EQ(pkt2.v4PayLoad()->header().srcAddr, kSrcIpV4);
  EXPECT_EQ(pkt2.v4PayLoad()->header().dstAddr, kDstIpV4);
}

TEST(MPLSPacketTest, serializeRoundTripV6) {
  MPLSHdr hdr(MPLSHdr::Label{600, 0, true, 64});
  auto v6Pkt = makeV6Packet();
  MPLSPacket pkt(hdr, v6Pkt);

  auto buf = folly::IOBuf::create(pkt.length());
  buf->append(pkt.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  pkt.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  MPLSPacket pkt2(readCursor);

  EXPECT_FALSE(pkt2.v4PayLoad().has_value());
  EXPECT_TRUE(pkt2.v6PayLoad().has_value());
  EXPECT_EQ(pkt2.v6PayLoad()->header().srcAddr, kSrcIpV6);
  EXPECT_EQ(pkt2.v6PayLoad()->header().dstAddr, kDstIpV6);
}

TEST(MPLSPacketTest, serializeRoundTripHeaderOnly) {
  MPLSHdr hdr(MPLSHdr::Label{700, 0, true, 32});
  MPLSPacket pkt(hdr);

  auto buf = folly::IOBuf::create(pkt.length());
  buf->append(pkt.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  pkt.serialize(cursor);

  // Can't parse back without payload since cursor will try to read IP version
  // from beyond the label stack. Just verify serialization doesn't crash and
  // length is correct.
  EXPECT_EQ(buf->computeChainDataLength(), hdr.size());
}

// --- getTxPacket tests ---

TEST(MPLSPacketTest, getTxPacketV4) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt(hdr, v4Pkt);

  auto txPkt = pkt.getTxPacket(&TxPacket::allocateTxPacket);
  ASSERT_NE(txPkt, nullptr);
  EXPECT_EQ(txPkt->buf()->computeChainDataLength(), pkt.length());

  // Parse back and verify
  folly::io::Cursor cursor(txPkt->buf());
  MPLSPacket pkt2(cursor);
  EXPECT_TRUE(pkt2.v4PayLoad().has_value());
}

TEST(MPLSPacketTest, getTxPacketV6) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  auto v6Pkt = makeV6Packet();
  MPLSPacket pkt(hdr, v6Pkt);

  auto txPkt = pkt.getTxPacket(&TxPacket::allocateTxPacket);
  ASSERT_NE(txPkt, nullptr);
  EXPECT_EQ(txPkt->buf()->computeChainDataLength(), pkt.length());

  folly::io::Cursor cursor(txPkt->buf());
  MPLSPacket pkt2(cursor);
  EXPECT_TRUE(pkt2.v6PayLoad().has_value());
}

// --- equality tests ---

TEST(MPLSPacketTest, equalPackets) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt1(hdr, v4Pkt);
  MPLSPacket pkt2(hdr, v4Pkt);
  EXPECT_EQ(pkt1, pkt2);
}

TEST(MPLSPacketTest, unequalDifferentLabel) {
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt1(MPLSHdr(MPLSHdr::Label{100, 0, true, 64}), v4Pkt);
  MPLSPacket pkt2(MPLSHdr(MPLSHdr::Label{200, 0, true, 64}), v4Pkt);
  EXPECT_NE(pkt1, pkt2);
}

TEST(MPLSPacketTest, unequalDifferentPayloadType) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  MPLSPacket pkt1(hdr, makeV4Packet());
  MPLSPacket pkt2(hdr, makeV6Packet());
  EXPECT_NE(pkt1, pkt2);
}

// --- decrementTTL tests ---

TEST(MPLSPacketTest, decrementTTLWithV6Payload) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  auto v6Pkt = makeV6Packet();
  MPLSPacket pkt(hdr, v6Pkt);

  auto origHopLimit = pkt.v6PayLoad()->header().hopLimit;
  pkt.decrementTTL();
  EXPECT_EQ(pkt.v6PayLoad()->header().hopLimit, origHopLimit - 1);
}

TEST(MPLSPacketTest, decrementTTLWithV4Payload) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt(hdr, v4Pkt);

  auto origTtl = pkt.v4PayLoad()->header().ttl;
  pkt.decrementTTL();
  EXPECT_EQ(pkt.v4PayLoad()->header().ttl, origTtl - 1);
}

// --- decrementTTL edge case: TTL=1 ---

TEST(MPLSPacketTest, decrementTTLFromOneV4) {
  // TTL=1 is the minimum before expiry; decrement should reach 0
  IPv4Hdr ipHdr(
      kSrcIpV4, kDstIpV4, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP), 0);
  ipHdr.ttl = 1;
  UDPHeader udpHdr(8000, 8001, 0);
  IPPacket<folly::IPAddressV4> v4Pkt(
      ipHdr, UDPDatagram(udpHdr, std::vector<uint8_t>{0xaa}));

  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  MPLSPacket pkt(hdr, v4Pkt);

  EXPECT_EQ(pkt.v4PayLoad()->header().ttl, 1);
  pkt.decrementTTL();
  EXPECT_EQ(pkt.v4PayLoad()->header().ttl, 0);
}

TEST(MPLSPacketTest, decrementTTLFromOneV6) {
  IPv6Hdr ipHdr;
  ipHdr.srcAddr = kSrcIpV6;
  ipHdr.dstAddr = kDstIpV6;
  ipHdr.setProtocol(static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  ipHdr.hopLimit = 1;
  UDPHeader udpHdr(9000, 9001, 0);
  IPPacket<folly::IPAddressV6> v6Pkt(
      ipHdr, UDPDatagram(udpHdr, std::vector<uint8_t>{0x11}));

  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  MPLSPacket pkt(hdr, v6Pkt);

  EXPECT_EQ(pkt.v6PayLoad()->header().hopLimit, 1);
  pkt.decrementTTL();
  EXPECT_EQ(pkt.v6PayLoad()->header().hopLimit, 0);
}

// --- toString test ---

TEST(MPLSPacketTest, toStringContainsInfo) {
  MPLSHdr hdr(MPLSHdr::Label{100, 0, true, 64});
  MPLSPacket pkt(hdr, makeV4Packet());
  auto str = pkt.toString();
  EXPECT_NE(str.find("MPLS"), std::string::npos);
  // Verify label value appears
  EXPECT_NE(str.find("100"), std::string::npos);
}

// --- Multi-label stack ---

TEST(MPLSPacketTest, multiLabelStack) {
  std::vector<MPLSHdr::Label> labels = {
      {100, 0, false, 64},
      {200, 0, false, 64},
      {300, 0, true, 64},
  };
  MPLSHdr hdr(labels);
  auto v4Pkt = makeV4Packet();
  MPLSPacket pkt(hdr, v4Pkt);

  // 3 labels * 4 bytes = 12
  EXPECT_EQ(pkt.header().size(), 12);
  EXPECT_EQ(pkt.length(), 12 + v4Pkt.length());

  auto buf = folly::IOBuf::create(pkt.length());
  buf->append(pkt.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  pkt.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  MPLSPacket pkt2(readCursor);
  EXPECT_TRUE(pkt2.v4PayLoad().has_value());
  EXPECT_EQ(pkt2.header().size(), 12);
}
