/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <fb303/ServiceData.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/cast.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IOBuf;
using folly::io::Appender;
using folly::io::Cursor;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using ::testing::_;
using ::testing::Return;

TEST(UDPTest, Parse) {
  // setup a default state object
  auto handle = createTestHandle();
  auto sw = handle->getSw();
  auto state = sw->getState();
  PortID portID(1);
  VlanID vlanID(1);

  // Cache the current stats
  CounterCache counters(sw);

  // Create an UDP packet  without IP or L2
  auto pkt = MockRxPacket::fromHex(
      // Source port (67), destination port (68)
      "00 43 00 44"
      // Length (8), checksum (0x1234, faked)
      "00 08  12 34");
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  Cursor c(pkt->buf());
  UDPHeader hdr;
  hdr.parse(&c, sw->portStats(portID));
  EXPECT_EQ(67, hdr.srcPort);
  EXPECT_EQ(68, hdr.dstPort);
  EXPECT_EQ(8, hdr.length);
  EXPECT_EQ(0x1234, hdr.csum);

  // Create a smaller than acceptable header
  pkt = MockRxPacket::fromHex(
      // Source port (67), destination port (68)
      "00 43 00 44"
      // Missing length and checksum
  );
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  Cursor c2(pkt->buf());
  EXPECT_THROW(hdr.parse(&c2, sw->portStats(portID)), FbossError);
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "udp.too_small.sum", 1);
}

TEST(UDPTest, Write) {
  UDPHeader hdr1(1667, 1668, 512, 0x3412);
  auto buf = IOBuf::create(0);
  buf->clear();
  Appender appender(buf.get(), UDPHeader::size());
  hdr1.write(&appender);

  UDPHeader hdr2;
  Cursor cursor(buf.get());
  SwitchStats switchStats;
  PortStats portStats(PortID(0), "foo", &switchStats);
  hdr2.parse(&cursor, &portStats);
  EXPECT_TRUE(hdr1 == hdr2);
}
