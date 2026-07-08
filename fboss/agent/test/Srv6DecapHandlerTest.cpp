// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <gtest/gtest.h>
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

class Srv6DecapHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    auto config = testConfigA();
    cfg::MySidConfig mySidConfig;
    mySidConfig.locatorPrefix() = "3001:db8::/32";
    cfg::MySidEntryConfig entry;
    entry.decap() = cfg::DecapMySidConfig{};
    mySidConfig.entries()[0x7fff] = entry;
    config.mySidConfig() = mySidConfig;

    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
    waitForStateUpdates(sw_);
  }

  void TearDown() override {
    FLAGS_enable_nexthop_id_manager = false;
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(Srv6DecapHandlerTest, DecapV6InV6IncrementsCounter) {
  CounterCache counters(sw_);

  // IPv6-in-IPv6: [Eth | outer IPv6 (nextHdr=0x29, dst=MySID) | inner IPv6 +
  // UDP]
  auto buf = PktUtil::parseHexData(
      // dst mac, src mac
      "02 90 fb 43 a2 ec 02 90 fb 43 a2 ec"
      // 802.1q, VLAN 1
      "81 00 00 01"
      // IPv6 ethertype
      "86 dd"
      // outer IPv6: version=6, tc=0, flow=0, payload=48, nextHdr=0x29, hop=64
      "60 00 00 00 00 30 29 40"
      // outer src: 1::1
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 01"
      // outer dst: 3001:db8:7fff:: (MySID)
      "30 01 0d b8 7f ff 00 00 00 00 00 00 00 00 00 00"
      // inner IPv6: version=6, tc=0, flow=0, payload=8, nextHdr=0x11, hop=64
      "60 00 00 00 00 08 11 40"
      // inner src: 1::10
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 10"
      // inner dst: 2401:db00:2110:3001::1 (interface IP)
      "24 01 db 00 21 10 30 01 00 00 00 00 00 00 00 01"
      // UDP: src=9000, dst=9001, len=8, csum=0
      "23 28 23 29 00 08 00 00");

  handle_->rxPacket(
      folly::IOBuf::copyBuffer(buf.data(), buf.length()),
      PortDescriptor(PortID(1)),
      VlanID(1));

  counters.update();
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "srv6.decap_mysid_to_me.sum", 1);
}

TEST_F(Srv6DecapHandlerTest, DecapV4InV6IncrementsCounter) {
  CounterCache counters(sw_);

  // IPv4-in-IPv6: [Eth | outer IPv6 (nextHdr=0x04, dst=MySID) | inner IPv4 +
  // UDP]
  auto buf = PktUtil::parseHexData(
      // dst mac, src mac
      "02 90 fb 43 a2 ec 02 90 fb 43 a2 ec"
      // 802.1q, VLAN 1
      "81 00 00 01"
      // IPv6 ethertype
      "86 dd"
      // outer IPv6: version=6, tc=0, flow=0, payload=28, nextHdr=0x04, hop=64
      "60 00 00 00 00 1c 04 40"
      // outer src: 1::1
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 01"
      // outer dst: 3001:db8:7fff:: (MySID)
      "30 01 0d b8 7f ff 00 00 00 00 00 00 00 00 00 00"
      // inner IPv4: ver=4, ihl=5, tos=0, len=28, id=0, flags=0, ttl=64,
      // proto=17
      "45 00 00 1c 00 00 00 00 40 11 00 00"
      // inner src: 10.0.0.1
      "0a 00 00 01"
      // inner dst: 10.0.0.1
      "0a 00 00 01"
      // UDP: src=9000, dst=9001, len=8, csum=0
      "23 28 23 29 00 08 00 00");

  handle_->rxPacket(
      folly::IOBuf::copyBuffer(buf.data(), buf.length()),
      PortDescriptor(PortID(1)),
      VlanID(1));

  counters.update();
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "srv6.decap_mysid_to_me.sum", 1);
}

TEST_F(Srv6DecapHandlerTest, NonMySidDoesNotIncrement) {
  CounterCache counters(sw_);

  // IPv6-in-IPv6 with outer dst NOT matching MySID
  auto buf = PktUtil::parseHexData(
      // dst mac, src mac
      "02 90 fb 43 a2 ec 02 90 fb 43 a2 ec"
      // 802.1q, VLAN 1
      "81 00 00 01"
      // IPv6 ethertype
      "86 dd"
      // outer IPv6: nextHdr=0x29
      "60 00 00 00 00 30 29 40"
      // outer src: 1::1
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 01"
      // outer dst: 2001::1 (NOT a MySID)
      "20 01 00 00 00 00 00 00 00 00 00 00 00 00 00 01"
      // inner IPv6
      "60 00 00 00 00 08 11 40"
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 10"
      "24 01 db 00 21 10 30 01 00 00 00 00 00 00 00 01"
      // UDP
      "23 28 23 29 00 08 00 00");

  handle_->rxPacket(
      folly::IOBuf::copyBuffer(buf.data(), buf.length()),
      PortDescriptor(PortID(1)),
      VlanID(1));

  counters.update();
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "srv6.decap_mysid_to_me.sum", 0);
}

TEST_F(Srv6DecapHandlerTest, SubnetMatchIncrementsDropCounter) {
  CounterCache counters(sw_);

  // IPv6-in-IPv6 with outer dst in MySID subnet but not exact match
  auto buf = PktUtil::parseHexData(
      // dst mac, src mac
      "02 90 fb 43 a2 ec 02 90 fb 43 a2 ec"
      // 802.1q, VLAN 1
      "81 00 00 01"
      // IPv6 ethertype
      "86 dd"
      // outer IPv6: nextHdr=0x29
      "60 00 00 00 00 30 29 40"
      // outer src: 1::1
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 01"
      // outer dst: 3001:db8:7fff:0001:: (in /48 subnet but not exact MySID)
      "30 01 0d b8 7f ff 00 01 00 00 00 00 00 00 00 00"
      // inner IPv6
      "60 00 00 00 00 08 11 40"
      "00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 10"
      "24 01 db 00 21 10 30 01 00 00 00 00 00 00 00 01"
      // UDP
      "23 28 23 29 00 08 00 00");

  handle_->rxPacket(
      folly::IOBuf::copyBuffer(buf.data(), buf.length()),
      PortDescriptor(PortID(1)),
      VlanID(1));

  counters.update();
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "srv6.decap_mysid_to_me.sum", 0);
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "srv6.non_last_segment_decap_drop.sum", 1);
}
