// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/UDPDatagram.h"

#include <gtest/gtest.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/UDPHeader.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace {
UDPHeader makeTestUDPHeader() {
  return UDPHeader(
      /*srcPort=*/5000,
      /*dstPort=*/5001,
      /*length=*/0, // will be set by UDPDatagram constructor
      /*csum=*/0);
}

std::vector<uint8_t> makeTestPayload() {
  return {0x01, 0x02, 0x03, 0x04, 0x05};
}
} // namespace

// --- Constructor tests ---

TEST(UDPDatagramTest, constructFromHeaderAndPayload) {
  auto hdr = makeTestUDPHeader();
  auto payload = makeTestPayload();
  UDPDatagram dgram(hdr, payload);

  EXPECT_EQ(dgram.header().srcPort, 5000);
  EXPECT_EQ(dgram.header().dstPort, 5001);
  EXPECT_EQ(dgram.payload(), payload);
  // Constructor sets length = header size + payload size
  EXPECT_EQ(dgram.header().length, UDPHeader::size() + payload.size());
}

TEST(UDPDatagramTest, constructFromHeaderEmptyPayload) {
  auto hdr = makeTestUDPHeader();
  UDPDatagram dgram(hdr, {});

  EXPECT_EQ(dgram.header().srcPort, 5000);
  EXPECT_TRUE(dgram.payload().empty());
  EXPECT_EQ(dgram.header().length, UDPHeader::size());
}

TEST(UDPDatagramTest, constructorSetsLengthCorrectly) {
  auto hdr = makeTestUDPHeader();
  std::vector<uint8_t> payload(100, 0xff);
  UDPDatagram dgram(hdr, payload);
  EXPECT_EQ(dgram.header().length, UDPHeader::size() + 100);
}

// --- length() tests ---

TEST(UDPDatagramTest, lengthWithPayload) {
  auto hdr = makeTestUDPHeader();
  auto payload = makeTestPayload();
  UDPDatagram dgram(hdr, payload);
  // UDPHeader::size() (8) + 5 = 13
  EXPECT_EQ(dgram.length(), UDPHeader::size() + payload.size());
}

TEST(UDPDatagramTest, lengthWithoutPayload) {
  auto hdr = makeTestUDPHeader();
  UDPDatagram dgram(hdr, {});
  EXPECT_EQ(dgram.length(), UDPHeader::size());
}

// --- serialize and parse round-trip ---

TEST(UDPDatagramTest, serializeRoundTrip) {
  auto hdr = makeTestUDPHeader();
  auto payload = makeTestPayload();
  UDPDatagram dgram(hdr, payload);

  auto buf = folly::IOBuf::create(dgram.length());
  buf->append(dgram.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  dgram.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  UDPDatagram dgram2(readCursor);

  EXPECT_EQ(dgram, dgram2);
  EXPECT_EQ(dgram2.header().srcPort, 5000);
  EXPECT_EQ(dgram2.header().dstPort, 5001);
  EXPECT_EQ(dgram2.payload(), payload);
}

TEST(UDPDatagramTest, serializeRoundTripEmptyPayload) {
  auto hdr = makeTestUDPHeader();
  UDPDatagram dgram(hdr, {});

  auto buf = folly::IOBuf::create(dgram.length());
  buf->append(dgram.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  dgram.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  UDPDatagram dgram2(readCursor);

  EXPECT_EQ(dgram, dgram2);
  EXPECT_TRUE(dgram2.payload().empty());
}

// --- getTxPacket tests ---

TEST(UDPDatagramTest, getTxPacket) {
  auto hdr = makeTestUDPHeader();
  auto payload = makeTestPayload();
  UDPDatagram dgram(hdr, payload);

  auto txPkt = dgram.getTxPacket(&TxPacket::allocateTxPacket);
  ASSERT_NE(txPkt, nullptr);
  EXPECT_EQ(txPkt->buf()->computeChainDataLength(), dgram.length());

  // Parse back from TxPacket
  folly::io::Cursor cursor(txPkt->buf());
  UDPDatagram dgram2(cursor);
  EXPECT_EQ(dgram, dgram2);
}

TEST(UDPDatagramTest, getTxPacketEmptyPayload) {
  auto hdr = makeTestUDPHeader();
  UDPDatagram dgram(hdr, {});

  auto txPkt = dgram.getTxPacket(&TxPacket::allocateTxPacket);
  ASSERT_NE(txPkt, nullptr);
  EXPECT_EQ(txPkt->buf()->computeChainDataLength(), UDPHeader::size());
}

// --- equality tests ---

TEST(UDPDatagramTest, equalPackets) {
  auto hdr = makeTestUDPHeader();
  auto payload = makeTestPayload();
  UDPDatagram dgram1(hdr, payload);
  UDPDatagram dgram2(hdr, payload);
  EXPECT_EQ(dgram1, dgram2);
}

TEST(UDPDatagramTest, unequalDifferentPorts) {
  auto payload = makeTestPayload();
  UDPDatagram dgram1(UDPHeader(5000, 5001, 0), payload);
  UDPDatagram dgram2(UDPHeader(9000, 9001, 0), payload);
  EXPECT_NE(dgram1, dgram2);
}

TEST(UDPDatagramTest, unequalDifferentPayload) {
  auto hdr = makeTestUDPHeader();
  UDPDatagram dgram1(hdr, {0x01, 0x02});
  UDPDatagram dgram2(hdr, {0x03, 0x04});
  EXPECT_NE(dgram1, dgram2);
}

TEST(UDPDatagramTest, equalIgnoresChecksum) {
  auto hdr1 = UDPHeader(5000, 5001, 0, 0x1111);
  auto hdr2 = UDPHeader(5000, 5001, 0, 0x2222);
  auto payload = makeTestPayload();
  UDPDatagram dgram1(hdr1, payload);
  UDPDatagram dgram2(hdr2, payload);
  // UDPDatagram::operator== ignores checksum
  EXPECT_EQ(dgram1, dgram2);
}

// --- toString test ---

TEST(UDPDatagramTest, toStringContainsPorts) {
  auto hdr = makeTestUDPHeader();
  auto payload = makeTestPayload();
  UDPDatagram dgram(hdr, payload);
  auto str = dgram.toString();
  // Verify both ports and length appear in output
  EXPECT_NE(str.find("5000"), std::string::npos);
  EXPECT_NE(str.find("5001"), std::string::npos);
  EXPECT_NE(
      str.find(std::to_string(UDPHeader::size() + payload.size())),
      std::string::npos);
}

// --- Boundary port values ---

TEST(UDPDatagramTest, boundaryPortZero) {
  UDPDatagram dgram(UDPHeader(0, 0, 0), makeTestPayload());
  EXPECT_EQ(dgram.header().srcPort, 0);
  EXPECT_EQ(dgram.header().dstPort, 0);

  // Round-trip preserves port 0
  auto buf = folly::IOBuf::create(dgram.length());
  buf->append(dgram.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  dgram.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  UDPDatagram dgram2(readCursor);
  EXPECT_EQ(dgram2.header().srcPort, 0);
  EXPECT_EQ(dgram2.header().dstPort, 0);
}

TEST(UDPDatagramTest, boundaryPortMax) {
  UDPDatagram dgram(UDPHeader(65535, 65535, 0), makeTestPayload());
  EXPECT_EQ(dgram.header().srcPort, 65535);
  EXPECT_EQ(dgram.header().dstPort, 65535);

  // Round-trip preserves max ports
  auto buf = folly::IOBuf::create(dgram.length());
  buf->append(dgram.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  dgram.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  UDPDatagram dgram2(readCursor);
  EXPECT_EQ(dgram2.header().srcPort, 65535);
  EXPECT_EQ(dgram2.header().dstPort, 65535);
}

// --- Checksum preservation in round-trip ---

TEST(UDPDatagramTest, checksumPreservedInRoundTrip) {
  auto hdr = UDPHeader(5000, 5001, 0, 0xabcd);
  auto payload = makeTestPayload();
  UDPDatagram dgram(hdr, payload);

  auto buf = folly::IOBuf::create(dgram.length());
  buf->append(dgram.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  dgram.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  UDPDatagram dgram2(readCursor);
  EXPECT_EQ(dgram2.header().csum, 0xabcd);
}

// --- Large payload round-trip ---

TEST(UDPDatagramTest, largePayloadRoundTrip) {
  auto hdr = makeTestUDPHeader();
  std::vector<uint8_t> largePayload(1024, 0xab);
  UDPDatagram dgram(hdr, largePayload);

  auto buf = folly::IOBuf::create(dgram.length());
  buf->append(dgram.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  dgram.serialize(cursor);

  folly::io::Cursor readCursor(buf.get());
  UDPDatagram dgram2(readCursor);

  EXPECT_EQ(dgram2.payload().size(), 1024);
  EXPECT_EQ(dgram2.payload()[0], 0xab);
  EXPECT_EQ(dgram2.payload()[1023], 0xab);
}
