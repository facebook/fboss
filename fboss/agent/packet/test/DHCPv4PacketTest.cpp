/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/DHCPv4Packet.h"
#include <folly/Conv.h>
#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <algorithm>
#include <memory>
#include <string>
#include "fboss/agent/DHCPv4OptionsOfInterest.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/test/CounterCache.h"

using namespace facebook::fboss;
using folly::IOBuf;
using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Appender;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::string;
using std::unique_ptr;
using std::vector;

DHCPv4Packet makeDHCPPacket() {
  // Create an DHCP pkt without L2, IP or UDP
  auto pkt = MockRxPacket::fromHex(
      // op(1),htype(1), hlen(6), hops(1)
      "01  01  06  01"
      // xid (10.10.10.1)
      "0a  0a  0a  01"
      // secs(10) flags(0x8000)
      "00  0a  80  00"
      // ciaddr (10.10.10.2)
      "0a  0a  0a  02"
      // yiaddr (10.10.10.3)
      "0a  0a  0a  03"
      // siaddr (10.10.10.4)
      "0a  0a  0a  04"
      // giaddr (10.10.10.5)
      "0a  0a  0a  05"
      // Chaddr (01 02 03 04 05 06)
      "01  02  03  04"
      "05  06  00  00"
      "00  00  00  00"
      "00  00  00  00"
      // Sname (abcd)
      "61  62  63  64"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      // File (defg)
      "65  66  67  68"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      "00  00  00  00"
      // DHCP Cookie {99, 130, 83, 99};
      "63  82  53 63"
      // DHCP Message type option (DHCP discover message) + 1 byte pad
      "35  01  01 00"
      // 3 X Pad, 1 X end
      "00  00  00 ff");
  Cursor cursor(pkt->buf());
  DHCPv4Packet dhcp;
  dhcp.parse(&cursor);
  return dhcp;
}

TEST(DHCPv4PacketTest, Parse) {
  auto dhcp = makeDHCPPacket();

  EXPECT_EQ(1, dhcp.op);
  EXPECT_EQ(1, dhcp.htype);
  EXPECT_EQ(6, dhcp.hlen);
  EXPECT_EQ(IPAddressV4("10.10.10.1"), dhcp.xid);
  EXPECT_EQ(10, dhcp.secs);
  EXPECT_EQ(0x8000, dhcp.flags);
  EXPECT_EQ(IPAddressV4("10.10.10.2"), dhcp.ciaddr);
  EXPECT_EQ(IPAddressV4("10.10.10.3"), dhcp.yiaddr);
  EXPECT_EQ(IPAddressV4("10.10.10.4"), dhcp.siaddr);
  EXPECT_EQ(IPAddressV4("10.10.10.5"), dhcp.giaddr);
  uint8_t chaddr[MacAddress::SIZE];
  memcpy(chaddr, dhcp.chaddr.data(), MacAddress::SIZE);
  MacAddress chaddrMac =
      MacAddress::fromBinary(folly::ByteRange(chaddr, MacAddress::SIZE));
  EXPECT_EQ(MacAddress("010203040506"), chaddrMac);
  EXPECT_EQ(string("abcd"), string((char*)(dhcp.sname.data())));
  EXPECT_EQ(string("efgh"), string((char*)(dhcp.file.data())));
  EXPECT_TRUE(std::equal(
      dhcp.dhcpCookie.begin(),
      dhcp.dhcpCookie.end(),
      DHCPv4Packet::kOptionsCookie));

  auto smallPacket = MockRxPacket::fromHex("00 00 00 00");
  Cursor cursor2(smallPacket->buf());
  EXPECT_THROW(dhcp.parse(&cursor2), FbossError);
}

TEST(DHCPv4Packet, Equality) {
  auto dhcp1 = makeDHCPPacket();
  auto dhcp2 = makeDHCPPacket();
  EXPECT_EQ(dhcp1, dhcp2);
}

TEST(DHCPv4Packet, Write) {
  auto dhcp1 = makeDHCPPacket();
  auto buf = IOBuf::create(0);
  Appender appender(buf.get(), 100);
  dhcp1.write(&appender);
  EXPECT_EQ(buf->computeChainDataLength(), dhcp1.size());
  DHCPv4Packet dhcp2;
  Cursor cursor(buf.get());
  dhcp2.parse(&cursor);
  EXPECT_EQ(dhcp1, dhcp2);
}

TEST(DHCPv4Packet, getOption) {
  auto dhcpPkt = makeDHCPPacket();
  vector<uint8_t> optData;
  EXPECT_TRUE(DHCPv4Packet::getOptionSlow(
      DHCPv4OptionsOfInterest::DHCP_MESSAGE_TYPE, dhcpPkt.options, optData));
  EXPECT_EQ(1, optData.size());
  EXPECT_EQ(1, optData[0]);

  optData.clear();
  EXPECT_TRUE(DHCPv4Packet::getOptionSlow(
      DHCPv4OptionsOfInterest::PAD, dhcpPkt.options, optData));
  EXPECT_EQ(0, optData.size());

  optData.clear();
  EXPECT_TRUE(DHCPv4Packet::getOptionSlow(
      DHCPv4OptionsOfInterest::END, dhcpPkt.options, optData));
  EXPECT_EQ(0, optData.size());

  optData.clear();
  EXPECT_FALSE(DHCPv4Packet::getOptionSlow(
      DHCPv4OptionsOfInterest::DHCP_AGENT_OPTIONS, dhcpPkt.options, optData));
  EXPECT_EQ(0, optData.size());
}

TEST(DHCPv4Handler, pad) {
  auto dhcpPkt = makeDHCPPacket();
  auto origSize = dhcpPkt.size();
  EXPECT_LT(origSize, DHCPv4Packet::kMinSize);
  EXPECT_LT(dhcpPkt.size(), DHCPv4Packet::kMinSize);
  dhcpPkt.padToMinLength();
  EXPECT_EQ(dhcpPkt.size(), DHCPv4Packet::kMinSize);

  IOBuf buf(IOBuf::CREATE, DHCPv4Packet::kMinSize);
  buf.append(DHCPv4Packet::kMinSize);
  RWPrivateCursor cursor(&buf);
  dhcpPkt.write(&cursor);
  for (size_t n = origSize; n < DHCPv4Packet::kMinSize; ++n) {
    EXPECT_EQ(buf.data()[n], 0) << "offset " << n;
  }
}
