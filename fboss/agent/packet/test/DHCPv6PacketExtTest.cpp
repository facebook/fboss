// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/DHCPv6Packet.h"

#include <gtest/gtest.h>

#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/FbossError.h"

using namespace facebook::fboss;
using folly::IOBuf;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;

namespace {
const auto kLinkAddr = IPAddressV6("fe80::1");
const auto kPeerAddr = IPAddressV6("fe80::2");
const auto kMac = MacAddress("00:11:22:33:44:55");

DHCPv6Packet makeNonRelayPacket() {
  return DHCPv6Packet(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT), 0x123456);
}

DHCPv6Packet makeRelayForwardPacket() {
  return DHCPv6Packet(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD),
      5,
      kLinkAddr,
      kPeerAddr);
}

DHCPv6Packet makeRelayReplyPacket() {
  return DHCPv6Packet(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_REPLY),
      3,
      kLinkAddr,
      kPeerAddr);
}
} // namespace

// --- isDHCPv6Relay tests ---

TEST(DHCPv6PacketExtTest, isDHCPv6RelayForward) {
  auto pkt = makeRelayForwardPacket();
  EXPECT_TRUE(pkt.isDHCPv6Relay());
}

TEST(DHCPv6PacketExtTest, isDHCPv6RelayReply) {
  auto pkt = makeRelayReplyPacket();
  EXPECT_TRUE(pkt.isDHCPv6Relay());
}

TEST(DHCPv6PacketExtTest, isNotRelaySolicit) {
  auto pkt = makeNonRelayPacket();
  EXPECT_FALSE(pkt.isDHCPv6Relay());
}

TEST(DHCPv6PacketExtTest, isNotRelayRequest) {
  DHCPv6Packet pkt(static_cast<uint8_t>(DHCPv6Type::DHCPv6_REQUEST), 100);
  EXPECT_FALSE(pkt.isDHCPv6Relay());
}

// --- computePacketLength tests ---

TEST(DHCPv6PacketExtTest, computePacketLengthNonRelay) {
  auto pkt = makeNonRelayPacket();
  // type(1) + transactionId(3) + options(0) = 4
  EXPECT_EQ(pkt.computePacketLength(), 4);
}

TEST(DHCPv6PacketExtTest, computePacketLengthRelay) {
  auto pkt = makeRelayForwardPacket();
  // type(1) + hopcount(1) + linkAddr(16) + peerAddr(16) + options(0) = 34
  EXPECT_EQ(pkt.computePacketLength(), 34);
}

TEST(DHCPv6PacketExtTest, computePacketLengthWithOptions) {
  auto pkt = makeNonRelayPacket();
  pkt.addInterfaceIDOption(kMac);
  // type(1) + transactionId(3) + option header(4) + MAC(6) = 14
  EXPECT_EQ(pkt.computePacketLength(), 4 + 4 + MacAddress::SIZE);
}

// --- Non-relay write/parse round-trip ---

TEST(DHCPv6PacketExtTest, nonRelayWriteParseRoundTrip) {
  auto pkt = makeNonRelayPacket();

  auto len = pkt.computePacketLength();
  auto buf = IOBuf::create(len);
  buf->append(len);
  RWPrivateCursor cursor(buf.get());
  pkt.write(&cursor);

  Cursor readCursor(buf.get());
  DHCPv6Packet pkt2;
  pkt2.parse(&readCursor);

  EXPECT_EQ(pkt, pkt2);
  EXPECT_EQ(pkt2.type, static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT));
  EXPECT_EQ(pkt2.transactionId, 0x123456);
}

// --- Relay write/parse round-trip ---

TEST(DHCPv6PacketExtTest, relayWriteParseRoundTrip) {
  auto pkt = makeRelayForwardPacket();

  auto len = pkt.computePacketLength();
  auto buf = IOBuf::create(len);
  buf->append(len);
  RWPrivateCursor cursor(buf.get());
  pkt.write(&cursor);

  Cursor readCursor(buf.get());
  DHCPv6Packet pkt2;
  pkt2.parse(&readCursor);

  EXPECT_EQ(pkt, pkt2);
  EXPECT_EQ(pkt2.type, static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD));
  EXPECT_EQ(pkt2.hopCount, 5);
  EXPECT_EQ(pkt2.linkAddr, kLinkAddr);
  EXPECT_EQ(pkt2.peerAddr, kPeerAddr);
}

// --- Relay reply round-trip ---

TEST(DHCPv6PacketExtTest, relayReplyWriteParseRoundTrip) {
  auto pkt = makeRelayReplyPacket();

  auto len = pkt.computePacketLength();
  auto buf = IOBuf::create(len);
  buf->append(len);
  RWPrivateCursor cursor(buf.get());
  pkt.write(&cursor);

  Cursor readCursor(buf.get());
  DHCPv6Packet pkt2;
  pkt2.parse(&readCursor);

  EXPECT_EQ(pkt, pkt2);
  EXPECT_EQ(pkt2.type, static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_REPLY));
  EXPECT_EQ(pkt2.hopCount, 3);
}

// --- addInterfaceIDOption ---

TEST(DHCPv6PacketExtTest, addInterfaceIDOption) {
  auto pkt = makeNonRelayPacket();
  pkt.addInterfaceIDOption(kMac);

  // option header(4) + MAC size(6) = 10 bytes in options
  EXPECT_EQ(pkt.options.size(), 4 + MacAddress::SIZE);

  // Extract and verify
  std::unordered_set<uint16_t> selector;
  auto opts = pkt.extractOptions(selector);
  EXPECT_EQ(opts.size(), 1);
  EXPECT_EQ(
      opts[0].op,
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID));
  EXPECT_EQ(opts[0].len, MacAddress::SIZE);
  MacAddress mac =
      MacAddress::fromBinary(folly::ByteRange(opts[0].data, MacAddress::SIZE));
  EXPECT_EQ(mac, kMac);
}

// --- addRelayMessageOption ---

TEST(DHCPv6PacketExtTest, addRelayMessageOption) {
  auto innerPkt = makeNonRelayPacket();
  innerPkt.addInterfaceIDOption(kMac);

  auto relayPkt = makeRelayForwardPacket();
  relayPkt.addRelayMessageOption(innerPkt);

  // Options should contain the serialized inner packet
  auto innerLen = innerPkt.computePacketLength();
  // option header(4) + inner packet length
  EXPECT_EQ(relayPkt.options.size(), 4 + innerLen);

  // Extract relay message option
  std::unordered_set<uint16_t> selector{
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_RELAY_MSG)};
  auto opts = relayPkt.extractOptions(selector);
  EXPECT_EQ(opts.size(), 1);
  EXPECT_EQ(
      opts[0].op,
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_RELAY_MSG));
  EXPECT_EQ(opts[0].len, innerLen);

  // Parse inner packet from option data
  auto innerBuf = IOBuf::wrapBuffer(opts[0].data, opts[0].len);
  Cursor innerCursor(innerBuf.get());
  DHCPv6Packet parsedInner;
  parsedInner.parse(&innerCursor);
  EXPECT_EQ(parsedInner, innerPkt);
}

// --- Multiple options ---

TEST(DHCPv6PacketExtTest, multipleOptions) {
  auto pkt = makeRelayForwardPacket();
  pkt.addInterfaceIDOption(kMac);

  auto innerPkt = makeNonRelayPacket();
  pkt.addRelayMessageOption(innerPkt);

  // Should have 2 options
  std::unordered_set<uint16_t> selectAll;
  auto opts = pkt.extractOptions(selectAll);
  EXPECT_EQ(opts.size(), 2);
  EXPECT_EQ(
      opts[0].op,
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID));
  EXPECT_EQ(
      opts[1].op,
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_RELAY_MSG));
}

// --- extractOptions with selector ---

TEST(DHCPv6PacketExtTest, extractOptionsWithSelector) {
  auto pkt = makeRelayForwardPacket();
  pkt.addInterfaceIDOption(kMac);

  auto innerPkt = makeNonRelayPacket();
  pkt.addRelayMessageOption(innerPkt);

  // Only select interface ID
  std::unordered_set<uint16_t> selector{
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID)};
  auto opts = pkt.extractOptions(selector);
  EXPECT_EQ(opts.size(), 1);
  EXPECT_EQ(
      opts[0].op,
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID));
}

// --- appendOption ---

TEST(DHCPv6PacketExtTest, appendOptionRaw) {
  auto pkt = makeNonRelayPacket();
  uint8_t data[] = {0xaa, 0xbb, 0xcc};
  auto added = pkt.appendOption(42, 3, data);
  // Should return 4 (header) + 3 (data) = 7
  EXPECT_EQ(added, 7);
  EXPECT_EQ(pkt.options.size(), 7);
}

// --- equality tests ---

TEST(DHCPv6PacketExtTest, equalNonRelayPackets) {
  auto pkt1 = makeNonRelayPacket();
  auto pkt2 = makeNonRelayPacket();
  EXPECT_EQ(pkt1, pkt2);
}

TEST(DHCPv6PacketExtTest, equalRelayPackets) {
  auto pkt1 = makeRelayForwardPacket();
  auto pkt2 = makeRelayForwardPacket();
  EXPECT_EQ(pkt1, pkt2);
}

TEST(DHCPv6PacketExtTest, unequalDifferentType) {
  DHCPv6Packet pkt1(static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT), 100);
  DHCPv6Packet pkt2(static_cast<uint8_t>(DHCPv6Type::DHCPv6_REQUEST), 100);
  EXPECT_NE(pkt1, pkt2);
}

TEST(DHCPv6PacketExtTest, unequalDifferentTransactionId) {
  DHCPv6Packet pkt1(static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT), 100);
  DHCPv6Packet pkt2(static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT), 200);
  EXPECT_NE(pkt1, pkt2);
}

TEST(DHCPv6PacketExtTest, unequalDifferentOptions) {
  auto pkt1 = makeNonRelayPacket();
  auto pkt2 = makeNonRelayPacket();
  pkt1.addInterfaceIDOption(kMac);
  EXPECT_NE(pkt1, pkt2);
}

TEST(DHCPv6PacketExtTest, unequalRelayDifferentHopCount) {
  DHCPv6Packet pkt1(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD),
      1,
      kLinkAddr,
      kPeerAddr);
  DHCPv6Packet pkt2(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD),
      2,
      kLinkAddr,
      kPeerAddr);
  EXPECT_NE(pkt1, pkt2);
}

TEST(DHCPv6PacketExtTest, unequalRelayDifferentAddresses) {
  DHCPv6Packet pkt1(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD),
      0,
      kLinkAddr,
      kPeerAddr);
  DHCPv6Packet pkt2(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD),
      0,
      IPAddressV6("fe80::99"),
      kPeerAddr);
  EXPECT_NE(pkt1, pkt2);
}

// --- toString tests ---

TEST(DHCPv6PacketExtTest, toStringNonRelay) {
  auto pkt = makeNonRelayPacket();
  auto str = pkt.toString();
  EXPECT_NE(str.find("DHCPv6Packet"), std::string::npos);
  EXPECT_NE(str.find("transaction id"), std::string::npos);
}

TEST(DHCPv6PacketExtTest, toStringRelay) {
  auto pkt = makeRelayForwardPacket();
  auto str = pkt.toString();
  EXPECT_NE(str.find("relay"), std::string::npos);
  EXPECT_NE(str.find("hopCount"), std::string::npos);
  EXPECT_NE(str.find("linkAddr"), std::string::npos);
  EXPECT_NE(str.find("peerAddr"), std::string::npos);
}

// --- write/parse round-trip with options ---

TEST(DHCPv6PacketExtTest, nonRelayWithOptionsRoundTrip) {
  auto pkt = makeNonRelayPacket();
  pkt.addInterfaceIDOption(kMac);

  auto len = pkt.computePacketLength();
  auto buf = IOBuf::create(len);
  buf->append(len);
  RWPrivateCursor cursor(buf.get());
  pkt.write(&cursor);

  Cursor readCursor(buf.get());
  DHCPv6Packet pkt2;
  pkt2.parse(&readCursor);

  EXPECT_EQ(pkt, pkt2);
}

TEST(DHCPv6PacketExtTest, relayWithOptionsRoundTrip) {
  auto innerPkt = makeNonRelayPacket();
  innerPkt.addInterfaceIDOption(kMac);

  auto relayPkt = makeRelayForwardPacket();
  relayPkt.addInterfaceIDOption(MacAddress("aa:bb:cc:dd:ee:ff"));
  relayPkt.addRelayMessageOption(innerPkt);

  auto len = relayPkt.computePacketLength();
  auto buf = IOBuf::create(len);
  buf->append(len);
  RWPrivateCursor cursor(buf.get());
  relayPkt.write(&cursor);

  Cursor readCursor(buf.get());
  DHCPv6Packet parsed;
  parsed.parse(&readCursor);

  EXPECT_EQ(relayPkt, parsed);
}

// --- DHCPv6 type enum coverage ---

TEST(DHCPv6PacketExtTest, allNonRelayTypes) {
  for (auto t :
       {DHCPv6Type::DHCPv6_SOLICIT,
        DHCPv6Type::DHCPv6_ADVERTISE,
        DHCPv6Type::DHCPv6_REQUEST,
        DHCPv6Type::DHCPv6_CONFIRM,
        DHCPv6Type::DHCPv6_RENEW,
        DHCPv6Type::DHCPv6_REBIND,
        DHCPv6Type::DHCPv6_REPLY,
        DHCPv6Type::DHCPv6_RELEASE,
        DHCPv6Type::DHCPv6_DECLINE,
        DHCPv6Type::DHCPv6_RECONFIGURE,
        DHCPv6Type::DHCPv6_INFORM_REQ}) {
    DHCPv6Packet pkt(static_cast<uint8_t>(t), 0x42);
    EXPECT_FALSE(pkt.isDHCPv6Relay());
  }
}

// --- Transaction ID masking (24-bit) ---

TEST(DHCPv6PacketExtTest, transactionIdMaskedTo24Bits) {
  // Transaction ID is 24 bits; write sets (type << 24) | (transactionId &
  // 0x00ffffff)
  DHCPv6Packet pkt(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_SOLICIT), 0x00ABCDEF);

  auto len = pkt.computePacketLength();
  auto buf = IOBuf::create(len);
  buf->append(len);
  RWPrivateCursor cursor(buf.get());
  pkt.write(&cursor);

  Cursor readCursor(buf.get());
  DHCPv6Packet pkt2;
  pkt2.parse(&readCursor);

  EXPECT_EQ(pkt2.transactionId, 0x00ABCDEF);
}

// --- Constants ---

TEST(DHCPv6PacketExtTest, portConstants) {
  EXPECT_EQ(DHCPv6Packet::DHCP6_CLIENT_UDPPORT, 546);
  EXPECT_EQ(DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT, 547);
}

TEST(DHCPv6PacketExtTest, maxMsgLength) {
  EXPECT_EQ(DHCPv6Packet::MAX_DHCPV6_MSG_LENGTH, 1214);
}
