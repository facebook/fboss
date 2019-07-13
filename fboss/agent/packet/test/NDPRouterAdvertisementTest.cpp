/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/NDPRouterAdvertisement.h"

/*
 * References:
 *   https://code.google.com/p/googletest/wiki/Primer
 *   https://code.google.com/p/googletest/wiki/AdvancedGuide
 */
#include <gtest/gtest.h>

#include <folly/io/Cursor.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

using namespace facebook::fboss;
using folly::io::Cursor;

TEST(NDPRouterAdvertisementTest, default_constructor) {
  NDPRouterAdvertisement obj;
  EXPECT_EQ(0, obj.curHopLimit);
  EXPECT_EQ(0, obj.flags);
  EXPECT_EQ(0, obj.routerLifetime);
  EXPECT_EQ(0, obj.reachableTime);
  EXPECT_EQ(0, obj.retransTimer);
}

TEST(NDPRouterAdvertisementTest, copy_constructor) {
  uint8_t curHopLimit = 64;
  uint8_t flags = 0x08;
  uint16_t routerLifetime = 1800;
  uint32_t reachableTime = 0;
  uint32_t retransTimer = 0;
  NDPRouterAdvertisement lhs(
      curHopLimit, flags, routerLifetime, reachableTime, retransTimer);
  NDPRouterAdvertisement rhs(lhs);
  EXPECT_EQ(lhs, rhs);
}

TEST(NDPRouterAdvertisementTest, parameterized_data_constructor) {
  uint8_t curHopLimit = 64;
  uint8_t flags = 0x08;
  uint16_t routerLifetime = 1800;
  uint32_t reachableTime = 0;
  uint32_t retransTimer = 0;
  NDPRouterAdvertisement obj(
      curHopLimit, flags, routerLifetime, reachableTime, retransTimer);
  EXPECT_EQ(curHopLimit, obj.curHopLimit);
  EXPECT_EQ(flags, obj.flags);
  EXPECT_EQ(routerLifetime, obj.routerLifetime);
  EXPECT_EQ(reachableTime, obj.reachableTime);
  EXPECT_EQ(retransTimer, obj.retransTimer);
}

TEST(NDPRouterAdvertisementTest, cursor_data_constructor) {
  uint8_t curHopLimit = 64;
  uint8_t flags = 0x08;
  uint16_t routerLifetime = 1800;
  uint32_t reachableTime = 0;
  uint32_t retransTimer = 0;
  auto pkt = MockRxPacket::fromHex(
      // NDP Router Advertisement
      "40" // Cur Hop Limit: 64
      "08" // Flags
      "07 08" // Router Lifetime
      "00 00 00 00" // Reachable Time
      "00 00 00 00" // Retrans Timer
  );
  Cursor cursor(pkt->buf());
  NDPRouterAdvertisement obj(cursor);
  EXPECT_EQ(curHopLimit, obj.curHopLimit);
  EXPECT_EQ(flags, obj.flags);
  EXPECT_EQ(routerLifetime, obj.routerLifetime);
  EXPECT_EQ(reachableTime, obj.reachableTime);
  EXPECT_EQ(retransTimer, obj.retransTimer);
}

TEST(NDPRouterAdvertisementTest, cursor_data_constructor_too_small) {
  auto pkt = MockRxPacket::fromHex(
      // NDP Router Advertisement
      "40" // Cur Hop Limit: 64
      "08" // Flags
      "07 08" // Router Lifetime
      "00 00 00 00" // Reachable Time
      "00 00 00   " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ NDPRouterAdvertisement obj(cursor); }, HdrParseError);
}

TEST(NDPRouterAdvertisementTest, assignment_operator) {
  uint8_t curHopLimit = 64;
  uint8_t flags = 0x08;
  uint16_t routerLifetime = 1800;
  uint32_t reachableTime = 0;
  uint32_t retransTimer = 0;
  NDPRouterAdvertisement lhs(
      curHopLimit, flags, routerLifetime, reachableTime, retransTimer);
  NDPRouterAdvertisement rhs = lhs;
  EXPECT_EQ(lhs, rhs);
}

TEST(NDPRouterAdvertisementTest, equality_operator) {
  uint8_t curHopLimit = 64;
  uint8_t flags = 0x08;
  uint16_t routerLifetime = 1800;
  uint32_t reachableTime = 0;
  uint32_t retransTimer = 0;
  NDPRouterAdvertisement lhs(
      curHopLimit, flags, routerLifetime, reachableTime, retransTimer);
  NDPRouterAdvertisement rhs(
      curHopLimit, flags, routerLifetime, reachableTime, retransTimer);
  EXPECT_EQ(lhs, rhs);
}

TEST(NDPRouterAdvertisementTest, inequality_operator) {
  uint8_t curHopLimit = 64;
  uint8_t flags = 0x08;
  uint16_t routerLifetime = 1800;
  uint32_t reachableTime = 0;
  uint32_t retransTimer1 = 0;
  uint32_t retransTimer2 = 1;
  NDPRouterAdvertisement lhs(
      curHopLimit, flags, routerLifetime, reachableTime, retransTimer1);
  NDPRouterAdvertisement rhs(
      curHopLimit, flags, routerLifetime, reachableTime, retransTimer2);
  EXPECT_NE(lhs, rhs);
}
