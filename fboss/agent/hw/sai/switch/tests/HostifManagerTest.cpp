/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

using namespace facebook::fboss;

class HostifManagerTest : public ManagerTestBase {};

TEST_F(HostifManagerTest, createHostifTrap) {
  uint32_t queueId = 4;
  auto trapType = cfg::PacketRxReason::ARP;
  HostifTrapSaiId trapId =
      saiManagerTable->hostifManager().addHostifTrap(trapType, queueId);
  auto trapTypeExpected = saiApiTable->hostifApi().getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::TrapType{});
  auto trapTypeCfg = SaiHostifManager::packetReasonToHostifTrap(trapType);
  EXPECT_EQ(trapTypeCfg, trapTypeExpected);
}

TEST_F(HostifManagerTest, sharedHostifTrapGroup) {
  uint32_t queueId = 4;
  auto trapType1 = cfg::PacketRxReason::ARP;
  auto trapType2 = cfg::PacketRxReason::DHCP;
  auto& hostifManager = saiManagerTable->hostifManager();
  HostifTrapSaiId trapId1 = hostifManager.addHostifTrap(trapType1, queueId);
  HostifTrapSaiId trapId2 = hostifManager.addHostifTrap(trapType2, queueId);
  auto trapGroup1 = saiApiTable->hostifApi().getAttribute(
      trapId1, SaiHostifTrapTraits::Attributes::TrapGroup{});
  auto trapGroup2 = saiApiTable->hostifApi().getAttribute(
      trapId2, SaiHostifTrapTraits::Attributes::TrapGroup{});
  EXPECT_EQ(trapGroup1, trapGroup2);
  auto queueIdExpected = saiApiTable->hostifApi().getAttribute(
      HostifTrapGroupSaiId{trapGroup1},
      SaiHostifTrapGroupTraits::Attributes::Queue{});
  EXPECT_EQ(queueIdExpected, queueId);
}

TEST_F(HostifManagerTest, removeHostifTrap) {
  uint32_t queueId = 4;
  auto& hostifManager = saiManagerTable->hostifManager();
  auto trapType1 = cfg::PacketRxReason::ARP;
  auto trapType2 = cfg::PacketRxReason::DHCP;
  // create two traps using the same queue
  HostifTrapSaiId trapId1 = hostifManager.addHostifTrap(trapType1, queueId);
  HostifTrapSaiId trapId2 = hostifManager.addHostifTrap(trapType2, queueId);
  auto trapGroup1 = saiApiTable->hostifApi().getAttribute(
      trapId1, SaiHostifTrapTraits::Attributes::TrapGroup{});
  // remove one of them
  saiManagerTable->hostifManager().removeHostifTrap(trapType1);
  // trap group for the queue should still be valid
  auto trapGroup2 = saiApiTable->hostifApi().getAttribute(
      trapId2, SaiHostifTrapTraits::Attributes::TrapGroup{});
  EXPECT_EQ(trapGroup1, trapGroup2);
}
