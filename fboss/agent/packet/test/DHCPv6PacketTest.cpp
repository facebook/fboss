/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/DHCPv6Packet.h"
#include <folly/Conv.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <string>
#include "fboss/agent/DHCPv6Handler.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"

using namespace facebook::fboss;
using folly::IOBuf;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Appender;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::string;
using std::unique_ptr;
using std::vector;

DHCPv6Packet makeDHCPv6Packet() {
  DHCPv6Packet dhcp(static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT), 1000);
  dhcp.addInterfaceIDOption(MacAddress("33:33:33:00:00:01"));
  return dhcp;
}

DHCPv6Packet makeDHCPv6RelayFwdPacket(DHCPv6Packet& dhcpPktIn) {
  IPAddressV6 la("::");
  IPAddressV6 pa("fe08::01:1");
  DHCPv6Packet dhcpPktOut(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD), 0, la, pa);
  dhcpPktOut.addInterfaceIDOption(MacAddress("33:33:33:00:00:01"));
  dhcpPktOut.addRelayMessageOption(dhcpPktIn);
  return dhcpPktOut;
}

TEST(DHCPv6PacketTest, testWrite) {
  auto dhcp = makeDHCPv6Packet();
  EXPECT_EQ(static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT), dhcp.type);
  EXPECT_EQ(1000, dhcp.transactionId);
  EXPECT_EQ(MacAddress::SIZE + 4, dhcp.options.size());

  // serialize the packet
  auto dhcpLen = dhcp.computePacketLength();
  IOBuf buf = IOBuf(IOBuf::CREATE, dhcpLen);
  buf.append(dhcpLen);
  RWPrivateCursor cursor(&buf);
  dhcp.write(&cursor);

  // parse the packet back and compare
  Cursor receiver(&buf);
  DHCPv6Packet dhcp2;
  dhcp2.parse(&receiver);
  std::unordered_set<uint16_t> selector;
  auto options = dhcp2.extractOptions(selector);
  EXPECT_EQ(dhcp.type, dhcp2.type);
  EXPECT_EQ(dhcp.transactionId, dhcp2.transactionId);
  EXPECT_EQ(dhcp.options.size(), dhcp2.options.size());
  EXPECT_EQ(
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID),
      options[0].op);
  EXPECT_EQ(6, options[0].len);
  MacAddress mac = MacAddress::fromBinary(
      folly::ByteRange(options[0].data, MacAddress::SIZE));
  EXPECT_EQ(mac, MacAddress("33:33:33:00:00:01"));
}

TEST(DHCPv6PacketTest, testRelayForward) {
  auto dhcpPktIn = makeDHCPv6Packet();
  auto dhcpRelayFwd = makeDHCPv6RelayFwdPacket(dhcpPktIn);

  // check the relay forward packet
  EXPECT_EQ(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD),
      dhcpRelayFwd.type);
  EXPECT_EQ(0, dhcpRelayFwd.hopCount);
  // interface ID option + relay message option
  auto optionLen = MacAddress::SIZE + 4 + dhcpPktIn.computePacketLength() + 4;
  EXPECT_EQ(optionLen, dhcpRelayFwd.options.size());

  // serialize the relay forward packet
  auto dhcpLen = dhcpRelayFwd.computePacketLength();
  IOBuf buf = IOBuf(IOBuf::CREATE, dhcpLen);
  buf.append(dhcpLen);
  RWPrivateCursor cursor(&buf);
  dhcpRelayFwd.write(&cursor);

  // parse the packet back and compare
  Cursor receiver(&buf);
  DHCPv6Packet dhcp2;
  dhcp2.parse(&receiver);
  EXPECT_EQ(dhcpRelayFwd, dhcp2);

  std::unordered_set<uint16_t> selectAll;
  auto options = dhcp2.extractOptions(selectAll);
  // check the interface id option
  EXPECT_EQ(
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID),
      options[0].op);
  EXPECT_EQ(6, options[0].len);
  MacAddress mac = MacAddress::fromBinary(
      folly::ByteRange(options[0].data, MacAddress::SIZE));
  EXPECT_EQ(mac, MacAddress("33:33:33:00:00:01"));

  // check the relay message option
  EXPECT_EQ(
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_RELAY_MSG),
      options[1].op);
  EXPECT_EQ(dhcpPktIn.computePacketLength(), options[1].len);
  auto buf2 = IOBuf::wrapBuffer(options[1].data, options[1].len);
  Cursor innerCursor(buf2.get());
  DHCPv6Packet innerDhcp;
  innerDhcp.parse(&innerCursor);
  EXPECT_EQ(innerDhcp, dhcpPktIn);
}
