/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "common/stats/ServiceData.h"
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/IPHeaderV4.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/FbossError.h"

#include <boost/cast.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Cursor;
using std::make_unique;
using std::make_shared;
using std::make_pair;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using ::testing::_;
using ::testing::Return;

namespace {

unique_ptr<SwSwitch> setupSwitch() {
  auto state = testStateA();
  const auto& vlans = state->getVlans();
  // Set up an arp response entry for VLAN 1, 10.0.0.1,
  // so that we can detect the packet to 10.0.0.1 is for myself
  auto respTable1 = make_shared<ArpResponseTable>();
  respTable1->setEntry(IPAddressV4("10.0.0.1"),
                       MacAddress("00:02:00:00:00:01"),
                       InterfaceID(1));
  vlans->getVlan(VlanID(1))->setArpResponseTable(respTable1);

  return createMockSw(state);
}

} // unnamed namespace

TEST(IPv4Test, Parse) {
  auto sw = setupSwitch();
  PortID portID(1);
  VlanID vlanID(1);

  // Cache the current stats
  CounterCache counters(sw.get());

  // Create an IP pkt without L2
  auto pkt = MockRxPacket::fromHex(
    // Version(4), IHL(5), DSCP(7), ECN(1), Total Length(20)
    "45  1d  00 14"
    // Identification(0x3456), Flags(0x2), Fragment offset(0x1345)
    "34 56  53 45"
    // TTL(31), Protocol(6), Checksum (0x1234, fake)
    "1F  06  12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.0.10)
    "0a 00 00 0a"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  Cursor c(pkt->buf());
  IPHeaderV4 hdr;
  hdr.parse(sw.get(), portID, &c);
  EXPECT_EQ(4, hdr.getVersion());
  EXPECT_EQ(5, hdr.getIHL());
  EXPECT_EQ(7, hdr.getDSCP());
  EXPECT_EQ(1, hdr.getECN());
  EXPECT_EQ(20, hdr.getLength());
  EXPECT_EQ(0x3456, hdr.getID());
  EXPECT_EQ(0x2, hdr.getFlags());
  EXPECT_EQ(0x1345, hdr.getFragOffset());
  EXPECT_EQ(31, hdr.getTTL());
  EXPECT_EQ(6, hdr.getProto());
  EXPECT_EQ(0x1234, hdr.getChecksum());
  EXPECT_EQ(IPAddressV4("1.2.3.4"), hdr.getSrcIP());
  EXPECT_EQ(IPAddressV4("10.0.0.10"), hdr.getDstIP());

  // Create an IP pkt without L2, but smaller than 20 bytes
  pkt = MockRxPacket::fromHex(
    // Version(4), IHL(5), DSCP(7), ECN(1), Total Length(20)
    "45  1d  00 14"
    // Identification(0x3456), Flags(0x2), Fragment offset(0x1345)
    "34 56  53 45"
    // TTL(31), Protocol(6), Checksum (0x1234, fake)
    "1F  06  12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.0) one byte less
    "0a 00 00"
  );
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  Cursor c2(pkt->buf());
  EXPECT_THROW(hdr.parse(sw.get(), portID, &c2), FbossError);
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.too_small.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.mine.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + \
                      "ipv4.wrong_version.sum", 0);

  // create an IP pkt with wrong version
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "02 00 01 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(5), IHL(5), DSCP(7), ECN(1), Total Length(20)
    "55  1d  00 14"
    // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
    "34 56  53 45"
    // TTL(31), Protocol(6), Checksum (0x1234, fake)
    "1F  06  12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.0.10)
    "0a 00 00 0a"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  EXPECT_HW_CALL(sw, stateChangedMock(_)).Times(0);
  EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(0);
  sw->packetReceived(pkt->clone());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.too_small.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.mine.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 0);

  // Create an IP pkt to self
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "02 00 01 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(5), DSCP(7), ECN(1), Total Length(20)
    "45  1d  00 14"
    // Identification(0x3456), Flags(0x1), Fragment offset(0x1345)
    "34 56  53 45"
    // TTL(31), Protocol(6), Checksum (0x1234, fake)
    "1F  06  12 34"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.0.1)
    "0a 00 00 01"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(portID);
  pkt->setSrcVlan(vlanID);

  EXPECT_HW_CALL(sw, stateChangedMock(_)).Times(0);
  EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(0);
  sw->packetReceived(pkt->clone());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.too_small.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.mine.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + \
                      "ipv4.wrong_version.sum", 0);
}
