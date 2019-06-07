/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class HostifApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    hostifApi = std::make_unique<HostifApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<HostifApi> hostifApi;

  sai_object_id_t createHostifTrap(
      sai_hostif_trap_type_t trapId,
      sai_object_id_t trapGroupId) {
    HostifApiParameters::MemberAttributes::TrapType trapType(trapId);
    HostifApiParameters::MemberAttributes::PacketAction packetAction(
        SAI_PACKET_ACTION_TRAP);
    HostifApiParameters::MemberAttributes::TrapGroup trapGroup(trapGroupId);
    HostifApiParameters::MemberAttributes::TrapPriority trapPriority(
        SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY);
    HostifApiParameters::MemberAttributes attributes{
        {trapType, packetAction, trapPriority, trapGroup}};
    sai_object_id_t trap = hostifApi->createMember(attributes.attrs(), 0);
    return trap;
  }

  sai_object_id_t createHostifTrapGroup(uint32_t queueId) {
    HostifApiParameters::Attributes::Queue queue(queueId);
    HostifApiParameters::Attributes attributes{{queue, 0}};
    sai_object_id_t trapGroup = hostifApi->create(attributes.attrs(), 0);
    return trapGroup;
  }
};

TEST_F(HostifApiTest, sendPacket) {
  HostifApiParameters::TxPacketAttributes::EgressPortOrLag egressPort(10);
  HostifApiParameters::TxPacketAttributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS);
  HostifApiParameters::TxPacketAttributes a{{txType, egressPort}};
  folly::StringPiece testPacket = "TESTPACKET";
  HostifApiParameters::HostifApiPacket txPacket{
      (void*)(testPacket.toString().c_str()), testPacket.toString().length()};
  hostifApi->send(a.attrs(), 0, txPacket);
}

TEST_F(HostifApiTest, createTrap) {
  sai_hostif_trap_type_t trapType = SAI_HOSTIF_TRAP_TYPE_LACP;
  uint32_t queueId = 10;
  sai_object_id_t trapGroup = createHostifTrapGroup(queueId);
  sai_object_id_t trap = createHostifTrap(trapType, trapGroup);
  EXPECT_EQ(fs->htm.get(trap).trapType, trapType);
  EXPECT_EQ(fs->htm.get(trap).packetAction, SAI_PACKET_ACTION_TRAP);
  EXPECT_EQ(fs->htgm.get(trapGroup).queueId, queueId);
  hostifApi->remove(trapGroup);
  hostifApi->removeMember(trap);
}

TEST_F(HostifApiTest, removeTrap) {
  sai_hostif_trap_type_t trapType = SAI_HOSTIF_TRAP_TYPE_LACP;
  uint32_t queueId = 10;
  sai_object_id_t trapGroup = createHostifTrapGroup(queueId);
  sai_object_id_t trap = createHostifTrap(trapType, trapGroup);
  EXPECT_EQ(fs->htm.map().size(), 1);
  EXPECT_EQ(fs->htgm.map().size(), 1);
  hostifApi->remove(trapGroup);
  hostifApi->removeMember(trap);
  EXPECT_EQ(fs->htm.map().size(), 0);
  EXPECT_EQ(fs->htgm.map().size(), 0);
}
