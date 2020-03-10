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
      sai_hostif_trap_type_t trapId,
      sai_object_id_t trapGroupId) {
    SaiHostifTrapTraits::Attributes::TrapType trapType(trapId);
    SaiHostifTrapTraits::Attributes::PacketAction packetAction(
        SAI_PACKET_ACTION_TRAP);
    SaiHostifTrapTraits::Attributes::TrapGroup trapGroup(trapGroupId);
    SaiHostifTrapTraits::Attributes::TrapPriority trapPriority(
        SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY);
    SaiHostifTrapTraits::CreateAttributes attributes{
        trapType, packetAction, trapPriority, trapGroup};
    HostifTrapSaiId trap =
        hostifApi->create<SaiHostifTrapTraits>(attributes, 0);
    return trap;
  }

  HostifTrapGroupSaiId createHostifTrapGroup(uint32_t queueId) {
    SaiHostifTrapGroupTraits::Attributes::Queue queue(queueId);
    SaiHostifTrapGroupTraits::CreateAttributes attributes{queue, 0};
    HostifTrapGroupSaiId trapGroup =
        hostifApi->create<SaiHostifTrapGroupTraits>(attributes, 0);
    return trapGroup;
  }
};

TEST_F(HostifApiTest, sendPacket) {
  SaiTxPacketTraits::Attributes::EgressPortOrLag egressPort(10);
  SaiTxPacketTraits::Attributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS);
  SaiTxPacketTraits::TxAttributes a{txType, egressPort};
  folly::StringPiece testPacket = "TESTPACKET";
  SaiHostifApiPacket txPacket{(void*)(testPacket.toString().c_str()),
                              testPacket.toString().length()};
  hostifApi->send(a, 0, txPacket);
}

TEST_F(HostifApiTest, createTrap) {
  sai_hostif_trap_type_t trapType = SAI_HOSTIF_TRAP_TYPE_LACP;
  uint32_t queueId = 10;
  sai_object_id_t trapGroup = createHostifTrapGroup(queueId);
  sai_object_id_t trap = createHostifTrap(trapType, trapGroup);
  EXPECT_EQ(fs->hostIfTrapManager.get(trap).trapType, trapType);
  EXPECT_EQ(
      fs->hostIfTrapManager.get(trap).packetAction, SAI_PACKET_ACTION_TRAP);
  EXPECT_EQ(fs->hostifTrapGroupManager.get(trapGroup).queueId, queueId);
}

TEST_F(HostifApiTest, removeTrap) {
  sai_hostif_trap_type_t trapType = SAI_HOSTIF_TRAP_TYPE_LACP;
  uint32_t queueId = 10;
  auto trapGroup = createHostifTrapGroup(queueId);
  auto trap = createHostifTrap(trapType, trapGroup);
  EXPECT_EQ(fs->hostIfTrapManager.map().size(), 1);
  EXPECT_EQ(fs->hostifTrapGroupManager.map().size(), 1);
  hostifApi->remove(trapGroup);
  hostifApi->remove(trap);
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
