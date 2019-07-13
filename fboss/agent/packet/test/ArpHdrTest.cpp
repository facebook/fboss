/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/ArpHdr.h"

/*
 * References:
 *   https://code.google.com/p/googletest/wiki/Primer
 *   https://code.google.com/p/googletest/wiki/AdvancedGuide
 */
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Cursor;
using std::unique_ptr;

TEST(ArpHdrTest, default_constructor) {
  ArpHdr arpHdr;
  EXPECT_EQ(0, arpHdr.htype);
  EXPECT_EQ(0, arpHdr.ptype);
  EXPECT_EQ(0, arpHdr.hlen);
  EXPECT_EQ(0, arpHdr.plen);
  EXPECT_EQ(0, arpHdr.oper);
  EXPECT_EQ(MacAddress(), arpHdr.sha);
  EXPECT_EQ(IPAddressV4("0.0.0.0"), arpHdr.spa);
  EXPECT_EQ(MacAddress(), arpHdr.tha);
  EXPECT_EQ(IPAddressV4("0.0.0.0"), arpHdr.tpa);
}

TEST(ArpHdrTest, copy_constructor) {
  uint16_t htype = static_cast<uint16_t>(ARP_HTYPE::ARP_HTYPE_ETHERNET);
  uint16_t ptype = static_cast<uint16_t>(ARP_PTYPE::ARP_PTYPE_IPV4);
  uint8_t hlen = static_cast<uint8_t>(ARP_HLEN::ARP_HLEN_ETHERNET);
  uint8_t plen = static_cast<uint8_t>(ARP_PLEN::ARP_PLEN_IPV4);
  uint16_t oper = static_cast<uint16_t>(ARP_OPER::ARP_OPER_REQUEST);
  MacAddress sha("10:dd:b1:bb:5a:ef");
  IPAddressV4 spa("10.0.0.15");
  MacAddress tha("ff:ff:ff:ff:ff:ff");
  IPAddressV4 tpa("10.0.0.1");
  ArpHdr lhs(htype, ptype, hlen, plen, oper, sha, spa, tha, tpa);
  ArpHdr rhs(lhs);
  EXPECT_EQ(lhs, rhs);
}

TEST(ArpHdrTest, parameterized_data_constructor) {
  uint16_t htype = static_cast<uint16_t>(ARP_HTYPE::ARP_HTYPE_ETHERNET);
  uint16_t ptype = static_cast<uint16_t>(ARP_PTYPE::ARP_PTYPE_IPV4);
  uint8_t hlen = static_cast<uint8_t>(ARP_HLEN::ARP_HLEN_ETHERNET);
  uint8_t plen = static_cast<uint8_t>(ARP_PLEN::ARP_PLEN_IPV4);
  uint16_t oper = static_cast<uint16_t>(ARP_OPER::ARP_OPER_REQUEST);
  MacAddress sha("10:dd:b1:bb:5a:ef");
  IPAddressV4 spa("10.0.0.15");
  MacAddress tha("ff:ff:ff:ff:ff:ff");
  IPAddressV4 tpa("10.0.0.1");
  ArpHdr arpHdr(htype, ptype, hlen, plen, oper, sha, spa, tha, tpa);
  EXPECT_EQ(htype, arpHdr.htype);
  EXPECT_EQ(ptype, arpHdr.ptype);
  EXPECT_EQ(hlen, arpHdr.hlen);
  EXPECT_EQ(plen, arpHdr.plen);
  EXPECT_EQ(oper, arpHdr.oper);
  EXPECT_EQ(sha, arpHdr.sha);
  EXPECT_EQ(spa, arpHdr.spa);
  EXPECT_EQ(tha, arpHdr.tha);
  EXPECT_EQ(tpa, arpHdr.tpa);
}

TEST(ArpHdrTest, cursor_data_constructor) {
  uint16_t htype = static_cast<uint16_t>(ARP_HTYPE::ARP_HTYPE_ETHERNET);
  uint16_t ptype = static_cast<uint16_t>(ARP_PTYPE::ARP_PTYPE_IPV4);
  uint8_t hlen = static_cast<uint8_t>(ARP_HLEN::ARP_HLEN_ETHERNET);
  uint8_t plen = static_cast<uint8_t>(ARP_PLEN::ARP_PLEN_IPV4);
  uint16_t oper = static_cast<uint16_t>(ARP_OPER::ARP_OPER_REQUEST);
  MacAddress sha("10:dd:b1:bb:5a:ef");
  IPAddressV4 spa("10.0.0.15");
  MacAddress tha("ff:ff:ff:ff:ff:ff");
  IPAddressV4 tpa("10.0.0.1");
  auto pkt = MockRxPacket::fromHex(
      // ARP Header
      "00 01" // HTYPE: Ethernet (1)
      "08 00" // PTYPE: IPv4 (0x0800)
      "06" // HLEN:  6
      "04" // PLEN:  4
      "00 01" // OPER:  Request
      "10 dd b1 bb 5a ef" // Sender Hardware Address
      "0a 00 00 0f" // Sender Protocol Address: 10.0.0.15
      "ff ff ff ff ff ff" // Target Hardware Address
      "0a 00 00 01" // Target Protocol Address: 10.0.0.1
  );
  Cursor cursor(pkt->buf());
  ArpHdr arpHdr(cursor);
  EXPECT_EQ(htype, arpHdr.htype);
  EXPECT_EQ(ptype, arpHdr.ptype);
  EXPECT_EQ(hlen, arpHdr.hlen);
  EXPECT_EQ(plen, arpHdr.plen);
  EXPECT_EQ(oper, arpHdr.oper);
  EXPECT_EQ(sha, arpHdr.sha);
  EXPECT_EQ(spa, arpHdr.spa);
  EXPECT_EQ(tha, arpHdr.tha);
  EXPECT_EQ(tpa, arpHdr.tpa);
}

TEST(ArpHdrTest, cursor_data_constructor_too_small) {
  auto pkt = MockRxPacket::fromHex(
      // ARP Header
      "00 01" // HTYPE: Ethernet (1)
      "08 00" // PTYPE: IPv4 (0x0800)
      "06" // HLEN:  6
      "04" // PLEN:  4
      "00 01" // OPER:  Request
      "10 dd b1 bb 5a ef" // Sender Hardware Address
      "0a 00 00 0f" // Sender Protocol Address: 10.0.0.15
      "ff ff ff ff ff ff" // Target Hardware Address
      "0a 00 00   " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ ArpHdr arpHdr(cursor); }, HdrParseError);
}

TEST(ArpHdrTest, assignment_operator) {
  uint16_t htype = static_cast<uint16_t>(ARP_HTYPE::ARP_HTYPE_ETHERNET);
  uint16_t ptype = static_cast<uint16_t>(ARP_PTYPE::ARP_PTYPE_IPV4);
  uint8_t hlen = static_cast<uint8_t>(ARP_HLEN::ARP_HLEN_ETHERNET);
  uint8_t plen = static_cast<uint8_t>(ARP_PLEN::ARP_PLEN_IPV4);
  uint16_t oper = static_cast<uint16_t>(ARP_OPER::ARP_OPER_REQUEST);
  MacAddress sha("10:dd:b1:bb:5a:ef");
  IPAddressV4 spa("10.0.0.15");
  MacAddress tha("ff:ff:ff:ff:ff:ff");
  IPAddressV4 tpa("10.0.0.1");
  ArpHdr lhs(htype, ptype, hlen, plen, oper, sha, spa, tha, tpa);
  ArpHdr rhs = lhs;
  EXPECT_EQ(lhs, rhs);
}

TEST(ArpHdrTest, equality_operator) {
  uint16_t htype = static_cast<uint16_t>(ARP_HTYPE::ARP_HTYPE_ETHERNET);
  uint16_t ptype = static_cast<uint16_t>(ARP_PTYPE::ARP_PTYPE_IPV4);
  uint8_t hlen = static_cast<uint8_t>(ARP_HLEN::ARP_HLEN_ETHERNET);
  uint8_t plen = static_cast<uint8_t>(ARP_PLEN::ARP_PLEN_IPV4);
  uint16_t oper = static_cast<uint16_t>(ARP_OPER::ARP_OPER_REQUEST);
  MacAddress sha("10:dd:b1:bb:5a:ef");
  IPAddressV4 spa("10.0.0.15");
  MacAddress tha("ff:ff:ff:ff:ff:ff");
  IPAddressV4 tpa("10.0.0.1");
  ArpHdr lhs(htype, ptype, hlen, plen, oper, sha, spa, tha, tpa);
  ArpHdr rhs(htype, ptype, hlen, plen, oper, sha, spa, tha, tpa);
  EXPECT_EQ(lhs, rhs);
}

TEST(ArpHdrTest, inequality_operator) {
  uint16_t htype = static_cast<uint16_t>(ARP_HTYPE::ARP_HTYPE_ETHERNET);
  uint16_t ptype = static_cast<uint16_t>(ARP_PTYPE::ARP_PTYPE_IPV4);
  uint8_t hlen = static_cast<uint8_t>(ARP_HLEN::ARP_HLEN_ETHERNET);
  uint8_t plen = static_cast<uint8_t>(ARP_PLEN::ARP_PLEN_IPV4);
  uint16_t oper = static_cast<uint16_t>(ARP_OPER::ARP_OPER_REQUEST);
  MacAddress sha("10:dd:b1:bb:5a:ef");
  IPAddressV4 spa("10.0.0.15");
  MacAddress tha("ff:ff:ff:ff:ff:ff");
  IPAddressV4 tpa1("10.0.0.1");
  IPAddressV4 tpa2("10.0.0.2");
  ArpHdr lhs(htype, ptype, hlen, plen, oper, sha, spa, tha, tpa1);
  ArpHdr rhs(htype, ptype, hlen, plen, oper, sha, spa, tha, tpa2);
  EXPECT_NE(lhs, rhs);
}
