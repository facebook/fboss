/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/PktUtil.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Random.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IOBuf;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::Random;
using folly::io::Cursor;

IOBuf setupBuf() {
  IOBuf buf1(IOBuf::CREATE, 20);
  auto buf2 = IOBuf::create(20);
  buf1.append(20);
  buf2->append(20);
  memcpy(
      buf1.writableData(),
      "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"
      "\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a",
      20);
  memcpy(
      buf2->writableData(),
      "\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a"
      "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a",
      20);
  buf1.appendChain(std::move(buf2));
  return buf1;
}

TEST(PktUtilTest, ReadMac) {
  auto buf = setupBuf();

  // Test parsing at the start of the buffer,
  // where the MAC is entirely contiguous
  Cursor c(&buf);
  EXPECT_EQ(MacAddress("01:02:03:04:05:06"), PktUtil::readMac(&c));
  EXPECT_EQ(Cursor(&buf) + 6, c);

  // Test parsing a MAC that ends at the end of a buffer.
  c = Cursor(&buf) + 14;
  EXPECT_EQ(MacAddress("15:16:17:18:19:1a"), PktUtil::readMac(&c));
  EXPECT_EQ(Cursor(&buf) + 20, c);

  // Test parsing a MAC that spans two buffers
  c = Cursor(&buf) + 17;
  EXPECT_EQ(MacAddress("18:19:1a:21:22:23"), PktUtil::readMac(&c));
  EXPECT_EQ(Cursor(&buf) + 23, c);

  // Test parsing when the buffer is too short to contain a MAC
  c = Cursor(&buf) + 38;
  EXPECT_THROW(PktUtil::readMac(&c), std::out_of_range);
}

TEST(PktUtilTest, ReadIPv4) {
  auto buf = setupBuf();

  // Test parsing at the start of the buffer,
  // where the IP is entirely contiguous
  Cursor c(&buf);
  EXPECT_EQ(IPAddressV4("1.2.3.4"), PktUtil::readIPv4(&c));
  EXPECT_EQ(Cursor(&buf) + 4, c);

  // Test parsing an IP that ends at the end of a buffer.
  c = Cursor(&buf) + 16;
  EXPECT_EQ(IPAddressV4("23.24.25.26"), PktUtil::readIPv4(&c));
  EXPECT_EQ(Cursor(&buf) + 20, c);

  // Test parsing a MAC that spans two buffers
  c = Cursor(&buf) + 18;
  EXPECT_EQ(IPAddressV4("25.26.33.34"), PktUtil::readIPv4(&c));
  EXPECT_EQ(Cursor(&buf) + 22, c);

  // Test parsing when the buffer is too short to contain a MAC
  c = Cursor(&buf) + 38;
  EXPECT_THROW(PktUtil::readIPv4(&c), std::out_of_range);
}

TEST(PktUtilTest, ReadIPv6) {
  auto buf = setupBuf();

  // Test parsing at the start of the buffer,
  // where the IP is entirely contiguous
  Cursor c(&buf);
  EXPECT_EQ(
      IPAddressV6("0102:0304:0506:0708:090a:1112:1314:1516"),
      PktUtil::readIPv6(&c));
  EXPECT_EQ(Cursor(&buf) + 16, c);

  // Test parsing an IP that ends at the end of a buffer.
  c = Cursor(&buf) + 4;
  EXPECT_EQ(
      IPAddressV6("0506:0708:090a:1112:1314:1516:1718:191a"),
      PktUtil::readIPv6(&c));
  EXPECT_EQ(Cursor(&buf) + 20, c);

  // Test parsing a MAC that spans two buffers
  c = Cursor(&buf) + 12;
  EXPECT_EQ(
      IPAddressV6("1314:1516:1718:191a:2122:2324:2526:2728"),
      PktUtil::readIPv6(&c));
  EXPECT_EQ(Cursor(&buf) + 28, c);

  // Test parsing when the buffer is too short to contain a MAC
  c = Cursor(&buf) + 38;
  EXPECT_THROW(PktUtil::readIPv6(&c), std::out_of_range);
}

TEST(PktUtilTest, HexDumpEmptyBuffer) {
  IOBuf buf(IOBuf::CREATE, 0);
  EXPECT_EQ("", PktUtil::hexDump(buf));
}

TEST(PktUtilTest, HexDump) {
  size_t length = 64;
  IOBuf buf(IOBuf::CREATE, length);
  buf.append(length);
  uint8_t value = 0;
  for (size_t idx = 0; idx < length; ++idx) {
    buf.writableData()[idx] = value;
    ++value;
  }

  Cursor start(&buf);
  EXPECT_EQ("0000: 00", PktUtil::hexDump(start, start + 1));
  EXPECT_EQ(
      "0000: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e",
      PktUtil::hexDump(start, start + 15));
  EXPECT_EQ(
      "0000: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f",
      PktUtil::hexDump(start, start + 16));
  EXPECT_EQ(
      "0000: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n"
      "0010: 10",
      PktUtil::hexDump(start, start + 17));
  EXPECT_EQ(
      "0000: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n"
      "0010: 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f\n"
      "0020: 20 21 22 23 24 25 26 27",
      PktUtil::hexDump(start, start + 40));
}

void testCsum(uint32_t toInsert) {
  auto size = toInsert + 2;
  uint8_t bytes[size];
  bytes[0] = bytes[1] = 0;
  LOG(INFO) << " Checksum buffer size: " << toInsert;
  for (auto i = 2; i < size; ++i) {
    bytes[i] = Random::rand32(std::numeric_limits<uint8_t>::max());
  }
  // checksum is returned in host byte order need to
  // convert it to n/w byte order so that the value
  // gets interpreted correctly when we place it in
  // bytes array.
  uint16_t csum = htons(PktUtil::internetChecksum(bytes, size));
  memcpy(bytes, &csum, 2);
  EXPECT_EQ(0, PktUtil::internetChecksum(bytes, size));
}

TEST(Checksum, TestEven) {
  auto toInsert = Random::rand32(1000);
  if (toInsert % 2) {
    ++toInsert;
  }
  testCsum(toInsert);
}

TEST(Checksum, TestOdd) {
  auto toInsert = Random::rand32(1000);
  if (!(toInsert % 2)) {
    ++toInsert;
  }
  testCsum(toInsert);
}

TEST(Checksum, TestKnownEven) {
  // Example from RFC 1071
  uint8_t bytes[8];
  bytes[0] = 0x00;
  bytes[1] = 0x01;
  bytes[2] = 0xf2;
  bytes[3] = 0x03;
  bytes[4] = 0xf4;
  bytes[5] = 0xf5;
  bytes[6] = 0xf6;
  bytes[7] = 0xf7;
  // RFC 1071 omits the final inversion
  // in its illustrative examples.
  uint16_t expected = 0xddf2;
  expected = ~expected;
  EXPECT_EQ(expected, PktUtil::internetChecksum(bytes, 8));
}

TEST(Checksum, TestKnownOdd) {
  // Simple extension of example from RFC 1071
  // by appending a byte to the bytes in their
  // examples so that we end up with a odd number
  // of bytes.
  uint8_t bytes[9];
  bytes[0] = 0x00;
  bytes[1] = 0x01;
  bytes[2] = 0xf2;
  bytes[3] = 0x03;
  bytes[4] = 0xf4;
  bytes[5] = 0xf5;
  bytes[6] = 0xf6;
  bytes[7] = 0xf7;
  bytes[8] = 0x01;
  uint16_t expected = 0xdef2;
  expected = ~expected;
  EXPECT_EQ(expected, PktUtil::internetChecksum(bytes, 9));
}
