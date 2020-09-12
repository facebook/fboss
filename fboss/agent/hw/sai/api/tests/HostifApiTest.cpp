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

  HostifTrapSaiId createHostifTrap(
      sai_hostif_trap_type_t trapType,
      sai_object_id_t trapGroupId) {
    SaiHostifTrapTraits::Attributes::PacketAction packetAction(
        SAI_PACKET_ACTION_TRAP);
    SaiHostifTrapTraits::Attributes::TrapGroup trapGroup(trapGroupId);
    SaiHostifTrapTraits::Attributes::TrapPriority trapPriority(
        SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY);
    SaiHostifTrapTraits::CreateAttributes attributes{
        trapType, packetAction, trapPriority, trapGroup};
    HostifTrapSaiId trapId =
        hostifApi->create<SaiHostifTrapTraits>(attributes, 0);
    return trapId;
  }

  HostifTrapGroupSaiId createHostifTrapGroup(uint32_t queueId) {
    SaiHostifTrapGroupTraits::Attributes::Queue queue(queueId);
    SaiHostifTrapGroupTraits::CreateAttributes attributes{queue, 0};
    HostifTrapGroupSaiId trapGroupId =
        hostifApi->create<SaiHostifTrapGroupTraits>(attributes, 0);
    return trapGroupId;
  }
};

TEST_F(HostifApiTest, sendPacket) {
  SaiTxPacketTraits::Attributes::EgressPortOrLag egressPort(10);
  SaiTxPacketTraits::Attributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS);
  SaiTxPacketTraits::Attributes::EgressQueueIndex egressQueueIndex(7);
  SaiTxPacketTraits::TxAttributes a{txType, egressPort, egressQueueIndex};
  folly::StringPiece testPacket = "TESTPACKET";
  SaiHostifApiPacket txPacket{(void*)(testPacket.toString().c_str()),
                              testPacket.toString().length()};
  hostifApi->send(a, 0, txPacket);
}

TEST_F(HostifApiTest, createTrap) {
  sai_hostif_trap_type_t trapType = SAI_HOSTIF_TRAP_TYPE_LACP;
  uint32_t queueId = 10;
  sai_object_id_t trapGroupId = createHostifTrapGroup(queueId);
  sai_object_id_t trapId = createHostifTrap(trapType, trapGroupId);
  EXPECT_EQ(fs->hostIfTrapManager.get(trapId).trapType, trapType);
  EXPECT_EQ(
      fs->hostIfTrapManager.get(trapId).packetAction, SAI_PACKET_ACTION_TRAP);
  EXPECT_EQ(fs->hostifTrapGroupManager.get(trapGroupId).queueId, queueId);
}

TEST_F(HostifApiTest, removeTrap) {
  sai_hostif_trap_type_t trapType = SAI_HOSTIF_TRAP_TYPE_LACP;
  uint32_t queueId = 10;
  auto trapGroupId = createHostifTrapGroup(queueId);
  auto trapId = createHostifTrap(trapType, trapGroupId);
  EXPECT_EQ(fs->hostIfTrapManager.map().size(), 1);
  EXPECT_EQ(fs->hostifTrapGroupManager.map().size(), 1);
  hostifApi->remove(trapGroupId);
  hostifApi->remove(trapId);
  EXPECT_EQ(fs->hostIfTrapManager.map().size(), 0);
  EXPECT_EQ(fs->hostifTrapGroupManager.map().size(), 0);
}

TEST_F(HostifApiTest, formatTrapGroupAttributes) {
  SaiHostifTrapGroupTraits::Attributes::Queue q{7};
  EXPECT_EQ("Queue: 7", fmt::format("{}", q));
  SaiHostifTrapGroupTraits::Attributes::Policer p{42};
  EXPECT_EQ("Policer: 42", fmt::format("{}", p));
}

TEST_F(HostifApiTest, formatTrapAttributes) {
  SaiHostifTrapTraits::Attributes::TrapType tt(
      SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST);
  EXPECT_EQ("TrapType: 8192", fmt::format("{}", tt));
  SaiHostifTrapTraits::Attributes::PacketAction pa(SAI_PACKET_ACTION_TRAP);
  EXPECT_EQ("PacketAction: 4", fmt::format("{}", pa));
  SaiHostifTrapTraits::Attributes::TrapGroup tg(42);
  EXPECT_EQ("TrapGroup: 42", fmt::format("{}", tg));
  SaiHostifTrapTraits::Attributes::TrapPriority tp(3);
  EXPECT_EQ("TrapPriority: 3", fmt::format("{}", tp));
}

TEST_F(HostifApiTest, getTrapGroupAttributes) {
  HostifTrapGroupSaiId trapGroupId = createHostifTrapGroup(1);

  auto trapGroupQueueGot = hostifApi->getAttribute(
      trapGroupId, SaiHostifTrapGroupTraits::Attributes::Queue());
  auto trapGroupPolicierGot = hostifApi->getAttribute(
      trapGroupId, SaiHostifTrapGroupTraits::Attributes::Policer());

  EXPECT_EQ(trapGroupQueueGot, 1);
  EXPECT_EQ(trapGroupPolicierGot, 0);
}

TEST_F(HostifApiTest, setTrapGroupAttributes) {
  HostifTrapGroupSaiId trapGroupId = createHostifTrapGroup(1);

  SaiHostifTrapGroupTraits::Attributes::Queue queue(42);
  SaiHostifTrapGroupTraits::Attributes::Policer policier(42);

  // SAI spec does not support setting any attribute for trap group post
  // creation.
  EXPECT_THROW(hostifApi->setAttribute(trapGroupId, queue), SaiApiError);
  EXPECT_THROW(hostifApi->setAttribute(trapGroupId, policier), SaiApiError);
}

TEST_F(HostifApiTest, getTrapAttributes) {
  HostifTrapGroupSaiId trapGroupId = createHostifTrapGroup(1);
  HostifTrapSaiId trapId =
      createHostifTrap(SAI_HOSTIF_TRAP_TYPE_LACP, trapGroupId);

  auto trapPacketActionGot = hostifApi->getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::PacketAction());
  auto trapGroupGot = hostifApi->getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::TrapGroup());
  auto trapPriorityGot = hostifApi->getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::TrapPriority());
  auto trapTypeGot = hostifApi->getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::TrapType());

  EXPECT_EQ(trapPacketActionGot, SAI_PACKET_ACTION_TRAP);
  EXPECT_EQ(trapGroupGot, trapGroupId);
  EXPECT_EQ(trapPriorityGot, SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY);
  EXPECT_EQ(trapTypeGot, SAI_HOSTIF_TRAP_TYPE_LACP);
}

TEST_F(HostifApiTest, setTrapAttributes) {
  HostifTrapGroupSaiId trapGroupId = createHostifTrapGroup(1);
  HostifTrapSaiId trapId =
      createHostifTrap(SAI_HOSTIF_TRAP_TYPE_LACP, trapGroupId);

  SaiHostifTrapTraits::Attributes::PacketAction packetAction(
      SAI_PACKET_ACTION_LOG);
  SaiHostifTrapTraits::Attributes::TrapGroup trapGroup(42);
  SaiHostifTrapTraits::Attributes::TrapPriority trapPriority(
      SAI_SWITCH_ATTR_ACL_ENTRY_MAXIMUM_PRIORITY);

  hostifApi->setAttribute(trapId, packetAction);
  hostifApi->setAttribute(trapId, trapGroup);
  hostifApi->setAttribute(trapId, trapPriority);

  auto trapPacketActionGot = hostifApi->getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::PacketAction());
  auto trapGroupGot = hostifApi->getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::TrapGroup());
  auto trapPriorityGot = hostifApi->getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::TrapPriority());

  EXPECT_EQ(trapPacketActionGot, SAI_PACKET_ACTION_LOG);
  EXPECT_EQ(trapGroupGot, 42);
  EXPECT_EQ(trapPriorityGot, SAI_SWITCH_ATTR_ACL_ENTRY_MAXIMUM_PRIORITY);

  // SAI spec does not support setting type attribute for Trap post creation.
  SaiHostifTrapTraits::Attributes::TrapType trapType(
      SAI_HOSTIF_TRAP_TYPE_EAPOL);
  EXPECT_THROW(hostifApi->setAttribute(trapId, trapType), SaiApiError);
}
