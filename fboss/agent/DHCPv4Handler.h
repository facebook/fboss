/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <stdint.h>
#include <memory>
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwSwitch;
class RxPacket;
class UDPHeader;
class DHCPv4Packet;
class TxPacket;
class IPv4Hdr;

class DHCPv4Handler {
 public:
  enum BootpMsgType { BOOTREQUEST = 1, BOOTREPLY = 2 };
  enum SubOptionsOfInterest { AGENT_CIRCUIT_ID = 1 };
  static constexpr uint16_t kBootPSPort = 67;
  static constexpr uint16_t kBootPCPort = 68;
  static bool isDHCPv4Packet(const UDPHeader& udpHdr);
  static void handlePacket(
      SwSwitch* sw,
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      const IPv4Hdr& ipHdr,
      const UDPHeader& udpHdr,
      folly::io::Cursor cursor);

 private:
  static void processRequest(
      SwSwitch* sw,
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress srcMac,
      const IPv4Hdr& ipHdr,
      const DHCPv4Packet& dhcpPacket);
  static void processReply(
      SwSwitch* sw,
      std::unique_ptr<RxPacket> pkt,
      const IPv4Hdr& ipHdr,
      const DHCPv4Packet& dhcpPacket);
  static bool addAgentOptions(
      SwSwitch* sw,
      PortID port,
      folly::IPAddressV4 relayAddr,
      const DHCPv4Packet& dhcpPacketIn,
      DHCPv4Packet& dhcpPacketOut);
  static bool stripAgentOptions(
      SwSwitch* sw,
      PortID port,
      const DHCPv4Packet& dhcpPacketIn,
      DHCPv4Packet& dhcpPacketOut);
};

} // namespace facebook::fboss
