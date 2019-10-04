// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketUtils.h"

#include "fboss/agent/packet/PktFactory.h"
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

const auto kMPLSHdr =
    "00 12 c8 7e"
    "00 0c 80 7e"
    "00 06 49 7e";

void verifyMPLSHdr(const MPLSHdr& mplsHdr) {
  const std::vector<MPLSHdr::Label> expectedStack{
      MPLSHdr::Label(300, 4, false, 126),
      MPLSHdr::Label(200, 0, false, 126),
      MPLSHdr::Label(100, 4, true, 126),
  };
  EXPECT_EQ(expectedStack, mplsHdr.stack());
}

const auto kEthHdr =
    "02 90 fb 43"
    "a2 ec 02 90"
    "fb 43 a2 ec"
    "81 00 0f a1"
    "88 47";

void verifyEthHdr(const EthHdr& ethHdr) {
  EXPECT_EQ(ethHdr.srcAddr, folly::MacAddress("02:90:fb:43:a2:ec"));
  EXPECT_EQ(ethHdr.dstAddr, folly::MacAddress("02:90:fb:43:a2:ec"));
  EXPECT_EQ(ethHdr.vlanTags, EthHdr::VlanTags_t{VlanTag(4001, 0x8100)});
  EXPECT_EQ(ethHdr.etherType, 0x8847);
}

void verifySerialization(folly::IOBuf* expectBuf, folly::IOBuf* actualBuf) {
  folly::io::Cursor cursorIn(expectBuf);
  folly::io::Cursor cursorOut(actualBuf);

  auto expectSerialize = PktUtil::hexDump(cursorIn);
  auto actualSerialize = PktUtil::hexDump(cursorOut);

  EXPECT_EQ(expectSerialize, actualSerialize);
}
} // anonymous namespace

TEST(PacketUtilTests, UDPDatagram) {
  // test deserialization
  auto udpStr = std::string(kUDP);

  auto bufIn = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(udpStr));
  folly::io::Cursor rCursor{bufIn.get()};

  auto udpPkt = utility::UDPDatagram(rCursor);
  verifyUdp(udpPkt.header(), udpPkt.payload());

  // test serialization
  auto bufOut = folly::IOBuf::create(udpPkt.length());
  bufOut->append(udpPkt.length());

  folly::io::RWPrivateCursor rwCursor{bufOut.get()};
  udpPkt.serialize(rwCursor);

  verifySerialization(bufIn.get(), bufOut.get());
}

TEST(PacketUtilTests, IpV6Packet) {
  // test deserialization
  auto ipv6 = std::string(kIPv6Hdr);
  ipv6.append(kUDP);

  auto bufIn = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(ipv6));
  folly::io::Cursor cursor{bufIn.get()};

  auto ip6Pkt = utility::IPv6Packet(cursor);
  auto ip6Hdr = ip6Pkt.header();
  auto ip6Payload = ip6Pkt.payload();
  verifyIP6Hdr(ip6Hdr);
  auto udpPkt = ip6Payload.value();
  verifyUdp(udpPkt.header(), udpPkt.payload());

  // test serialization
  auto bufOut = folly::IOBuf::create(ip6Pkt.length());
  bufOut->append(ip6Pkt.length());

  folly::io::RWPrivateCursor rwCursor{bufOut.get()};
  ip6Pkt.serialize(rwCursor);

  verifySerialization(bufIn.get(), bufOut.get());
}

TEST(PacketUtilTests, IpV4Packet) {
  // test deserialization
  auto ipv4 = std::string(kIPv4Hdr);
  ipv4.append(kUDP);

  auto bufIn = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(ipv4));
  folly::io::Cursor cursor{bufIn.get()};

  auto ip4Pkt = utility::IPv4Packet(cursor);
  auto ip4Hdr = ip4Pkt.header();
  auto ip4Payload = ip4Pkt.payload();
  verifyIP4Hdr(ip4Hdr);
  auto udpPkt = ip4Payload.value();
  verifyUdp(udpPkt.header(), udpPkt.payload());

  // test serialization
  auto bufOut = folly::IOBuf::create(ip4Pkt.length());
  bufOut->append(ip4Pkt.length());

  folly::io::RWPrivateCursor rwCursor{bufOut.get()};
  ip4Pkt.serialize(rwCursor);

  verifySerialization(bufIn.get(), bufOut.get());
}

TEST(PacketUtilTests, MPLSPacket) {
  // test deserialization
  auto mpls = std::string(kMPLSHdr);
  mpls.append(kIPv6Hdr);
  mpls.append(kUDP);

  auto bufIn = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(mpls));
  folly::io::Cursor cursor{bufIn.get()};

  auto mplsPkt = utility::MPLSPacket(cursor);
  auto mplsHdr = mplsPkt.header();
  verifyMPLSHdr(mplsHdr);
  auto ip6Pkt = mplsPkt.v6PayLoad().value();
  auto ip6Hdr = ip6Pkt.header();
  auto ip6Payload = ip6Pkt.payload();
  verifyIP6Hdr(ip6Hdr);
  auto udpPkt = ip6Payload.value();
  verifyUdp(udpPkt.header(), udpPkt.payload());

  // test serialization
  auto bufOut = folly::IOBuf::create(mplsPkt.length());
  bufOut->append(mplsPkt.length());

  folly::io::RWPrivateCursor rwCursor{bufOut.get()};
  mplsPkt.serialize(rwCursor);

  verifySerialization(bufIn.get(), bufOut.get());
}

TEST(PacketUtilTests, EthFrame) {
  auto eth = std::string(kEthHdr);
  eth.append(kMPLSHdr);
  eth.append(kIPv6Hdr);
  eth.append(kUDP);

  auto bufIn = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(eth));
  folly::io::Cursor cursor{bufIn.get()};

  auto ethPkt = utility::EthFrame(cursor);
  verifyEthHdr(ethPkt.header());

  auto mplsPkt = ethPkt.mplsPayLoad().value();
  auto mplsHdr = mplsPkt.header();
  verifyMPLSHdr(mplsHdr);
  auto ip6Pkt = mplsPkt.v6PayLoad().value();
  auto ip6Hdr = ip6Pkt.header();
  auto ip6Payload = ip6Pkt.payload();
  verifyIP6Hdr(ip6Hdr);
  auto udpPkt = ip6Payload.value();
  verifyUdp(udpPkt.header(), udpPkt.payload());

  // test serialization
  auto bufOut = folly::IOBuf::create(ethPkt.length());
  bufOut->append(ethPkt.length());

  folly::io::RWPrivateCursor rwCursor{bufOut.get()};
  ethPkt.serialize(rwCursor);

  verifySerialization(bufIn.get(), bufOut.get());
}
