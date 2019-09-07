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
} // anonymous namespace

TEST(PacketUtilTests, UDPDatagram) {
  auto buf = std::make_unique<folly::IOBuf>(PktUtil::parseHexData(kUDP));
  folly::io::Cursor cursor{buf.get()};

  auto udpPkt = utility::UDPDatagram(cursor);
  verifyUdp(udpPkt.header(), udpPkt.payload());
}
