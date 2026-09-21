// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/TCPPacket.h"

#include <gtest/gtest.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/TCPHeader.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace {
TCPHeader makeTestHeader() {
  return TCPHeader(
      /*srcPort=*/8080,
      /*dstPort=*/443,
      /*seqNumber=*/1000,
      /*ackNumber=*/2000,
      /*flags=*/0x12, // SYN+ACK
      /*windowSize=*/65535,
      /*urgentPointer=*/0);
}

std::vector<uint8_t> makeTestPayload() {
  return {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe};
}
} // namespace

// --- Constructor tests ---

TEST(TCPPacketTest, constructFromHeaderAndPayload) {
  auto hdr = makeTestHeader();
  auto payload = makeTestPayload();
  TCPPacket pkt(hdr, payload);

  EXPECT_EQ(pkt.header().srcPort, 8080);
  EXPECT_EQ(pkt.header().dstPort, 443);
  EXPECT_EQ(pkt.header().sequenceNumber, 1000);
  EXPECT_EQ(pkt.header().ackNumber, 2000);
  EXPECT_EQ(pkt.payload(), payload);
}

TEST(TCPPacketTest, constructFromHeaderEmptyPayload) {
  auto hdr = makeTestHeader();
  std::vector<uint8_t> empty;
  TCPPacket pkt(hdr, empty);

  EXPECT_EQ(pkt.header().srcPort, 8080);
  EXPECT_TRUE(pkt.payload().empty());
}

// --- length() tests ---

TEST(TCPPacketTest, lengthWithPayload) {
  auto hdr = makeTestHeader();
  auto payload = makeTestPayload();
  TCPPacket pkt(hdr, payload);
  // TCP header (20) + payload (6) = 26
  EXPECT_EQ(pkt.length(), TCPHeader::size() + payload.size());
}

TEST(TCPPacketTest, lengthWithoutPayload) {
  auto hdr = makeTestHeader();
  TCPPacket pkt(hdr, {});
  EXPECT_EQ(pkt.length(), TCPHeader::size());
}

// --- serialize and parse round-trip ---

TEST(TCPPacketTest, serializeRoundTrip) {
  auto hdr = makeTestHeader();
  auto payload = makeTestPayload();
  TCPPacket pkt(hdr, payload);

  auto buf = folly::IOBuf::create(pkt.length());
  buf->append(pkt.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  pkt.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  TCPPacket pkt2(readCursor, pkt.length());

  EXPECT_EQ(pkt, pkt2);
  EXPECT_EQ(pkt2.header().srcPort, 8080);
  EXPECT_EQ(pkt2.header().dstPort, 443);
  EXPECT_EQ(pkt2.header().sequenceNumber, 1000);
  EXPECT_EQ(pkt2.header().ackNumber, 2000);
  EXPECT_EQ(pkt2.payload(), payload);
}

TEST(TCPPacketTest, serializeRoundTripEmptyPayload) {
  auto hdr = makeTestHeader();
  TCPPacket pkt(hdr, {});

  auto buf = folly::IOBuf::create(pkt.length());
  buf->append(pkt.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  pkt.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  TCPPacket pkt2(readCursor, pkt.length());

  EXPECT_EQ(pkt, pkt2);
  EXPECT_TRUE(pkt2.payload().empty());
}

// --- getTxPacket tests ---

TEST(TCPPacketTest, getTxPacket) {
  auto hdr = makeTestHeader();
  auto payload = makeTestPayload();
  TCPPacket pkt(hdr, payload);

  auto txPkt = pkt.getTxPacket(&TxPacket::allocateTxPacket);
  ASSERT_NE(txPkt, nullptr);
  EXPECT_EQ(txPkt->buf()->computeChainDataLength(), pkt.length());

  // Parse back from TxPacket
  folly::io::Cursor cursor(txPkt->buf());
  TCPPacket pkt2(cursor, pkt.length());
  EXPECT_EQ(pkt, pkt2);
}

TEST(TCPPacketTest, getTxPacketEmptyPayload) {
  auto hdr = makeTestHeader();
  TCPPacket pkt(hdr, {});

  auto txPkt = pkt.getTxPacket(&TxPacket::allocateTxPacket);
  ASSERT_NE(txPkt, nullptr);
  EXPECT_EQ(txPkt->buf()->computeChainDataLength(), TCPHeader::size());
}

// --- equality tests ---

TEST(TCPPacketTest, equalPackets) {
  auto hdr = makeTestHeader();
  auto payload = makeTestPayload();
  TCPPacket pkt1(hdr, payload);
  TCPPacket pkt2(hdr, payload);
  EXPECT_EQ(pkt1, pkt2);
}

TEST(TCPPacketTest, unequalDifferentPorts) {
  auto hdr1 = makeTestHeader();
  auto hdr2 = TCPHeader(9090, 80);
  auto payload = makeTestPayload();
  TCPPacket pkt1(hdr1, payload);
  TCPPacket pkt2(hdr2, payload);
  EXPECT_NE(pkt1, pkt2);
}

TEST(TCPPacketTest, unequalDifferentPayload) {
  auto hdr = makeTestHeader();
  TCPPacket pkt1(hdr, {0x01, 0x02});
  TCPPacket pkt2(hdr, {0x03, 0x04});
  EXPECT_NE(pkt1, pkt2);
}

TEST(TCPPacketTest, equalIgnoresChecksum) {
  auto hdr1 = makeTestHeader();
  auto hdr2 = makeTestHeader();
  hdr1.csum = 0x1111;
  hdr2.csum = 0x2222;
  auto payload = makeTestPayload();
  TCPPacket pkt1(hdr1, payload);
  TCPPacket pkt2(hdr2, payload);
  // TCPPacket::operator== ignores checksum
  EXPECT_EQ(pkt1, pkt2);
}

// --- toString test ---

TEST(TCPPacketTest, toStringContainsPorts) {
  auto hdr = makeTestHeader();
  auto payload = makeTestPayload();
  TCPPacket pkt(hdr, payload);
  auto str = pkt.toString();
  // Verify both ports appear in output
  EXPECT_NE(str.find("8080"), std::string::npos);
  EXPECT_NE(str.find("443"), std::string::npos);
  // Verify sequence number appears
  EXPECT_NE(str.find("1000"), std::string::npos);
}

// --- Parse from cursor with payload ---

TEST(TCPPacketTest, parseFromCursorLargePayload) {
  auto hdr = makeTestHeader();
  std::vector<uint8_t> largePayload(512, 0xab);
  TCPPacket pkt(hdr, largePayload);

  auto buf = folly::IOBuf::create(pkt.length());
  buf->append(pkt.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  pkt.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  TCPPacket pkt2(readCursor, pkt.length());

  EXPECT_EQ(pkt2.payload().size(), 512);
  EXPECT_EQ(pkt2.payload()[0], 0xab);
  EXPECT_EQ(pkt2.payload()[511], 0xab);
}
