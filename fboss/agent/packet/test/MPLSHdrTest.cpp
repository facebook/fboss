// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/MPLSHdr.h"

namespace facebook::fboss {

TEST(MPLSHdrTest, parameterized_data_constructor_label) {
  MPLSHdr hdr{MPLSHdr::Label{103, 4, 1, 79}};
  EXPECT_EQ(MPLSHdr::Label::kSizeBytes, hdr.size());
  const auto& label = hdr.stack()[0];
  EXPECT_EQ(103, label.label);
  EXPECT_EQ(4, label.trafficClass);
  EXPECT_EQ(1, label.bottomOfStack);
  EXPECT_EQ(79, label.timeToLive);
}

TEST(MPLSHdrTest, parameterized_data_constructor_stack) {
  std::vector<MPLSHdr::Label> stack{MPLSHdr::Label{101, 4, 0, 79},
                                    MPLSHdr::Label{102, 4, 0, 79},
                                    MPLSHdr::Label{103, 4, 1, 79}};
  MPLSHdr hdr{stack};
  const auto& hdrStack = hdr.stack();
  EXPECT_EQ(hdr.size(), 3 * MPLSHdr::Label::kSizeBytes);
  for (auto i = 0; i < hdrStack.size(); i++) {
    EXPECT_EQ(stack[i], hdrStack[i]);
  }
}

TEST(MPLSHdrTest, copy_constructor) {
  std::vector<MPLSHdr::Label> stack{MPLSHdr::Label{101, 4, 0, 79},
                                    MPLSHdr::Label{102, 4, 0, 79},
                                    MPLSHdr::Label{103, 4, 1, 79}};
  MPLSHdr hdr0{stack};
  MPLSHdr hdr1{hdr0};
  const auto& hdr0Stack = hdr0.stack();
  const auto& hdr1Stack = hdr1.stack();

  EXPECT_EQ(hdr1.size(), hdr0.size());
  EXPECT_EQ(hdr1.size(), 3 * MPLSHdr::Label::kSizeBytes);
  EXPECT_EQ(hdr0Stack, hdr1Stack);
}

TEST(MPLSHdrTest, cursor_data_constructor_1_label) {
  auto rxPkt = MockRxPacket::fromHex(
      "00067" // 103 label
      "d" // 110,1 -> exp 6 bottomOfStack 1
      "3f" // timeToLive 63
  );
  folly::io::Cursor cursor(rxPkt->buf());
  MPLSHdr hdr(&cursor);

  const auto& stack = hdr.stack();

  EXPECT_EQ(MPLSHdr::Label::kSizeBytes, hdr.size());
  EXPECT_EQ(stack[0].label, 103);
  EXPECT_EQ(stack[0].trafficClass, 6);
  EXPECT_EQ(stack[0].bottomOfStack, 1);
  EXPECT_EQ(stack[0].timeToLive, 63);
}

TEST(MPLSHdrTest, cursor_data_constructor_3_labels) {
  auto rxPkt = MockRxPacket::fromHex(
      "054c4" // 21700 label
      "8" // 010,0 -> exp 4 bottomOfStack 0
      "7d" // timeToLive 125

      "00068" // 104 label
      "c" // 110,0 -> exp 6 bottomOfStack 0
      "7e" // timeToLive 126

      "00067" // 103 label
      "d" // 110,1 -> exp 6 bottomOfStack 1
      "3f" // timeToLive 63

      // ip header
      "6" // VERSION: 6
      "ff" // Traffic Class
      "fffff" // Flow Label
      "00 00" // Payload Length
      "3B" // Next Header: No Next Header
      "01" // Hop Limit: 1
      // IPv6 Source Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 03"
      // IPv6 Destination Address
      "26 20 00 00 1c fe fa ce"
      "b0 0c 00 00 00 00 00 04"

      // junk
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef"
      "deadbeef");

  std::vector<MPLSHdr::Label> stack{
      MPLSHdr::Label{21700, 4, false, 125},
      MPLSHdr::Label{104, 6, false, 126},
      MPLSHdr::Label{103, 6, true, 63},
  };
  folly::io::Cursor cursor(rxPkt->buf());
  MPLSHdr hdr(&cursor);
  EXPECT_EQ(hdr.size(), 3 * MPLSHdr::Label::kSizeBytes);
  const auto& hdrStack = hdr.stack();
  EXPECT_EQ(hdrStack.size(), stack.size());
  for (auto i = 0; i < hdrStack.size(); i++) {
    EXPECT_EQ(stack[i], hdrStack[i]);
  }
}

} // namespace facebook::fboss
