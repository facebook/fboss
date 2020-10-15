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
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class HostifTrapStoreTest : public SaiStoreTest {
 public:
  HostifTrapGroupSaiId createTrapGroup(uint32_t queue) {
    SaiHostifTrapGroupTraits::CreateAttributes c{queue, std::nullopt};
    return saiApiTable->hostifApi().create<SaiHostifTrapGroupTraits>(c, 0);
  }

  HostifTrapSaiId createTrap(int32_t trapType) {
    SaiHostifTrapTraits::CreateAttributes c{
        trapType, SAI_PACKET_ACTION_TRAP, std::nullopt, std::nullopt};
    return saiApiTable->hostifApi().create<SaiHostifTrapTraits>(c, 0);
  }
};

TEST_F(HostifTrapStoreTest, loadHostifTrapGroups) {
  auto hostifTrapGroupSaiId1 = createTrapGroup(2);
  auto hostifTrapGroupSaiId2 = createTrapGroup(3);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiHostifTrapGroupTraits>();

  SaiHostifTrapGroupTraits::AdapterHostKey k1{2};
  SaiHostifTrapGroupTraits::AdapterHostKey k2{3};
  auto got = store.get(k1);
  EXPECT_EQ(got->adapterKey(), hostifTrapGroupSaiId1);
  got = store.get(k2);
  EXPECT_EQ(got->adapterKey(), hostifTrapGroupSaiId2);
}

TEST_F(HostifTrapStoreTest, loadHostifTrap) {
  auto hostifTrapSaiId1 = createTrap(SAI_HOSTIF_TRAP_TYPE_IP2ME);
  auto hostifTrapSaiId2 =
      createTrap(SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiHostifTrapTraits>();

  SaiHostifTrapTraits::AdapterHostKey k1{SAI_HOSTIF_TRAP_TYPE_IP2ME};
  SaiHostifTrapTraits::AdapterHostKey k2{
      SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY};
  auto got = store.get(k1);
  EXPECT_EQ(got->adapterKey(), hostifTrapSaiId1);
  got = store.get(k2);
  EXPECT_EQ(got->adapterKey(), hostifTrapSaiId2);
}

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

  SaiHostifTrapTraits::Attributes::PacketAction packetAction{};
  EXPECT_EQ(
      saiApiTable->hostifApi().getAttribute(obj.adapterKey(), packetAction),
      SAI_PACKET_ACTION_DROP);
}

TEST_F(HostifTrapStoreTest, hostifTrapSerDeser) {
  auto hostifTrapSaiId = createTrap(SAI_HOSTIF_TRAP_TYPE_IP2ME);
  verifyAdapterKeySerDeser<SaiHostifTrapTraits>({hostifTrapSaiId});
}

TEST_F(HostifTrapStoreTest, hostifTrapGroupSerDeser) {
  auto hostifTrapGroupSaiId = createTrapGroup(2);
  verifyAdapterKeySerDeser<SaiHostifTrapGroupTraits>({hostifTrapGroupSaiId});
}

TEST_F(HostifTrapStoreTest, hostifTrapToStr) {
  std::ignore = createTrap(SAI_HOSTIF_TRAP_TYPE_IP2ME);
  verifyToStr<SaiHostifTrapTraits>();
}

TEST_F(HostifTrapStoreTest, hostifTrapGroupToStr) {
  std::ignore = createTrapGroup(2);
  verifyToStr<SaiHostifTrapGroupTraits>();
}
