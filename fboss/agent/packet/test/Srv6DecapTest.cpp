// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"

using namespace facebook::fboss;

TEST(Srv6DecapTest, decapsulateV6InV6Packet) {
  // IPv6-in-IPv6 packet: [Eth + VLAN | outer IPv6 (nextHdr=0x29) | inner IPv6 +
  // UDP]
  auto ioBuf = PktUtil::parseHexData(
      // dst mac, src mac, vlan-type
      "44 4c a8 e4 19 e3 02 90 fb 43 a2 ec 81 00"
      // vlan tag and IPv6 ethertype
      "0f a1 86 dd"
      // outer IPv6 header (40 bytes)
      // version=6, tc=0, flow=0, payload=48, nextHdr=0x29(IPv6), hopLimit=64
      "60 00 00 00 00 30 29 40"
      // outer src: 1::1
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 01"
      // outer dst: 3001:db8:7fff:: (MySID)
      "30 01 0d b8 7f ff 00 00 00 00 00 00 00 00 00 00"
      // inner IPv6 header (40 bytes)
      // version=6, tc=0, flow=0, payload=8, nextHdr=0x11(UDP), hopLimit=64
      "60 00 00 00 00 08 11 40"
      // inner src: 1::10
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 10"
      // inner dst: 1:: (our interface IP)
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
      // UDP: src=9000, dst=9001, len=8, checksum=0
      "23 28 23 29 00 08 00 00");

  // L2 header size: 6(dst) + 6(src) + 4(vlan) + 2(ethertype) = 18
  size_t l2HeaderSize = 18;
  PktUtil::decapsulatePacket(
      &ioBuf,
      l2HeaderSize,
      IPv6Hdr::SIZE,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));

  // After decap: same L2 header + inner IPv6 + UDP
  auto expectedIOBuf = PktUtil::parseHexData(
      // dst mac, src mac, vlan-type (preserved)
      "44 4c a8 e4 19 e3 02 90 fb 43 a2 ec 81 00"
      // vlan tag and IPv6 ethertype (preserved)
      "0f a1 86 dd"
      // inner IPv6 header (outer stripped)
      "60 00 00 00 00 08 11 40"
      // inner src: 1::10
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 10"
      // inner dst: 1::
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
      // UDP
      "23 28 23 29 00 08 00 00");

  EXPECT_TRUE(folly::IOBufEqualTo()(ioBuf, expectedIOBuf));
}

TEST(Srv6DecapTest, decapsulateV4InV6Packet) {
  // IPv4-in-IPv6 packet: [Eth + VLAN | outer IPv6 (nextHdr=0x04) | inner IPv4 +
  // UDP]
  auto ioBuf = PktUtil::parseHexData(
      // dst mac, src mac, vlan-type
      "44 4c a8 e4 19 e3 02 90 fb 43 a2 ec 81 00"
      // vlan tag and IPv6 ethertype
      "0f a1 86 dd"
      // outer IPv6 header (40 bytes)
      // version=6, tc=0, flow=0, payload=28, nextHdr=0x04(IPv4), hopLimit=64
      "60 00 00 00 00 1c 04 40"
      // outer src: 1::1
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 01"
      // outer dst: 3001:db8:7fff:: (MySID)
      "30 01 0d b8 7f ff 00 00 00 00 00 00 00 00 00 00"
      // inner IPv4 header (20 bytes)
      // version=4, ihl=5, tos=0, len=28, id=0, flags=0, ttl=64, proto=17(UDP)
      "45 00 00 1c 00 00 00 00 40 11 00 00"
      // inner src: 10.0.0.1
      "0a 00 00 01"
      // inner dst: 1.1.1.1
      "01 01 01 01"
      // UDP: src=9000, dst=9001, len=8, checksum=0
      "23 28 23 29 00 08 00 00");

  size_t l2HeaderSize = 18;
  PktUtil::decapsulatePacket(
      &ioBuf,
      l2HeaderSize,
      IPv6Hdr::SIZE,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));

  // After decap: L2 header with ethertype changed to IPv4 + inner IPv4 + UDP
  auto expectedIOBuf = PktUtil::parseHexData(
      // dst mac, src mac, vlan-type (preserved)
      "44 4c a8 e4 19 e3 02 90 fb 43 a2 ec 81 00"
      // vlan tag and IPv4 ethertype (rewritten)
      "0f a1 08 00"
      // inner IPv4 header (outer IPv6 stripped)
      "45 00 00 1c 00 00 00 00 40 11 00 00"
      "0a 00 00 01"
      "01 01 01 01"
      // UDP
      "23 28 23 29 00 08 00 00");

  EXPECT_TRUE(folly::IOBufEqualTo()(ioBuf, expectedIOBuf));
}
