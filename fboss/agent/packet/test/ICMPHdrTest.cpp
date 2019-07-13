/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/ICMPHdr.h"

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

TEST(ICMPHdrTest, default_constructor) {
  ICMPHdr icmpv6Hdr;
  EXPECT_EQ(0, icmpv6Hdr.type);
  EXPECT_EQ(0, icmpv6Hdr.code);
  EXPECT_EQ(0, icmpv6Hdr.csum);
}

TEST(ICMPHdrTest, copy_constructor) {
  uint8_t type = static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST);
  uint8_t code = 0;
  uint16_t csum = 0; // Obviously wrong
  ICMPHdr lhs(type, code, csum);
  ICMPHdr rhs(lhs);
  EXPECT_EQ(lhs, rhs);
}

TEST(ICMPHdrTest, parameterized_data_constructor) {
  uint8_t type = static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST);
  uint8_t code = 0;
  uint16_t csum = 0; // Obviously wrong
  ICMPHdr lhs(type, code, csum);
  EXPECT_EQ(type, lhs.type);
  EXPECT_EQ(code, lhs.code);
  EXPECT_EQ(csum, lhs.csum);
}

TEST(ICMPHdrTest, cursor_data_constructor) {
  uint8_t type = static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST);
  uint8_t code = 0;
  uint16_t csum = 0; // Obviously wrong
  auto pkt = MockRxPacket::fromHex(
      // ICMPv6 Header
      "80" // Type: Echo Request
      "00" // Code: 0
      "00 00" // Checksum
  );
  Cursor cursor(pkt->buf());
  ICMPHdr icmpv6Hdr(cursor);
  EXPECT_EQ(type, icmpv6Hdr.type);
  EXPECT_EQ(code, icmpv6Hdr.code);
  EXPECT_EQ(csum, icmpv6Hdr.csum);
}

TEST(ICMPHdrTest, cursor_data_constructor_too_small) {
  auto pkt = MockRxPacket::fromHex(
      // ICMPv6 Header
      "80" // Type: Echo Request
      "00" // Code: 0
      "00   " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ ICMPHdr icmpv6Hdr(cursor); }, HdrParseError);
}

TEST(ICMPHdrTest, assignment_operator) {
  uint8_t type = static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST);
  uint8_t code = 0;
  uint16_t csum = 0; // Obviously wrong
  ICMPHdr lhs(type, code, csum);
  ICMPHdr rhs = lhs;
  EXPECT_EQ(lhs, rhs);
}

TEST(ICMPHdrTest, equality_operator) {
  uint8_t type = static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST);
  uint8_t code = 02;
  uint16_t csum = 0; // Obviously wrong
  ICMPHdr lhs(type, code, csum);
  ICMPHdr rhs(type, code, csum);
  EXPECT_EQ(lhs, rhs);
}

TEST(ICMPHdrTest, inequality_operator) {
  uint8_t type = static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST);
  uint8_t code = 0;
  uint16_t csum1 = 0; // Obviously wrong
  uint16_t csum2 = 1; // Obviously wrong
  ICMPHdr lhs(type, code, csum1);
  ICMPHdr rhs(type, code, csum2);
  EXPECT_NE(lhs, rhs);
}
