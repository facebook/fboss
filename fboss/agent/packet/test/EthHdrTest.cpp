/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/EthHdr.h"

/*
 * References:
 *   https://code.google.com/p/googletest/wiki/Primer
 *   https://code.google.com/p/googletest/wiki/AdvancedGuide
 */
#include <gtest/gtest.h>

#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::Cursor;
using std::vector;

TEST(EthHdrTest, default_constructor) {
  EthHdr ethHdr;
  EXPECT_EQ(MacAddress(), ethHdr.dstAddr);
  EXPECT_EQ(MacAddress(), ethHdr.srcAddr);
  EXPECT_EQ(vector<VlanTag>(), ethHdr.vlanTags);
  EXPECT_EQ(0, ethHdr.etherType);
}

TEST(EthHdrTest, copy_constructor) {
  MacAddress dstAddr("ff:ff:ff:ff:ff:ff");
  MacAddress srcAddr("10:dd:b1:bb:5a:ef");
  vector<VlanTag> vlanTags;
  vlanTags.push_back(VlanTag(0x81000001));
  vlanTags.push_back(VlanTag(0x88A80002));
  vlanTags.push_back(VlanTag(0x88A80003));
  uint16_t etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4);
  EthHdr lhs(dstAddr, srcAddr, vlanTags, etherType);
  EthHdr rhs(lhs);
  EXPECT_EQ(lhs, rhs);
}

TEST(EthHdrTest, parameterized_data_constructor) {
  MacAddress dstAddr("ff:ff:ff:ff:ff:ff");
  MacAddress srcAddr("10:dd:b1:bb:5a:ef");
  vector<VlanTag> vlanTags;
  vlanTags.push_back(VlanTag(0x81000001));
  vlanTags.push_back(VlanTag(0x88A80002));
  vlanTags.push_back(VlanTag(0x88A80003));
  uint16_t etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6);
  EthHdr ethHdr(dstAddr, srcAddr, vlanTags, etherType);
  EXPECT_EQ(dstAddr, ethHdr.dstAddr);
  EXPECT_EQ(srcAddr, ethHdr.srcAddr);
  EXPECT_EQ(vlanTags, ethHdr.vlanTags);
  EXPECT_EQ(etherType, ethHdr.etherType);
}

TEST(EthHdrTest, cursor_data_constructor) {
  MacAddress dstAddr("ff:ff:ff:ff:ff:ff");
  MacAddress srcAddr("10:dd:b1:bb:5a:ef");
  vector<VlanTag> vlanTags;
  vlanTags.push_back(VlanTag(0x81000001));
  vlanTags.push_back(VlanTag(0x88A80002));
  vlanTags.push_back(VlanTag(0x88A80003));
  uint16_t etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6);
  auto pkt = MockRxPacket::fromHex(
      // Ethernet Header
      "ff ff ff ff ff ff" // Destination MAC Address
      "10 dd b1 bb 5a ef" // Source MAC Address
      "81 00 00 01" // VLAN: 1
      "88 A8 00 02" // VLAN: 2
      "88 A8 00 03" // VLAN: 3
      "86 DD" // EtherType: IPv6
  );
  Cursor cursor(pkt->buf());
  EthHdr ethHdr(cursor);
  EXPECT_EQ(dstAddr, ethHdr.dstAddr);
  EXPECT_EQ(srcAddr, ethHdr.srcAddr);
  EXPECT_EQ(vlanTags, ethHdr.vlanTags);
  EXPECT_EQ(etherType, ethHdr.etherType);
}

TEST(EthHdrTest, cursor_data_constructor_too_small_0) {
  auto pkt = MockRxPacket::fromHex(
      // Ethernet Header
      "ff ff ff ff ff ff" // Destination MAC Address
      "10 dd b1 bb 5a ef" // Source MAC Address
      "86   " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ EthHdr ethHdr(cursor); }, HdrParseError);
}

TEST(EthHdrTest, cursor_data_constructor_too_small_1) {
  auto pkt = MockRxPacket::fromHex(
      // Ethernet Header
      "ff ff ff ff ff ff" // Destination MAC Address
      "10 dd b1 bb 5a ef" // Source MAC Address
      "81 00 00 01" // VLAN: 1
      "88 A8 00 02" // VLAN: 2
      "88 A8 00 03" // VLAN: 3
      "86   " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ EthHdr ethHdr(cursor); }, HdrParseError);
}

TEST(EthHdrTest, assignment_operator) {
  MacAddress dstAddr("ff:ff:ff:ff:ff:ff");
  MacAddress srcAddr("10:dd:b1:bb:5a:ef");
  vector<VlanTag> vlanTags;
  vlanTags.push_back(VlanTag(0x81000001));
  vlanTags.push_back(VlanTag(0x88A80002));
  vlanTags.push_back(VlanTag(0x88A80003));
  uint16_t etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4);
  EthHdr lhs(dstAddr, srcAddr, vlanTags, etherType);
  EthHdr rhs = lhs;
  EXPECT_EQ(lhs, rhs);
}

TEST(EthHdrTest, equality_operator) {
  MacAddress dstAddr("ff:ff:ff:ff:ff:ff");
  MacAddress srcAddr("10:dd:b1:bb:5a:ef");
  vector<VlanTag> vlanTags;
  vlanTags.push_back(VlanTag(0x81000001));
  vlanTags.push_back(VlanTag(0x88A80002));
  vlanTags.push_back(VlanTag(0x88A80003));
  uint16_t etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4);
  EthHdr lhs(dstAddr, srcAddr, vlanTags, etherType);
  EthHdr rhs(dstAddr, srcAddr, vlanTags, etherType);
  EXPECT_EQ(lhs, rhs);
}

TEST(EthHdrTest, inequality_operator) {
  MacAddress dstAddr("ff:ff:ff:ff:ff:ff");
  MacAddress srcAddr("10:dd:b1:bb:5a:ef");
  vector<VlanTag> vlanTags;
  vlanTags.push_back(VlanTag(0x81000001));
  vlanTags.push_back(VlanTag(0x88A80002));
  vlanTags.push_back(VlanTag(0x88A80003));
  uint16_t etherType1 = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4);
  uint16_t etherType2 = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6);
  EthHdr lhs(dstAddr, srcAddr, vlanTags, etherType1);
  EthHdr rhs(dstAddr, srcAddr, vlanTags, etherType2);
  EXPECT_NE(lhs, rhs);
}

TEST(EthHdrTest, vlan_tag) {
  // Vlan id has bits in both octets set.
  // Drop eligibility is 0
  VlanTag vlan1(2050, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN), 0, 4);
  EXPECT_EQ(vlan1.vid(), 2050);
  EXPECT_EQ(vlan1.tpid(), static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN));
  EXPECT_EQ(vlan1.dei(), 0);
  EXPECT_EQ(vlan1.pcp(), 4);

  // Vlan id has set bits in only one octets
  // Drop eligibility is 1
  VlanTag vlan2(50, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_QINQ), 1, 0);
  EXPECT_EQ(vlan2.vid(), 50);
  EXPECT_EQ(vlan2.tpid(), static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_QINQ));
  EXPECT_EQ(vlan2.dei(), 1);
  EXPECT_EQ(vlan2.pcp(), 0);
}
