// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketUtils.h"

#include "fboss/agent/packet/PktUtil.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <gtest/gtest.h>
#include <memory>

using namespace ::testing;
using namespace facebook::fboss;

namespace {
constexpr auto kUDP =
    "c6 7c 1a 73"
    "00 18 5c 75"
    "de ad be ef"
    "de ad be ef"
    "de ad be ef"
    "de ad be ef";
void verifyUdp(const UDPHeader& udpHdr, const std::vector<uint8_t>& payload) {
  EXPECT_EQ(udpHdr.srcPort, 0xc67c);
  EXPECT_EQ(udpHdr.dstPort, 0x1a73);
  EXPECT_EQ(udpHdr.csum, 0x5c75);
  EXPECT_EQ(udpHdr.length, 0x18);
  EXPECT_EQ(payload.size(), 0x10);
  for (auto i = 0; i < 0x10; i += 4) {
    EXPECT_EQ(payload[i], 0xde);
    EXPECT_EQ(payload[i + 1], 0xad);
    EXPECT_EQ(payload[i + 2], 0xbe);
    EXPECT_EQ(payload[i + 3], 0xef);
  }
}

constexpr auto kIPv4Hdr =
    "45 c0 00 1d"
    "1b 9d 00 00"
    "ff 11 ca 1e"
    "0a 7a 60 24"
    "0a 7a 60 25";

void verifyIP4Hdr(const IPv4Hdr& ip4Hdr) {
  EXPECT_EQ(ip4Hdr.ihl, 0x05);
  EXPECT_EQ(ip4Hdr.dscp, 48);
  EXPECT_EQ(ip4Hdr.ecn, 0);
  EXPECT_EQ(ip4Hdr.length, 0x1d);
  EXPECT_EQ(ip4Hdr.id, 0x1b9d);
  EXPECT_EQ(ip4Hdr.fragmentOffset, 0);
  EXPECT_EQ(ip4Hdr.ttl, 0xff);
  EXPECT_EQ(ip4Hdr.protocol, 0x11);
  EXPECT_EQ(ip4Hdr.csum, 0xca1e);
  EXPECT_EQ(ip4Hdr.srcAddr, folly::IPAddressV4("10.122.96.36"));
  EXPECT_EQ(ip4Hdr.dstAddr, folly::IPAddressV4("10.122.96.37"));
}

constexpr auto kIPv6Hdr =
    "6c 00 00 00"
    "00 18 11 ff"
    "24 01 db 00"
    "e2 11 91 00"
    "10 20 00 00"
    "00 00 00 24"
    "24 01 db 00"
    "e2 11 91 00"
    "10 20 00 00"
    "00 00 00 25";

void verifyIP6Hdr(const IPv6Hdr& ip6Hdr) {
  EXPECT_EQ(ip6Hdr.trafficClass, 0xc0);
  EXPECT_EQ(ip6Hdr.payloadLength, 0x18);
  EXPECT_EQ(ip6Hdr.nextHeader, 0x11);
  EXPECT_EQ(ip6Hdr.hopLimit, 0xff);
  EXPECT_EQ(ip6Hdr.srcAddr, folly::IPAddressV6("2401:db00:e211:9100:1020::24"));
  EXPECT_EQ(ip6Hdr.dstAddr, folly::IPAddressV6("2401:db00:e211:9100:1020::25"));
}
} // anonymous namespace

TEST(PacketUtilTests, UDPDatagram) {
  auto buf = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(kUDP));
  folly::io::Cursor cursor{buf.get()};

  auto udpPkt = utility::UDPDatagram(cursor);
  verifyUdp(udpPkt.header(), udpPkt.payload());
}

TEST(PacketUtilTests, IpV6Packet) {
  auto ipv6 = std::string(kIPv6Hdr);
  ipv6.append(kUDP);

  auto buf = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(ipv6));
  folly::io::Cursor cursor{buf.get()};

  auto ip6Pkt = utility::IPv6Packet(cursor);
  auto ip6Hdr = ip6Pkt.header();
  auto ip6Payload = ip6Pkt.payload();
  verifyIP6Hdr(ip6Hdr);
  auto udpPkt = ip6Payload.value();
  verifyUdp(udpPkt.header(), udpPkt.payload());
}

TEST(PacketUtilTests, IpV4Packet) {
  auto ipv4 = std::string(kIPv4Hdr);
  ipv4.append(kUDP);

  auto buf = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(ipv4));
  folly::io::Cursor cursor{buf.get()};

  auto ip4Pkt = utility::IPv4Packet(cursor);
  auto ip4Hdr = ip4Pkt.header();
  auto ip4Payload = ip4Pkt.payload();
  verifyIP4Hdr(ip4Hdr);
  auto udpPkt = ip4Payload.value();
  verifyUdp(udpPkt.header(), udpPkt.payload());
}
