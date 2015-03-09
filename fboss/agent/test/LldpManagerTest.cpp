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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/LldpManager.h"

#include <boost/cast.hpp>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
using ::testing::AtLeast;


using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::Cursor;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

using ::testing::_;

namespace {

const MacAddress testLocalMac = MacAddress("00:00:00:00:00:02");
unique_ptr<SwSwitch> setupSwitch() {
  // Setup a default state object
  // reusing this, as this seems to be legit RSW config under which we should
  // do any unit tests.
  auto state = testStateA();
  return createMockSw(state, testLocalMac);
}

TxMatchFn checkLldpPDU() {
   return [=](const TxPacket* pkt) {
     const auto* buf = pkt->buf();
     // create a large enough packet buffer to fill up all LLDP fields
     EXPECT_EQ(98, buf->computeChainDataLength());

     Cursor c(buf);

     auto dstMac = PktUtil::readMac(&c);
     if (dstMac.toString() != LldpManager::LLDP_DEST_MAC.toString()) {
         throw FbossError("expected dest MAC to be ",
                 LldpManager::LLDP_DEST_MAC, "; got ", dstMac);
     }

     auto srcMac = PktUtil::readMac(&c);
     if (srcMac.toString() != testLocalMac.toString()) {
         throw FbossError("expected source MAC to be ",
                 testLocalMac, "; got ", srcMac);
     }
     auto ethertype = c.readBE<uint16_t>();
     VLOG(0) << "\ndstMac is " << dstMac.toString() << " srcMac is " <<
         srcMac.toString() << " ethertype is " << ethertype;
     if (ethertype != 0x8100) {
         throw FbossError(
                 " expected VLAN tag to be present, found ethertype ",
                 ethertype, " with srcMac-", srcMac);
     }

     auto tag = c.readBE<uint16_t>();
     auto innerEthertype = c.readBE<uint16_t>();
     if (innerEthertype != LldpManager::ETHERTYPE_LLDP) {
         throw FbossError(" expected LLDP ethertype, found ",
                 innerEthertype);
     }
     // verify the TLVs here.
     auto chassisTLVType = c.readBE<uint16_t>();
     uint16_t expectedChassisTLVTypeLength = (
             (LldpManager::CHASSIS_TLV_TYPE <<
              LldpManager::TLV_TYPE_LEFT_SHIFT_OFFSET) |
             LldpManager::CHASSIS_TLV_LENGTH );
     if (chassisTLVType != expectedChassisTLVTypeLength) {
         throw FbossError("expected chassis tlv type and length -",
                 expectedChassisTLVTypeLength, " found -", chassisTLVType);
     }
     VLOG(0) << "\n ChassisTLV Sub-type - " << c.readBE<uint16_t>() <<
         " cpu Mac is " << PktUtil::readMac(&c);
   };
}

TEST(LldpManagerTest, LldpSend) {
  auto sw = setupSwitch();
  //setAllPortsUp(sw->getState());
  SwSwitch* swPtr = sw.get();
  EXPECT_HW_CALL(sw, sendPacketOutOfPort_(TxPacketMatcher::createMatcher(
                  "Lldp PDU", checkLldpPDU()))).Times(AtLeast(1));
  LldpManager lldpManager(swPtr);
  lldpManager.sendLldpOnAllPorts(false);
}
} // unnamed namespace
