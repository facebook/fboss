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

class HostifManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();
  }
};

TEST_F(HostifManagerTest, createHostifTrap) {
  uint32_t queueId = 4;
  auto trapType = cfg::PacketRxReason::ARP;
  sai_object_id_t trapId =
      saiManagerTable->hostifManager().incRefOrAddHostifTrapGroup(
          trapType, queueId);
  auto trapTypeExpected = saiApiTable->hostifApi().getMemberAttribute(
      HostifApiParameters::MemberAttributes::TrapType(), trapId);
  auto trapTypeCfg =
      saiManagerTable->hostifManager().packetReasonToHostifTrap(trapType);
  EXPECT_EQ(trapTypeCfg, trapTypeExpected);
  saiManagerTable->hostifManager().removeHostifTrap(trapType);
}

TEST_F(HostifManagerTest, sharedHostifTrapGroup) {
  uint32_t queueId = 4;
  auto trapType1 = cfg::PacketRxReason::ARP;
  auto trapType2 = cfg::PacketRxReason::DHCP;
  sai_object_id_t trapId1 =
      saiManagerTable->hostifManager().incRefOrAddHostifTrapGroup(
          trapType1, queueId);
  sai_object_id_t trapId2 =
      saiManagerTable->hostifManager().incRefOrAddHostifTrapGroup(
          trapType2, queueId);
  auto trapGroup1 = saiApiTable->hostifApi().getMemberAttribute(
      HostifApiParameters::MemberAttributes::TrapGroup(), trapId1);
  auto trapGroup2 = saiApiTable->hostifApi().getMemberAttribute(
      HostifApiParameters::MemberAttributes::TrapGroup(), trapId2);
  EXPECT_EQ(trapGroup1, trapGroup2);
  auto queueIdExpected = saiApiTable->hostifApi().getAttribute(
      HostifApiParameters::Attributes::Queue(), trapGroup1);
  EXPECT_EQ(queueIdExpected, queueId);
  saiManagerTable->hostifManager().removeHostifTrap(trapType1);
  saiManagerTable->hostifManager().removeHostifTrap(trapType2);
}

TEST_F(HostifManagerTest, removeHostifTrap) {
  uint32_t queueId = 4;
  auto trapType1 = cfg::PacketRxReason::ARP;
  auto trapType2 = cfg::PacketRxReason::DHCP;
  sai_object_id_t trapId1 =
      saiManagerTable->hostifManager().incRefOrAddHostifTrapGroup(
          trapType1, queueId);
  sai_object_id_t trapId2 =
      saiManagerTable->hostifManager().incRefOrAddHostifTrapGroup(
          trapType2, queueId);
  auto trapGroup1 = saiApiTable->hostifApi().getMemberAttribute(
      HostifApiParameters::MemberAttributes::TrapGroup(), trapId1);
  saiManagerTable->hostifManager().removeHostifTrap(trapType1);
  auto trapGroup2 = saiApiTable->hostifApi().getMemberAttribute(
      HostifApiParameters::MemberAttributes::TrapGroup(), trapId2);
  EXPECT_EQ(trapGroup1, trapGroup2);
  saiManagerTable->hostifManager().removeHostifTrap(trapType2);
}
