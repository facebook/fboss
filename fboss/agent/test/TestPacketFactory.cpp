/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/TestPacketFactory.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/MoveWrapper.h>
#include <folly/String.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/PktUtil.h"

#include <memory>

using folly::IOBuf;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
/**
 * Some constants for easy creation of packets
 */

const std::string
    kL2PacketHeader = // Source MAC, Destination MAC (will be overwritten)
    "00 00 00 00 00 00  00 00 00 00 00 00"
    // 802.1q, VLAN 0 (Not used)
    "81 00  00 00"
    // Next header code (Will be overwritten)
    "00 00";

// length 20 bytes
const std::string kIPv4PacketHeader = // Version(4), IHL(5), DSCP(0), ECN(0),
                                      // Total Length(20 + 123)
    "45  00  00 8F"
    // Identification(0), Flags(0), Fragment offset(0)
    "00 00  00 00"
    // TTL(31), Protocol(11, UDP), Checksum (0, fake)
    "1F  11  00 00"
    // Source IP (will be overwritten)
    "00 00 00 00"
    // Destination IP (will be overwritten)
    "00 00 00 00";

// length 24 bytes
const std::string kIPv6PacketHeader = // Version 6, traffic class, flow label
    "6e 00 00 00"
    // Payload length (123)
    "00 7b"
    // Next header: 17 (UDP), Hop Limit (255)
    "11 ff"
    // Source IP (wil be overwritten)
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
    // Destination IP (will be overwritten)
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00";

// length 123 bytes
const std::string kPayload =
    "1a 0a 1a 0a 00 7b 58 75"
    "1c 1c 68 0a 74 65 72 72 61 67 72 61 70 68 08 02"
    "0b 63 73 77 32 33 61 2e 73 6e 63 31 15 e0 d4 03"
    "18 20 08 87 b2 a8 6f e8 48 71 f7 35 93 38 b6 8e"
    "b2 df de 66 e1 e5 9b 32 da 95 8a a7 51 4e 78 eb"
    "9e 14 1c 18 10 fe 80 00 00 00 00 00 00 02 1c 73"
    "ff fe ea d4 73 00 1c 18 04 0a 2e 01 48 00 00 26"
    "c2 9a c6 03 1b 00 16 84 c0 d5 a9 ac f9 9d 05 00"
    "18 00 00";
} // namespace

namespace facebook::fboss {

folly::IOBuf createV4Packet(
    folly::IPAddressV4 srcAddr,
    folly::IPAddressV4 dstAddr,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const std::string& payloadPad) {
  auto pkt = PktUtil::parseHexData(
      kL2PacketHeader + kIPv4PacketHeader + kPayload + payloadPad);

  // Write L2 header
  folly::io::RWPrivateCursor rwCursor(&pkt);
  TxPacket::writeEthHeader(
      &rwCursor, dstMac, srcMac, VlanID(0), IPv4Handler::ETHERTYPE_IPV4);

  // Write L3 src/dst IP Addresses
  rwCursor.skip(12);
  rwCursor.write<uint32_t>(srcAddr.toLong());
  rwCursor.write<uint32_t>(dstAddr.toLong());

  XLOG(DBG2) << "\n" << folly::hexDump(pkt.data(), pkt.length());

  return pkt;
}

folly::IOBuf createV6Packet(
    const folly::IPAddressV6& srcAddr,
    const folly::IPAddressV6& dstAddr,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const std::string& payloadPad) {
  auto pkt = PktUtil::parseHexData(
      kL2PacketHeader + kIPv6PacketHeader + kPayload + payloadPad);

  // Write L2 header
  folly::io::RWPrivateCursor rwCursor(&pkt);
  TxPacket::writeEthHeader(
      &rwCursor, dstMac, srcMac, VlanID(0), IPv6Handler::ETHERTYPE_IPV6);

  // Write L3 src/dst IP addresses
  rwCursor.skip(8);
  rwCursor.push(srcAddr.bytes(), IPAddressV6::byteCount());
  rwCursor.push(dstAddr.bytes(), IPAddressV6::byteCount());

  XLOG(DBG2) << "\n" << folly::hexDump(pkt.data(), pkt.length());

  return pkt;
}

/* Convenience function to copy a buffer to a TxPacket.
 * L2 Header is advanced from RxPacket but headroom is provided in TxPacket.
 */
std::unique_ptr<TxPacket> createTxPacket(
    SwSwitch* sw,
    const folly::IOBuf& buf) {
  auto txPkt = sw->allocatePacket(buf.length());
  auto txBuf = txPkt->buf();
  txBuf->trimStart(EthHdr::SIZE);
  memcpy(
      txBuf->writableData(),
      buf.data() + EthHdr::SIZE,
      buf.length() - EthHdr::SIZE);
  XLOG(DBG2) << "\n" << folly::hexDump(txBuf->data(), txBuf->length());

  return txPkt;
}

} // namespace facebook::fboss
