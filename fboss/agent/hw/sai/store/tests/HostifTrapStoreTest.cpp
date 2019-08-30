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
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class HostifTrapStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;

  HostifTrapGroupSaiId createTrapGroup(uint32_t queue) {
    SaiHostifTrapGroupTraits::CreateAttributes c{queue, std::nullopt};
    return saiApiTable->hostifApi().create2<SaiHostifTrapGroupTraits>(c, 0);
  }

  HostifTrapSaiId createTrap(int32_t trapType) {
    SaiHostifTrapTraits::CreateAttributes c{
        trapType, SAI_PACKET_ACTION_TRAP, std::nullopt, std::nullopt};
    return saiApiTable->hostifApi().create2<SaiHostifTrapTraits>(c, 0);
  }
};

TEST_F(HostifTrapStoreTest, trapGroupLoadCtor) {
  auto id = createTrapGroup(2);
  SaiObject<SaiHostifTrapGroupTraits> trapGroupObj(id);
  EXPECT_EQ(trapGroupObj.adapterKey(), id);
}

TEST_F(HostifTrapStoreTest, trapLoadCtor) {
  auto id = createTrap(SAI_HOSTIF_TRAP_TYPE_IP2ME);
  SaiObject<SaiHostifTrapTraits> trapObj(id);
  EXPECT_EQ(trapObj.adapterKey(), id);
}

TEST_F(HostifTrapStoreTest, trapGroupCreateCtor) {
  SaiHostifTrapGroupTraits::CreateAttributes c{2, std::nullopt};
  SaiObject<SaiHostifTrapGroupTraits> obj({2}, c, 0);
  EXPECT_EQ(GET_ATTR(HostifTrapGroup, Queue, obj.attributes()), 2);
}

TEST_F(HostifTrapStoreTest, trapCreateCtor) {
  SaiHostifTrapTraits::CreateAttributes c{SAI_HOSTIF_TRAP_TYPE_IP2ME,
                                          SAI_PACKET_ACTION_TRAP,
                                          std::nullopt,
                                          std::nullopt};
  SaiObject<SaiHostifTrapTraits> obj({SAI_PACKET_ACTION_TRAP}, c, 0);
  EXPECT_EQ(
      GET_ATTR(HostifTrap, TrapType, obj.attributes()),
      SAI_HOSTIF_TRAP_TYPE_IP2ME);
  EXPECT_EQ(
      GET_ATTR(HostifTrap, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_TRAP);
}

TEST_F(HostifTrapStoreTest, trapSetAction) {
  SaiHostifTrapTraits::CreateAttributes c{SAI_HOSTIF_TRAP_TYPE_IP2ME,
                                          SAI_PACKET_ACTION_TRAP,
                                          std::nullopt,
                                          std::nullopt};
  SaiObject<SaiHostifTrapTraits> obj({SAI_PACKET_ACTION_TRAP}, c, 0);
  EXPECT_EQ(
      GET_ATTR(HostifTrap, TrapType, obj.attributes()),
      SAI_HOSTIF_TRAP_TYPE_IP2ME);
  EXPECT_EQ(
      GET_ATTR(HostifTrap, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_TRAP);
  SaiHostifTrapTraits::CreateAttributes newAttrs{SAI_HOSTIF_TRAP_TYPE_IP2ME,
                                                 SAI_PACKET_ACTION_DROP,
                                                 std::nullopt,
                                                 std::nullopt};
  obj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_ATTR(HostifTrap, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_DROP);
}
