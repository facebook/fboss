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

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class VlanApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    vlanApi = std::make_unique<VlanApi>();
  }
  void checkVlan(VlanSaiId vlanId) const {
    EXPECT_EQ(vlanId, fs->vm.get(vlanId).id);
  }
  void checkVlanMember(VlanSaiId vlanId, VlanMemberSaiId vlanMemberId) const {
    EXPECT_EQ(vlanMemberId, fs->vm.getMember(vlanMemberId).id);
    EXPECT_EQ(vlanId, fs->vm.getMember(vlanMemberId).vlanId);
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<VlanApi> vlanApi;
};

TEST_F(VlanApiTest, createVlan) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
}

TEST_F(VlanApiTest, removeVlan) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  vlanApi->remove(vlanId);
}

TEST_F(VlanApiTest, createVlanMember) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{0};
  auto vlanMemberId = vlanApi->create<SaiVlanMemberTraits>(
      {vlanIdAttribute, bridgePortIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
}

TEST_F(VlanApiTest, multipleVlan) {
  auto vlanId1 = vlanApi->create<SaiVlanTraits>({42}, 0);
  auto vlanId2 = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId1);
  checkVlan(vlanId2);
  EXPECT_NE(vlanId1, vlanId2);
  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId2};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{0};
  auto vlanMemberId = vlanApi->create<SaiVlanMemberTraits>(
      {vlanIdAttribute, bridgePortIdAttribute}, 0);
  checkVlanMember(vlanId2, vlanMemberId);
}

TEST_F(VlanApiTest, removeVlanMember) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{0};
  auto vlanMemberId = vlanApi->create<SaiVlanMemberTraits>(
      {vlanIdAttribute, bridgePortIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
  vlanApi->remove(vlanMemberId);
}

TEST_F(VlanApiTest, multipleVlanMembers) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute1{0};
  auto vlanMemberId1 = vlanApi->create<SaiVlanMemberTraits>(
      {vlanIdAttribute, bridgePortIdAttribute1}, 0);
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute2{1};
  auto vlanMemberId2 = vlanApi->create<SaiVlanMemberTraits>(
      {vlanIdAttribute, bridgePortIdAttribute2}, 0);
  checkVlanMember(vlanId, vlanMemberId1);
  checkVlanMember(vlanId, vlanMemberId2);
  EXPECT_NE(vlanMemberId1, vlanMemberId2);
}

TEST_F(VlanApiTest, getVlanAttribute) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  auto vlanIdGot =
      vlanApi->getAttribute(vlanId, SaiVlanTraits::Attributes::VlanId());
  EXPECT_EQ(42, vlanIdGot);
}

TEST_F(VlanApiTest, getVlanMemberAttribute) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{0};
  auto vlanMemberId = vlanApi->create<SaiVlanMemberTraits>(
      {vlanIdAttribute, bridgePortIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
  auto vlanIdGot = vlanApi->getAttribute(
      vlanMemberId, SaiVlanMemberTraits::Attributes::VlanId());
  EXPECT_EQ(vlanId, vlanIdGot);
}

TEST_F(VlanApiTest, setVlanMemberAttribute) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{0};
  auto vlanMemberId = vlanApi->create<SaiVlanMemberTraits>(
      {vlanIdAttribute, bridgePortIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
  auto bridgePortId = 42;
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute2(
      bridgePortId);
  vlanApi->setAttribute(vlanMemberId, bridgePortIdAttribute2);
  auto bridgePortIdGot = vlanApi->getAttribute(
      vlanMemberId, SaiVlanMemberTraits::Attributes::BridgePortId());
  EXPECT_EQ(bridgePortId, bridgePortIdGot);
}

/*
 * This test doesn't compile -- it represents using a Traits that doesn't
 * belong to the API. (trying to create a next hop group with the vlan api)
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
TEST_F(VlanApiTest, doesntCompile) {
  auto vlanId = vlanApi->create<SaiNextHopGroupTraits>({10}, 0);
}
*/
