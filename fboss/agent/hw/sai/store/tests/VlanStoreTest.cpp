/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class VlanStoreTest : public SaiStoreTest {
 public:
  VlanSaiId createVlan(uint16_t vlanId) const {
    return saiApiTable->vlanApi().create<SaiVlanTraits>({vlanId}, 0);
  }

  VlanMemberSaiId createVlanMember(
      sai_object_id_t vlanId,
      sai_object_id_t bridgePortId) const {
    return saiApiTable->vlanApi().create<SaiVlanMemberTraits>(
        {vlanId, bridgePortId}, 0);
  }
};

TEST_F(VlanStoreTest, loadVlans) {
  auto vlanSaiId = createVlan(42);
  auto vlanSaiId2 = createVlan(400);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiVlanTraits>();

  auto got = store.get(SaiVlanTraits::Attributes::VlanId{42});
  EXPECT_EQ(got->adapterKey(), vlanSaiId);
  got = store.get(SaiVlanTraits::Attributes::VlanId{400});
  EXPECT_EQ(got->adapterKey(), vlanSaiId2);
}

TEST_F(VlanStoreTest, loadVlanMember) {
  auto vlanId = createVlan(42);
  auto vlanMemberId = createVlanMember(vlanId, 10);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiVlanMemberTraits>();

  auto got = store.get(SaiVlanMemberTraits::Attributes::BridgePortId{10});
  EXPECT_EQ(got->adapterKey(), vlanMemberId);
}

TEST_F(VlanStoreTest, vlanLoadCtor) {
  auto vlanId = createVlan(42);
  SaiObject<SaiVlanTraits> obj(vlanId);
  EXPECT_EQ(obj.adapterKey(), vlanId);
}

TEST_F(VlanStoreTest, vlanMemberLoadCtor) {
  auto vlanId = createVlan(42);
  auto vlanMemberId = createVlanMember(vlanId, 10);
  SaiObject<SaiVlanMemberTraits> obj(vlanMemberId);
  EXPECT_EQ(obj.adapterKey(), vlanMemberId);
}

TEST_F(VlanStoreTest, vlanCreateCtor) {
  SaiVlanTraits::CreateAttributes c{42};
  SaiVlanTraits::AdapterHostKey k{42};
  SaiObject<SaiVlanTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(Vlan, VlanId, obj.attributes()), 42);
}

TEST_F(VlanStoreTest, vlanMemberCreateCtor) {
  auto vlanId = createVlan(42);
  SaiVlanMemberTraits::CreateAttributes c{vlanId, 10};
  SaiVlanMemberTraits::AdapterHostKey k{10};
  SaiObject<SaiVlanMemberTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(VlanMember, VlanId, obj.attributes()), vlanId);
  EXPECT_EQ(GET_ATTR(VlanMember, BridgePortId, obj.attributes()), 10);
}

TEST_F(VlanStoreTest, serDeserVlanStore) {
  auto vlanSaiId = createVlan(42);
  verifyAdapterKeySerDeser<SaiVlanTraits>({vlanSaiId});
}

TEST_F(VlanStoreTest, toStrVlanStore) {
  std::ignore = createVlan(42);
  verifyToStr<SaiVlanTraits>();
}

TEST_F(VlanStoreTest, serDeserVlanMemberStore) {
  auto vlanId = createVlan(42);
  auto vlanMemberId = createVlanMember(vlanId, 10);

  verifyAdapterKeySerDeser<SaiVlanMemberTraits>({vlanMemberId});
}

TEST_F(VlanStoreTest, toStrVlanMemberStore) {
  auto vlanId = createVlan(42);
  std::ignore = createVlanMember(vlanId, 10);
  verifyToStr<SaiVlanMemberTraits>();
}
