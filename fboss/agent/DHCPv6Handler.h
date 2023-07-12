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
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <stdint.h>
#include <memory>
#include "fboss/agent/packet/DHCPv6Packet.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwSwitch;
class RxPacket;
class UDPHeader;
class DHCPv6Packet;
class TxPacket;
class IPv6Hdr;

class DHCPv6Handler {
 public:
  enum { MAX_RELAY_HOPCOUNT = 10 };

  static bool isForDHCPv6RelayOrServer(const UDPHeader& udpHdr);

  template <typename VlanOrIntfT>
  static void handlePacket(
      SwSwitch* sw,
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      const IPv6Hdr& ipHdr,
      const UDPHeader& udpHdr,
      folly::io::Cursor cursor,
      const std::shared_ptr<VlanOrIntfT>& vlanOrIntf);

 private:
  /**
   * process DHCPv6 packet from client and send relay forward
   */
  template <typename VlanOrIntfT>
  static void processDHCPv6Packet(
      SwSwitch* sw,
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      const IPv6Hdr& ipHdr,
      const DHCPv6Packet& dhcpPacket,
      const std::shared_ptr<VlanOrIntfT>& vlanOrIntf);

  /**
   * process relay reply from server or relay forward message from other agents
   */
  static void processDHCPv6RelayForward(
      SwSwitch* sw,
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      const IPv6Hdr& ipHdr,
      DHCPv6Packet& dhcpPacket);

  static void processDHCPv6RelayReply(
      SwSwitch* sw,
      std::unique_ptr<RxPacket> pkt,
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      const IPv6Hdr& ipHdr,
      DHCPv6Packet& dhcpPacket);
};

} // namespace facebook::fboss
