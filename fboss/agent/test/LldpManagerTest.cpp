/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/LldpManager.h"
#include <fb303/ServiceData.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>
#include <limits.h>
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/cast.hpp>
#include <gtest/gtest.h>
#include "gmock/gmock.h"

using ::testing::AtLeast;

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::Cursor;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

using ::testing::_;

namespace {
// TODO(joseph5wu) Network control strict priority queue
const uint8_t kNCStrictPriorityQueue = 7;
unique_ptr<HwTestHandle> setupTestHandle(bool enableLldp = false) {
  // Setup a default state object
  // reusing this, as this seems to be legit RSW config under which we should
  // do any unit tests.
  auto switchFlags =
      enableLldp ? SwitchFlags::ENABLE_LLDP : SwitchFlags::DEFAULT;
  auto state = testStateAWithPortsUp();
  addSwitchInfo(
      state,
      cfg::SwitchType::NPU,
      0, /*SwitchId*/
      cfg::AsicType::ASIC_TYPE_MOCK,
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX(),
      0, /* switchIndex*/
      std::nullopt, /* sysPort min*/
      std::nullopt, /*sysPort max()*/
      MockPlatform::getMockLocalMac().toString());
  return createTestHandle(state, switchFlags);
}

TxMatchFn checkLldpPDU() {
  return [=](const TxPacket* pkt) {
    const auto* buf = pkt->buf();
    // create a large enough packet buffer to fill up all LLDP fields
    // portname may be "portX" or "portXX", so length can vary accordingly.
    const auto chainlen = buf->computeChainDataLength();
    const auto minlen = LldpManager::LldpPktSize(
        std::string(""),
        std::string("portX"),
        std::string(""),
        std::string(""));
    const auto maxlen = LldpManager::LldpPktSize(
        std::string(HOST_NAME_MAX, 'x'),
        std::string("portXX"),
        std::string(255, 'x'),
        std::string("FBOSS"));

    EXPECT_LE(minlen, chainlen);
    EXPECT_LE(chainlen, maxlen);

    Cursor c(buf);

    auto dstMac = PktUtil::readMac(&c);
    if (dstMac.toString() != LldpManager::LLDP_DEST_MAC.toString()) {
      throw FbossError(
          "expected dest MAC to be ",
          LldpManager::LLDP_DEST_MAC,
          "; got ",
          dstMac);
    }

    auto srcMac = PktUtil::readMac(&c);
    if (srcMac.toString() != MockPlatform::getMockLocalMac().toString()) {
      throw FbossError(
          "expected source MAC to be ",
          MockPlatform::getMockLocalMac(),
          "; got ",
          srcMac);
    }
    auto ethertype = c.readBE<uint16_t>();
    XLOG(DBG0) << "\ndstMac is " << dstMac.toString() << " srcMac is "
               << srcMac.toString() << " ethertype is " << ethertype;
    if (ethertype != 0x8100) {
      throw FbossError(
          " expected VLAN tag to be present, found ethertype ",
          ethertype,
          " with srcMac-",
          srcMac);
    }

    // read out vlan tag
    c.readBE<uint16_t>();
    auto innerEthertype = c.readBE<uint16_t>();
    if (innerEthertype != LldpManager::ETHERTYPE_LLDP) {
      throw FbossError(" expected LLDP ethertype, found ", innerEthertype);
    }
    // verify the TLVs here.
    auto chassisTLVType = c.readBE<uint16_t>();
    uint16_t expectedChassisTLVTypeLength =
        ((static_cast<uint16_t>(LldpTlvType::CHASSIS)
          << LldpManager::TLV_TYPE_LEFT_SHIFT_OFFSET) |
         LldpManager::CHASSIS_TLV_LENGTH);
    if (chassisTLVType != expectedChassisTLVTypeLength) {
      throw FbossError(
          "expected chassis tlv type and length -",
          expectedChassisTLVTypeLength,
          " found -",
          chassisTLVType);
    }
    XLOG(DBG0) << "\n ChassisTLV Sub-type - " << c.readBE<uint16_t>()
               << " cpu Mac is " << PktUtil::readMac(&c);
  };
}

TEST(LldpManagerTest, LldpSend) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  EXPECT_HW_CALL(
      sw,
      sendPacketOutOfPortAsync_(
          TxPacketMatcher::createMatcher("Lldp PDU", checkLldpPDU()),
          _,
          std::optional<uint8_t>(kNCStrictPriorityQueue)))
      .Times(AtLeast(1));
  LldpManager lldpManager(sw);
  lldpManager.sendLldpOnAllPorts();
}

TEST(LldpManagerTest, LldpSendPeriodic) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  EXPECT_HW_CALL(
      sw,
      sendPacketOutOfPortAsync_(
          TxPacketMatcher::createMatcher("Lldp PDU", checkLldpPDU()),
          _,
          std::optional<uint8_t>(kNCStrictPriorityQueue)))
      .Times(AtLeast(1));
  LldpManager lldpManager(sw);
  lldpManager.start();
  lldpManager.stop();
}

TEST(LldpManagerTest, NoLldpPktsIfSwitchConfigured) {
  auto handle = setupTestHandle(true /*enableLldp*/);
  auto sw = handle->getSw();

  EXPECT_HW_CALL(
      sw,
      sendPacketOutOfPortAsync_(
          TxPacketMatcher::createMatcher("Lldp PDU", checkLldpPDU()),
          _,
          std::optional<uint8_t>(kNCStrictPriorityQueue)))
      .Times(AtLeast(0));
}

TEST(LldpManagerTest, LldpPktsPostConfigured) {
  auto handle = setupTestHandle(true /*enableLldp*/);
  auto sw = handle->getSw();

  EXPECT_HW_CALL(
      sw,
      sendPacketOutOfPortAsync_(
          TxPacketMatcher::createMatcher("Lldp PDU", checkLldpPDU()),
          _,
          std::optional<uint8_t>(kNCStrictPriorityQueue)))
      .Times(AtLeast(1));
  // Initial state applied, no more config to apply
  sw->initialConfigApplied(std::chrono::steady_clock::now());
}

TEST(LldpManagerTest, NotEnabledTest) {
  // Setup switch without flags enabling LLDP, and
  // send an LLDP frame nevertheless. Used to segfault
  // because of NULL dereference.
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  PortID portID(1);
  VlanID vlanID(1);

  // Cache the current stats
  CounterCache counters(sw);

  // Random parseable LLDP packet found using the fuzzer
  auto pkt = PktUtil::parseHexData(
      "02 00 01 00 00 01 02 00 02 01 02 03"
      "81 00 00 01 88 cc 02 0d 00 14 34 56"
      "53 0c 1f 06 12 34 01 02 03 04 0a 32"
      "00 73 21 21 21 4a 21 02 02 06 02 02"
      "00 00 00 00 f2 00 00 0d 0d 0d 0d 0d"
      "00 00 00 00 00 94 94 94 94 00 00 3b"
      "3b de 00 00");

  handle->rxPacket(std::make_unique<folly::IOBuf>(pkt), portID, vlanID);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.unhandled.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "lldp.recvd.sum", 0);
}

TEST(LldpManagerTest, LldpParse) {
  auto lldpParseHelper = [](cfg::SwitchType switchType,
                            std::optional<VlanID> vlanID) {
    cfg::SwitchConfig config = testConfigA(switchType);
    *config.ports()[0].routable() = true;

    auto handle = createTestHandle(&config, SwitchFlags::ENABLE_LLDP);
    auto sw = handle->getSw();

    // Cache the current stats
    CounterCache counters(sw);

    auto pkt = LldpManager::createLldpPkt(
        sw,
        MacAddress("2:2:2:2:2:10"),
        vlanID,
        "somesysname0",
        "portname",
        "someportdesc0",
        1,
        LldpManager::SYSTEM_CAPABILITY_ROUTER);

    handle->rxPacket(
        std::make_unique<folly::IOBuf>(*pkt->buf()), PortID(1), vlanID);

    counters.update();
    counters.checkDelta(
        SwitchStats::kCounterPrefix + "trapped.unhandled.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "lldp.recvd.sum", 1);
    counters.checkDelta(
        SwitchStats::kCounterPrefix + "lldp.validate_mismatch.sum", 0);
  };

  lldpParseHelper(cfg::SwitchType::NPU, VlanID(1));
  lldpParseHelper(cfg::SwitchType::VOQ, std::nullopt /* vlanID */);
}

TEST(LldpManagerTest, LldpValidationPass) {
  auto lldpValidationPassHelper = [](cfg::SwitchType switchType,
                                     std::optional<VlanID> vlanID) {
    cfg::SwitchConfig config = testConfigA(switchType);
    *config.ports()[0].routable() = true;
    config.ports()[0].Port::name() = "FooP0";
    config.ports()[0].Port::description() = "FooP0 Port Description here";

    config.ports()[0].expectedLLDPValues()[cfg::LLDPTag::SYSTEM_NAME] =
        "somesysname0";
    config.ports()[0].expectedLLDPValues()[cfg::LLDPTag::PORT_DESC] =
        "someportdesc0";

    auto handle = createTestHandle(&config, SwitchFlags::ENABLE_LLDP);
    auto sw = handle->getSw();

    // Cache the current stats
    CounterCache counters(sw);

    auto pkt = LldpManager::createLldpPkt(
        sw,
        MacAddress("2:2:2:2:2:10"),
        vlanID,
        "somesysname0",
        "portname",
        "someportdesc0",
        120,
        LldpManager::SYSTEM_CAPABILITY_ROUTER);

    handle->rxPacket(
        std::make_unique<folly::IOBuf>(*pkt->buf()), PortID(1), vlanID);

    sw->updateStats();
    counters.update();
    counters.checkDelta(
        SwitchStats::kCounterPrefix + "trapped.unhandled.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "lldp.recvd.sum", 1);
    counters.checkDelta(
        SwitchStats::kCounterPrefix + "lldp.validate_mismatch.sum", 0);
    counters.checkDelta(
        SwitchStats::kCounterPrefix + "lldp.neighbors_size.sum", 1);
  };

  lldpValidationPassHelper(cfg::SwitchType::NPU, VlanID(1));
  lldpValidationPassHelper(cfg::SwitchType::VOQ, std::nullopt /* vlanID */);
}

TEST(LldpManagerTest, LldpValidationFail) {
  auto lldpValidationFailHelper = [](cfg::SwitchType switchType,
                                     PortID portID,
                                     std::optional<VlanID> vlanID) {
    cfg::SwitchConfig config = testConfigA(switchType);
    *config.ports()[0].routable() = true;
    config.ports()[0].Port::name() = "FooP0";
    config.ports()[0].Port::description() = "FooP0 Port Description here";

    config.ports()[0].expectedLLDPValues()[cfg::LLDPTag::SYSTEM_NAME] =
        "somesysname0";
    config.ports()[0].expectedLLDPValues()[cfg::LLDPTag::PORT_DESC] =
        "someportdesc0";

    for (const auto& v : *config.ports()[0].expectedLLDPValues()) {
      auto port_name = std::string("<no name set>");
      auto port_name_opt = config.ports()[0].Port::name();
      if (port_name_opt)
        port_name = *port_name_opt;

      XLOG(DBG4) << port_name << ": "
                 << std::to_string(static_cast<int>(v.first)) << " -> "
                 << v.second;
    }

    auto handle = createTestHandle(&config, SwitchFlags::ENABLE_LLDP);
    auto sw = handle->getSw();

    // Cache the current stats
    CounterCache counters(sw);

    auto pkt = LldpManager::createLldpPkt(
        sw,
        MacAddress("2:2:2:2:2:10"),
        vlanID,
        "otherhost",
        "otherport",
        "otherdesc",
        1,
        LldpManager::SYSTEM_CAPABILITY_ROUTER);

    handle->rxPacket(
        std::make_unique<folly::IOBuf>(*pkt->buf()), portID, vlanID);

    counters.update();
    counters.checkDelta(
        SwitchStats::kCounterPrefix + "trapped.unhandled.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "lldp.recvd.sum", 1);
    counters.checkDelta(
        SwitchStats::kCounterPrefix + "lldp.validate_mismatch.sum", 1);
  };

  lldpValidationFailHelper(cfg::SwitchType::NPU, PortID(1), VlanID(1));
  lldpValidationFailHelper(
      cfg::SwitchType::VOQ, PortID(5), std::nullopt /* vlanID */);
}
} // unnamed namespace
