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
  void checkVlan(const sai_object_id_t& vlanId) const {
    EXPECT_EQ(vlanId, fs->vm.get(vlanId).id);
  }
  void checkVlanMember(
      const sai_object_id_t& vlanId,
      const sai_object_id_t& vlanMemberId) const {
    EXPECT_EQ(vlanMemberId, fs->vm.getMember(vlanMemberId).id);
    EXPECT_EQ(vlanId, fs->vm.getMember(vlanMemberId).vlanId);
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<VlanApi> vlanApi;
};

TEST_F(VlanApiTest, createVlan) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
}

TEST_F(VlanApiTest, removeVlan) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
  vlanApi->remove(vlanId);
}

TEST_F(VlanApiTest, createVlanMember) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
  VlanApiParameters::MemberAttributeType vlanIdAttribute =
      VlanApiParameters::MemberAttributes::VlanId(vlanId);
  auto vlanMemberId = vlanApi->createMember({vlanIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
}

TEST_F(VlanApiTest, multipleVlan) {
  auto vlanId1 = vlanApi->create2({42}, 0);
  auto vlanId2 = vlanApi->create2({42}, 0);
  checkVlan(vlanId1);
  checkVlan(vlanId2);
  EXPECT_NE(vlanId1, vlanId2);
  VlanApiParameters::MemberAttributeType vlanIdAttribute =
      VlanApiParameters::MemberAttributes::VlanId(vlanId2);
  auto vlanMemberId = vlanApi->createMember({vlanIdAttribute}, 0);
  checkVlanMember(vlanId2, vlanMemberId);
}

TEST_F(VlanApiTest, removeVlanMember) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
  VlanApiParameters::MemberAttributeType vlanIdAttribute =
      VlanApiParameters::MemberAttributes::VlanId(vlanId);
  auto vlanMemberId = vlanApi->createMember({vlanIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
  vlanApi->removeMember(vlanMemberId);
}

TEST_F(VlanApiTest, multipleVlanMembers) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
  VlanApiParameters::MemberAttributeType vlanIdAttribute =
      VlanApiParameters::MemberAttributes::VlanId(vlanId);
  auto vlanMemberId1 = vlanApi->createMember({vlanIdAttribute}, 0);
  auto vlanMemberId2 = vlanApi->createMember({vlanIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId1);
  checkVlanMember(vlanId, vlanMemberId2);
  EXPECT_NE(vlanMemberId1, vlanMemberId2);
}

TEST_F(VlanApiTest, getVlanAttribute) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
  auto vlanIdGot =
      vlanApi->getAttribute(VlanApiParameters::Attributes::VlanId(), vlanId);
  EXPECT_EQ(42, vlanIdGot);
}

TEST_F(VlanApiTest, getVlanMemberAttribute) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
  VlanApiParameters::MemberAttributeType vlanIdAttribute =
      VlanApiParameters::MemberAttributes::VlanId(vlanId);
  auto vlanMemberId = vlanApi->createMember({vlanIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
  auto vlanIdGot = vlanApi->getMemberAttribute(
      VlanApiParameters::MemberAttributes::VlanId(), vlanMemberId);
  EXPECT_EQ(vlanId, vlanIdGot);
}

TEST_F(VlanApiTest, setVlanMemberAttribute) {
  auto vlanId = vlanApi->create2({42}, 0);
  checkVlan(vlanId);
  VlanApiParameters::MemberAttributeType vlanIdAttribute =
      VlanApiParameters::MemberAttributes::VlanId(vlanId);
  auto vlanMemberId = vlanApi->createMember({vlanIdAttribute}, 0);
  checkVlanMember(vlanId, vlanMemberId);
  auto bridgePortId = 42;
  VlanApiParameters::MemberAttributes::BridgePortId bridgePortIdAttribute(
      bridgePortId);
  vlanApi->setMemberAttribute(bridgePortIdAttribute, vlanMemberId);
  auto bridgePortIdGot = vlanApi->getMemberAttribute(
      VlanApiParameters::MemberAttributes::BridgePortId(), vlanMemberId);
  EXPECT_EQ(bridgePortId, bridgePortIdGot);
}
