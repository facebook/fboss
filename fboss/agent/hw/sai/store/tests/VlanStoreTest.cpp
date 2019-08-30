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

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <variant>

using namespace facebook::fboss;

class VlanStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;

  VlanSaiId createVlan(uint16_t vlanId) const {
    return saiApiTable->vlanApi().create2<SaiVlanTraits>({vlanId}, 0);
  }

  VlanMemberSaiId createVlanMember(
      sai_object_id_t vlanId,
      sai_object_id_t bridgePortId) const {
    return saiApiTable->vlanApi().create2<SaiVlanMemberTraits>(
        {vlanId, bridgePortId}, 0);
  }
};

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
