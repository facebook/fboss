// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/PktFactory.h"

#include <gtest/gtest.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PTPHeader.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace {
const auto kSrcMac = folly::MacAddress("02:00:00:00:00:01");
const auto kDstMac = folly::MacAddress("02:00:00:00:00:02");
const auto kSrcIpV4 = folly::IPAddressV4("10.0.0.1");
const auto kDstIpV4 = folly::IPAddressV4("10.0.0.2");
const auto kSrcIpV6 = folly::IPAddressV6("2001:db8::1");
const auto kDstIpV6 = folly::IPAddressV6("2001:db8::2");
const auto kTestVlan = VlanID(1);

// Helper to read Ethernet header from a packet and verify MACs and ethertype
void verifyEthHeader(
    folly::io::Cursor& cursor,
    folly::MacAddress expectedDst,
    folly::MacAddress expectedSrc,
    uint16_t expectedEtherType,
    bool hasVlan = false,
    std::optional<VlanID> expectedVlan = std::nullopt) {
  // Read dst MAC
  uint8_t dstBytes[6];
  cursor.pull(dstBytes, 6);
  EXPECT_EQ(folly::MacAddress::fromBinary({dstBytes, 6}), expectedDst);
  // Read src MAC
  uint8_t srcBytes[6];
  cursor.pull(srcBytes, 6);
  EXPECT_EQ(folly::MacAddress::fromBinary({srcBytes, 6}), expectedSrc);

  if (hasVlan) {
    auto tpid = cursor.readBE<uint16_t>();
    EXPECT_EQ(tpid, 0x8100);
    auto vlanTag = cursor.readBE<uint16_t>();
    if (expectedVlan) {
      EXPECT_EQ(VlanID(vlanTag & 0xFFF), *expectedVlan);
    }
  }
  auto etherType = cursor.readBE<uint16_t>();
  EXPECT_EQ(etherType, expectedEtherType);
}
} // namespace

// --- makeEthHdr tests ---

TEST(PktFactoryTest, makeEthHdrWithVlan) {
  auto hdr = makeEthHdr(kSrcMac, kDstMac, kTestVlan, ETHERTYPE::ETHERTYPE_IPV6);
  EXPECT_EQ(hdr.getSrcMac(), kSrcMac);
  EXPECT_EQ(hdr.getDstMac(), kDstMac);
  EXPECT_EQ(hdr.getVlanTags().size(), 1);
  EXPECT_EQ(VlanID(hdr.getVlanTags()[0].vid()), kTestVlan);
  EXPECT_EQ(
      hdr.getEtherType(), static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
}

TEST(PktFactoryTest, makeEthHdrNoVlan) {
  auto hdr =
      makeEthHdr(kSrcMac, kDstMac, std::nullopt, ETHERTYPE::ETHERTYPE_IPV4);
  EXPECT_EQ(hdr.getSrcMac(), kSrcMac);
  EXPECT_EQ(hdr.getDstMac(), kDstMac);
  EXPECT_EQ(hdr.getVlanTags().size(), 0);
  EXPECT_EQ(
      hdr.getEtherType(), static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
}

// --- makeEthTxPacket tests ---

TEST(PktFactoryTest, makeEthTxPacketWithVlan) {
  auto pkt = makeEthTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      ETHERTYPE::ETHERTYPE_IPV6,
      std::nullopt);
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      true,
      kTestVlan);
}

TEST(PktFactoryTest, makeEthTxPacketNoVlan) {
  auto pkt = makeEthTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      ETHERTYPE::ETHERTYPE_IPV4,
      std::nullopt);
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4),
      false);
}

TEST(PktFactoryTest, makeEthTxPacketWithCustomPayload) {
  std::vector<uint8_t> payload = {0xde, 0xad, 0xbe, 0xef};
  auto pkt = makeEthTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      ETHERTYPE::ETHERTYPE_IPV4,
      payload);
  ASSERT_NE(pkt, nullptr);
  // Packet should be EthHdr::SIZE + 4 (payload)
  EXPECT_EQ(pkt->buf()->computeChainDataLength(), EthHdr::SIZE + 4);
}

TEST(PktFactoryTest, makeEthTxPacketDefaultPayload) {
  auto pkt = makeEthTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      ETHERTYPE::ETHERTYPE_IPV4,
      std::nullopt);
  ASSERT_NE(pkt, nullptr);
  // Default payload is 256 bytes of 0xff
  EXPECT_EQ(pkt->buf()->computeChainDataLength(), EthHdr::SIZE + 256);
}

TEST(PktFactoryTest, makeEthTxPacketEmptyPayload) {
  std::vector<uint8_t> payload = {};
  auto pkt = makeEthTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      ETHERTYPE::ETHERTYPE_IPV4,
      payload);
  ASSERT_NE(pkt, nullptr);
  EXPECT_EQ(pkt->buf()->computeChainDataLength(), EthHdr::SIZE);
}

TEST(PktFactoryTest, makeEthTxPacketMaxVlan) {
  auto pkt = makeEthTxPacket(
      &TxPacket::allocateTxPacket,
      VlanID(4095),
      kSrcMac,
      kDstMac,
      ETHERTYPE::ETHERTYPE_IPV6,
      std::nullopt);
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      true,
      VlanID(4095));
}

// --- makeIpTxPacket tests ---

TEST(PktFactoryTest, makeIpTxPacketV6) {
  auto pkt = makeIpTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6,
      /*trafficClass=*/0x10,
      /*hopLimit=*/64);
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      true,
      kTestVlan);
  // Parse IPv6 header
  IPv6Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV6);
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV6);
  EXPECT_EQ(ipHdr.trafficClass, 0x10);
  EXPECT_EQ(ipHdr.hopLimit, 64);
}

TEST(PktFactoryTest, makeIpTxPacketV4) {
  auto pkt = makeIpTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV4,
      kDstIpV4,
      /*dscp=*/0x08,
      /*ttl=*/128);
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4),
      true,
      kTestVlan);
  IPv4Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV4);
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV4);
  EXPECT_EQ(ipHdr.ttl, 128);
  EXPECT_EQ(ipHdr.dscp, 0x08);
}

TEST(PktFactoryTest, makeIpTxPacketGenericV6) {
  auto pkt = makeIpTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV6),
      folly::IPAddress(kDstIpV6));
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      false);
  IPv6Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV6);
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV6);
}

TEST(PktFactoryTest, makeIpTxPacketGenericV4) {
  auto pkt = makeIpTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV4),
      folly::IPAddress(kDstIpV4));
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4),
      false);
  IPv4Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV4);
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV4);
}

// --- makeUDPTxPacket tests ---

TEST(PktFactoryTest, makeUDPTxPacketV6) {
  auto pkt = makeUDPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6,
      /*srcPort=*/8000,
      /*dstPort=*/8001,
      /*trafficClass=*/0,
      /*hopLimit=*/64);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v6 = frame.v6PayLoad();
  ASSERT_TRUE(v6.has_value());
  EXPECT_EQ(v6->header().srcAddr, kSrcIpV6);
  EXPECT_EQ(v6->header().dstAddr, kDstIpV6);
  EXPECT_EQ(
      v6->header().nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  auto udp = v6->udpPayload();
  ASSERT_TRUE(udp.has_value());
  EXPECT_EQ(udp->header().srcPort, 8000);
  EXPECT_EQ(udp->header().dstPort, 8001);
}

TEST(PktFactoryTest, makeUDPTxPacketV4) {
  auto pkt = makeUDPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV4,
      kDstIpV4,
      /*srcPort=*/9000,
      /*dstPort=*/9001,
      /*dscp=*/0,
      /*ttl=*/255);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v4 = frame.v4PayLoad();
  ASSERT_TRUE(v4.has_value());
  EXPECT_EQ(v4->header().srcAddr, kSrcIpV4);
  EXPECT_EQ(v4->header().dstAddr, kDstIpV4);
  EXPECT_EQ(
      v4->header().protocol, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  auto udp = v4->udpPayload();
  ASSERT_TRUE(udp.has_value());
  EXPECT_EQ(udp->header().srcPort, 9000);
  EXPECT_EQ(udp->header().dstPort, 9001);
}

TEST(PktFactoryTest, makeUDPTxPacketGenericV6) {
  auto pkt = makeUDPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV6),
      folly::IPAddress(kDstIpV6),
      /*srcPort=*/1000,
      /*dstPort=*/2000);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  EXPECT_TRUE(frame.v6PayLoad().has_value());
  auto udp = frame.v6PayLoad()->udpPayload();
  ASSERT_TRUE(udp.has_value());
  EXPECT_EQ(udp->header().srcPort, 1000);
  EXPECT_EQ(udp->header().dstPort, 2000);
}

TEST(PktFactoryTest, makeUDPTxPacketGenericV4) {
  auto pkt = makeUDPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV4),
      folly::IPAddress(kDstIpV4),
      /*srcPort=*/3000,
      /*dstPort=*/4000);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  EXPECT_TRUE(frame.v4PayLoad().has_value());
  auto udp = frame.v4PayLoad()->udpPayload();
  ASSERT_TRUE(udp.has_value());
  EXPECT_EQ(udp->header().srcPort, 3000);
  EXPECT_EQ(udp->header().dstPort, 4000);
}

TEST(PktFactoryTest, makeUDPTxPacketWithPayload) {
  std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
  auto pkt = makeUDPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6,
      /*srcPort=*/5000,
      /*dstPort=*/5001,
      /*trafficClass=*/0,
      /*hopLimit=*/255,
      payload);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v6 = frame.v6PayLoad();
  ASSERT_TRUE(v6.has_value());
  auto udp = v6->udpPayload();
  ASSERT_TRUE(udp.has_value());
  // UDP length = header (8) + payload (3)
  EXPECT_EQ(udp->header().length, 8 + 3);
}

TEST(PktFactoryTest, makeUDPTxPacketEmptyPayload) {
  std::vector<uint8_t> payload = {};
  auto pkt = makeUDPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6,
      /*srcPort=*/5500,
      /*dstPort=*/5501,
      /*trafficClass=*/0,
      /*hopLimit=*/64,
      payload);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v6 = frame.v6PayLoad();
  ASSERT_TRUE(v6.has_value());
  auto udp = v6->udpPayload();
  ASSERT_TRUE(udp.has_value());
  // UDP length = header (8) + empty payload (0)
  EXPECT_EQ(udp->header().length, 8);
}

TEST(PktFactoryTest, makeUDPTxPacketDefaultPayload) {
  auto pkt = makeUDPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6,
      /*srcPort=*/7000,
      /*dstPort=*/7001);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v6 = frame.v6PayLoad();
  ASSERT_TRUE(v6.has_value());
  auto udp = v6->udpPayload();
  ASSERT_TRUE(udp.has_value());
  // UDP length = header (8) + default payload (256)
  EXPECT_EQ(udp->header().length, 8 + 256);
}

// --- makeTCPTxPacket tests ---

TEST(PktFactoryTest, makeTCPTxPacketV6) {
  auto pkt = makeTCPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6,
      /*srcPort=*/80,
      /*dstPort=*/443,
      /*trafficClass=*/0,
      /*hopLimit=*/128);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v6 = frame.v6PayLoad();
  ASSERT_TRUE(v6.has_value());
  EXPECT_EQ(v6->header().srcAddr, kSrcIpV6);
  EXPECT_EQ(v6->header().dstAddr, kDstIpV6);
  EXPECT_EQ(
      v6->header().nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP));
  auto tcp = v6->tcpPayload();
  ASSERT_TRUE(tcp.has_value());
  EXPECT_EQ(tcp->header().srcPort, 80);
  EXPECT_EQ(tcp->header().dstPort, 443);
}

TEST(PktFactoryTest, makeTCPTxPacketV4) {
  auto pkt = makeTCPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV4,
      kDstIpV4,
      /*srcPort=*/22,
      /*dstPort=*/8080,
      /*dscp=*/0,
      /*ttl=*/64);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v4 = frame.v4PayLoad();
  ASSERT_TRUE(v4.has_value());
  EXPECT_EQ(v4->header().srcAddr, kSrcIpV4);
  EXPECT_EQ(v4->header().dstAddr, kDstIpV4);
  EXPECT_EQ(
      v4->header().protocol, static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP));
  auto tcp = v4->tcpPayload();
  ASSERT_TRUE(tcp.has_value());
  EXPECT_EQ(tcp->header().srcPort, 22);
  EXPECT_EQ(tcp->header().dstPort, 8080);
}

TEST(PktFactoryTest, makeTCPTxPacketGenericV6) {
  auto pkt = makeTCPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV6),
      folly::IPAddress(kDstIpV6),
      /*srcPort=*/1234,
      /*dstPort=*/5678);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  EXPECT_TRUE(frame.v6PayLoad().has_value());
  auto tcp = frame.v6PayLoad()->tcpPayload();
  ASSERT_TRUE(tcp.has_value());
  EXPECT_EQ(tcp->header().srcPort, 1234);
  EXPECT_EQ(tcp->header().dstPort, 5678);
}

TEST(PktFactoryTest, makeTCPTxPacketGenericV4) {
  auto pkt = makeTCPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV4),
      folly::IPAddress(kDstIpV4),
      /*srcPort=*/4321,
      /*dstPort=*/8765);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  EXPECT_TRUE(frame.v4PayLoad().has_value());
  auto tcp = frame.v4PayLoad()->tcpPayload();
  ASSERT_TRUE(tcp.has_value());
  EXPECT_EQ(tcp->header().srcPort, 4321);
  EXPECT_EQ(tcp->header().dstPort, 8765);
}

TEST(PktFactoryTest, makeTCPTxPacketDstMacOnlyV4) {
  auto pkt = makeTCPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kDstMac,
      folly::IPAddress(kDstIpV4),
      /*l4SrcPort=*/1111,
      /*l4DstPort=*/2222);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v4 = frame.v4PayLoad();
  ASSERT_TRUE(v4.has_value());
  EXPECT_EQ(v4->header().dstAddr, kDstIpV4);
  auto tcp = v4->tcpPayload();
  ASSERT_TRUE(tcp.has_value());
  EXPECT_EQ(tcp->header().srcPort, 1111);
  EXPECT_EQ(tcp->header().dstPort, 2222);
}

TEST(PktFactoryTest, makeTCPTxPacketDstMacOnlyV6) {
  auto pkt = makeTCPTxPacket(
      &TxPacket::allocateTxPacket,
      std::nullopt,
      kDstMac,
      folly::IPAddress(kDstIpV6),
      /*l4SrcPort=*/3333,
      /*l4DstPort=*/4444);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto v6 = frame.v6PayLoad();
  ASSERT_TRUE(v6.has_value());
  EXPECT_EQ(v6->header().dstAddr, kDstIpV6);
  auto tcp = v6->tcpPayload();
  ASSERT_TRUE(tcp.has_value());
  EXPECT_EQ(tcp->header().srcPort, 3333);
  EXPECT_EQ(tcp->header().dstPort, 4444);
}

// --- makeIpInIpTxPacket tests ---

TEST(PktFactoryTest, makeIpInIpTxPacket) {
  auto outerSrcIp = folly::IPAddressV6("2001:db8:1::1");
  auto outerDstIp = folly::IPAddressV6("2001:db8:1::2");
  auto innerSrcIp = folly::IPAddressV6("2001:db8:2::1");
  auto innerDstIp = folly::IPAddressV6("2001:db8:2::2");
  auto pkt = makeIpInIpTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      outerSrcIp,
      outerDstIp,
      innerSrcIp,
      innerDstIp,
      /*srcPort=*/6000,
      /*dstPort=*/6001,
      /*outerTrafficClass=*/0x20,
      /*innerTrafficClass=*/0x10,
      /*hopLimit=*/64);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt, /*skipTtlDecrement=*/true);
  auto outerV6 = frame.v6PayLoad();
  ASSERT_TRUE(outerV6.has_value());
  EXPECT_EQ(outerV6->header().srcAddr, outerSrcIp);
  EXPECT_EQ(outerV6->header().dstAddr, outerDstIp);
  EXPECT_EQ(outerV6->header().trafficClass, 0x20);
  EXPECT_EQ(outerV6->header().hopLimit, 64);
  EXPECT_EQ(
      outerV6->header().nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6));
  // Inner IPv6 packet
  auto innerV6 = outerV6->v6PayLoad();
  ASSERT_NE(innerV6, nullptr);
  EXPECT_EQ(innerV6->header().srcAddr, innerSrcIp);
  EXPECT_EQ(innerV6->header().dstAddr, innerDstIp);
  EXPECT_EQ(innerV6->header().trafficClass, 0x10);
  auto udp = innerV6->udpPayload();
  ASSERT_TRUE(udp.has_value());
  EXPECT_EQ(udp->header().srcPort, 6000);
  EXPECT_EQ(udp->header().dstPort, 6001);
}

TEST(PktFactoryTest, makeIpInIpTxPacketWithInnerV4) {
  auto outerSrcIp = folly::IPAddressV6("2001:db8:1::1");
  auto outerDstIp = folly::IPAddressV6("2001:db8:1::2");
  auto innerSrcIp = folly::IPAddressV4("10.0.0.1");
  auto innerDstIp = folly::IPAddressV4("10.0.0.2");
  auto pkt = makeIpInIpTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      outerSrcIp,
      outerDstIp,
      innerSrcIp,
      innerDstIp,
      /*srcPort=*/7000,
      /*dstPort=*/7001,
      /*outerTrafficClass=*/0x20,
      /*innerDscp=*/0x10,
      /*hopLimit=*/64);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt, /*skipTtlDecrement=*/true);
  auto outerV6 = frame.v6PayLoad();
  ASSERT_TRUE(outerV6.has_value());
  EXPECT_EQ(outerV6->header().srcAddr, outerSrcIp);
  EXPECT_EQ(outerV6->header().dstAddr, outerDstIp);
  EXPECT_EQ(outerV6->header().trafficClass, 0x20);
  EXPECT_EQ(outerV6->header().hopLimit, 64);
  EXPECT_EQ(
      outerV6->header().nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4));
  // Inner IPv4 packet
  auto* innerV4 = outerV6->v4PayLoad();
  ASSERT_NE(innerV4, nullptr);
  EXPECT_EQ(innerV4->header().srcAddr, innerSrcIp);
  EXPECT_EQ(innerV4->header().dstAddr, innerDstIp);
  EXPECT_EQ(innerV4->header().dscp, 0x10);
  auto udp = innerV4->udpPayload();
  ASSERT_TRUE(udp.has_value());
  EXPECT_EQ(udp->header().srcPort, 7000);
  EXPECT_EQ(udp->header().dstPort, 7001);
}

// --- makeARPTxPacket tests ---

TEST(PktFactoryTest, makeARPTxPacketRequest) {
  auto pkt = makeARPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      folly::MacAddress("ff:ff:ff:ff:ff:ff"),
      folly::IPAddress(kSrcIpV4),
      folly::IPAddress(kDstIpV4),
      ARP_OPER::ARP_OPER_REQUEST);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto hdr = frame.header();
  EXPECT_EQ(hdr.getSrcMac(), kSrcMac);
  EXPECT_EQ(hdr.etherType, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_ARP));
  auto arp = frame.arpHdr();
  ASSERT_TRUE(arp.has_value());
  EXPECT_EQ(arp->oper, static_cast<uint16_t>(ARP_OPER::ARP_OPER_REQUEST));
  EXPECT_EQ(arp->sha, kSrcMac);
  EXPECT_EQ(arp->spa, kSrcIpV4);
  EXPECT_EQ(arp->tpa, kDstIpV4);
  EXPECT_EQ(arp->tha, folly::MacAddress("00:00:00:00:00:00"));
}

TEST(PktFactoryTest, makeARPTxPacketReply) {
  auto targetMac = folly::MacAddress("02:00:00:00:00:03");
  auto pkt = makeARPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV4),
      folly::IPAddress(kDstIpV4),
      ARP_OPER::ARP_OPER_REPLY,
      targetMac);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto arp = frame.arpHdr();
  ASSERT_TRUE(arp.has_value());
  EXPECT_EQ(arp->oper, static_cast<uint16_t>(ARP_OPER::ARP_OPER_REPLY));
  EXPECT_EQ(arp->tha, targetMac);
}

TEST(PktFactoryTest, makeARPTxPacketReplyDefaultTargetMac) {
  // When no targetMac provided for reply, should use dstMac
  auto pkt = makeARPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV4),
      folly::IPAddress(kDstIpV4),
      ARP_OPER::ARP_OPER_REPLY);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt);
  auto arp = frame.arpHdr();
  ASSERT_TRUE(arp.has_value());
  EXPECT_EQ(arp->tha, kDstMac);
}

// --- makeNeighborSolicitation tests ---

TEST(PktFactoryTest, makeNeighborSolicitation) {
  auto pkt = makeNeighborSolicitation(
      &TxPacket::allocateTxPacket, kTestVlan, kSrcMac, kSrcIpV6, kDstIpV6);
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  // Skip eth header (18 bytes with vlan)
  cursor.skip(18);
  IPv6Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV6);
  // Dst should be solicited node multicast for kDstIpV6
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV6.getSolicitedNodeAddress());
  EXPECT_EQ(
      ipHdr.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP));
  EXPECT_EQ(ipHdr.hopLimit, 255);
}

// --- makeNeighborAdvertisement tests ---

TEST(PktFactoryTest, makeNeighborAdvertisement) {
  auto pkt = makeNeighborAdvertisement(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6);
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  cursor.skip(18);
  IPv6Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV6);
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV6);
  EXPECT_EQ(
      ipHdr.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP));
  EXPECT_EQ(ipHdr.hopLimit, 255);
}

TEST(PktFactoryTest, makeNeighborAdvertisementZeroDstIp) {
  auto pkt = makeNeighborAdvertisement(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      folly::IPAddressV6("::"));
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  cursor.skip(18);
  IPv6Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.dstAddr, folly::IPAddressV6("ff01::1"));
}

// --- makePTPTxPacket tests ---

TEST(PktFactoryTest, makePTPTxPacketSyncThrows) {
  // PTP_SYNC is not supported by PTPHeader::getPayloadSize
  EXPECT_THROW(
      makePTPTxPacket(
          &TxPacket::allocateTxPacket,
          kTestVlan,
          kSrcMac,
          kDstMac,
          kSrcIpV6,
          kDstIpV6,
          /*trafficClass=*/0,
          /*hopLimit=*/1,
          PTPMessageType::PTP_SYNC),
      FbossError);
}

TEST(PktFactoryTest, makePTPTxPacketDelayReq) {
  auto pkt = makePTPTxPacket(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      kSrcIpV6,
      kDstIpV6,
      /*trafficClass=*/0x04,
      /*hopLimit=*/1,
      PTPMessageType::PTP_DELAY_REQUEST);
  ASSERT_NE(pkt, nullptr);
  auto frame = makeEthFrame(*pkt, /*skipTtlDecrement=*/true);
  auto v6 = frame.v6PayLoad();
  ASSERT_TRUE(v6.has_value());
  EXPECT_EQ(v6->header().srcAddr, kSrcIpV6);
  EXPECT_EQ(v6->header().dstAddr, kDstIpV6);
  EXPECT_EQ(v6->header().trafficClass, 0x04);
  EXPECT_EQ(v6->header().hopLimit, 1);
  EXPECT_EQ(
      v6->header().nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  auto udp = v6->udpPayload();
  ASSERT_TRUE(udp.has_value());
  EXPECT_EQ(udp->header().srcPort, PTP_UDP_EVENT_PORT);
  EXPECT_EQ(udp->header().dstPort, PTP_UDP_EVENT_PORT);
}

// --- makeSflowV5Packet tests ---

TEST(PktFactoryTest, makeSflowV5PacketV6) {
  auto pkt = makeSflowV5Packet(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV6),
      folly::IPAddress(kDstIpV6),
      /*srcPort=*/6343,
      /*dstPort=*/6343,
      /*trafficClass=*/0,
      /*hopLimit=*/64,
      /*ingressInterface=*/1,
      /*egressInterface=*/2,
      /*samplingRate=*/1000,
      /*computeChecksum=*/false,
      std::vector<uint8_t>(64, 0xff));
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
      true,
      kTestVlan);
  IPv6Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV6);
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV6);
  EXPECT_EQ(ipHdr.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
}

TEST(PktFactoryTest, makeSflowV5PacketV4) {
  auto pkt = makeSflowV5Packet(
      &TxPacket::allocateTxPacket,
      kTestVlan,
      kSrcMac,
      kDstMac,
      folly::IPAddress(kSrcIpV4),
      folly::IPAddress(kDstIpV4),
      /*srcPort=*/6343,
      /*dstPort=*/6343,
      /*trafficClass=*/0,
      /*hopLimit=*/64,
      /*ingressInterface=*/1,
      /*egressInterface=*/2,
      /*samplingRate=*/500,
      /*computeChecksum=*/false,
      std::vector<uint8_t>(64, 0xff));
  ASSERT_NE(pkt, nullptr);
  folly::io::Cursor cursor(pkt->buf());
  verifyEthHeader(
      cursor,
      kDstMac,
      kSrcMac,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4),
      true,
      kTestVlan);
  IPv4Hdr ipHdr(cursor);
  EXPECT_EQ(ipHdr.srcAddr, kSrcIpV4);
  EXPECT_EQ(ipHdr.dstAddr, kDstIpV4);
}
