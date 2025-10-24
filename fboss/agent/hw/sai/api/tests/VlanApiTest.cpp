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

  VlanMemberSaiId createVlanMember(
      const sai_object_id_t vlanId,
      const sai_object_id_t bridgePortId) const {
    SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId};
    SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{
        bridgePortId};
    return vlanApi->create<SaiVlanMemberTraits>(
        {vlanIdAttribute, bridgePortIdAttribute, std::nullopt}, 0);
  }

  VlanMemberSaiId createVlanMemberWithTaggingMode(
      const sai_object_id_t vlanId,
      const sai_object_id_t bridgePortId,
      const sai_vlan_tagging_mode_t taggingMode) const {
    SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{vlanId};
    SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{
        bridgePortId};
    SaiVlanMemberTraits::Attributes::VlanTaggingMode vlanTaggingModeAttribute{
        taggingMode};
    return vlanApi->create<SaiVlanMemberTraits>(
        {vlanIdAttribute, bridgePortIdAttribute, vlanTaggingModeAttribute}, 0);
  }

  void checkVlan(VlanSaiId vlanId) const {
    EXPECT_EQ(vlanId, fs->vlanManager.get(vlanId).id);
  }
  void checkVlanMember(VlanSaiId vlanId, VlanMemberSaiId vlanMemberId) const {
    EXPECT_EQ(vlanMemberId, fs->vlanManager.getMember(vlanMemberId).id);
    EXPECT_EQ(vlanId, fs->vlanManager.getMember(vlanMemberId).vlanId);
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
  auto vlanMemberId = createVlanMember(vlanId, 0);
  checkVlanMember(vlanId, vlanMemberId);

  // Verify default tagging mode is UNTAGGED when not specified
  auto taggingMode = vlanApi->getAttribute(
      vlanMemberId, SaiVlanMemberTraits::Attributes::VlanTaggingMode());
  EXPECT_EQ(taggingMode, SAI_VLAN_TAGGING_MODE_UNTAGGED);
}

TEST_F(VlanApiTest, multipleVlan) {
  auto vlanId1 = vlanApi->create<SaiVlanTraits>({42}, 0);
  auto vlanId2 = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId1);
  checkVlan(vlanId2);
  EXPECT_NE(vlanId1, vlanId2);
  auto vlanMemberId = createVlanMember(vlanId2, 0);
  checkVlanMember(vlanId2, vlanMemberId);
}

TEST_F(VlanApiTest, removeVlanMember) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  auto vlanMemberId = createVlanMember(vlanId, 0);
  checkVlanMember(vlanId, vlanMemberId);
  vlanApi->remove(vlanMemberId);
}

TEST_F(VlanApiTest, multipleVlanMembers) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);

  // Create first member with tagged mode
  auto vlanMemberId1 = createVlanMemberWithTaggingMode(
      vlanId, static_cast<BridgePortSaiId>(0), SAI_VLAN_TAGGING_MODE_TAGGED);

  // Create second member with untagged mode
  auto vlanMemberId2 = createVlanMemberWithTaggingMode(
      vlanId, static_cast<BridgePortSaiId>(1), SAI_VLAN_TAGGING_MODE_UNTAGGED);

  checkVlanMember(vlanId, vlanMemberId1);
  checkVlanMember(vlanId, vlanMemberId2);
  EXPECT_NE(vlanMemberId1, vlanMemberId2);

  // Verify tagging modes are set correctly for both members
  auto taggingMode1 = vlanApi->getAttribute(
      vlanMemberId1, SaiVlanMemberTraits::Attributes::VlanTaggingMode());
  EXPECT_EQ(taggingMode1, SAI_VLAN_TAGGING_MODE_TAGGED);

  auto taggingMode2 = vlanApi->getAttribute(
      vlanMemberId2, SaiVlanMemberTraits::Attributes::VlanTaggingMode());
  EXPECT_EQ(taggingMode2, SAI_VLAN_TAGGING_MODE_UNTAGGED);
}

TEST_F(VlanApiTest, getVlanAttribute) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  auto vlanMemberId = createVlanMember(vlanId, 0);
  checkVlanMember(vlanId, vlanMemberId);

  auto vlanIdGot =
      vlanApi->getAttribute(vlanId, SaiVlanTraits::Attributes::VlanId());
  auto memberListGot =
      vlanApi->getAttribute(vlanId, SaiVlanTraits::Attributes::MemberList());

  EXPECT_EQ(vlanIdGot, 42);
  EXPECT_EQ(memberListGot.size(), 1);
}

TEST_F(VlanApiTest, setVlanAttribute) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  auto vlanMemberId = createVlanMember(vlanId, 0);
  checkVlanMember(vlanId, vlanMemberId);

  SaiVlanTraits::Attributes::VlanId vlanIdAttribute{42};
  SaiVlanTraits::Attributes::MemberList memberListAttribute{};

  // Vlan does not support setting attributes post creation
  EXPECT_THROW(vlanApi->setAttribute(vlanId, vlanIdAttribute), SaiApiError);
  EXPECT_THROW(vlanApi->setAttribute(vlanId, memberListAttribute), SaiApiError);
}

TEST_F(VlanApiTest, getVlanMemberAttribute) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);

  BridgePortSaiId bridgePortId{0};
  auto vlanMemberId = createVlanMemberWithTaggingMode(
      vlanId, bridgePortId, SAI_VLAN_TAGGING_MODE_TAGGED);
  checkVlanMember(vlanId, vlanMemberId);

  auto bridgePortIdGot = vlanApi->getAttribute(
      vlanMemberId, SaiVlanMemberTraits::Attributes::BridgePortId());
  auto vlanIdGot = vlanApi->getAttribute(
      vlanMemberId, SaiVlanMemberTraits::Attributes::VlanId());
  auto taggingModeGot = vlanApi->getAttribute(
      vlanMemberId, SaiVlanMemberTraits::Attributes::VlanTaggingMode());

  EXPECT_EQ(vlanId, vlanIdGot);
  EXPECT_EQ(bridgePortId, bridgePortIdGot);
  EXPECT_EQ(taggingModeGot, SAI_VLAN_TAGGING_MODE_TAGGED);
}

TEST_F(VlanApiTest, setVlanMemberAttribute) {
  auto vlanId = vlanApi->create<SaiVlanTraits>({42}, 0);
  checkVlan(vlanId);
  auto vlanMemberId = createVlanMember(vlanId, 0);
  checkVlanMember(vlanId, vlanMemberId);

  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{0};
  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{42};

  // Sai spec does not allow setting Vlan member attributes post creation
  EXPECT_THROW(
      vlanApi->setAttribute(vlanMemberId, bridgePortIdAttribute), SaiApiError);
  EXPECT_THROW(
      vlanApi->setAttribute(vlanMemberId, vlanIdAttribute), SaiApiError);
}

TEST_F(VlanApiTest, formatVlanVlanIdAttribute) {
  SaiVlanTraits::Attributes::VlanId vi{42};
  std::string expected("VlanId: 42");
  EXPECT_EQ(expected, fmt::format("{}", vi));
}

TEST_F(VlanApiTest, formatVlanMemberVlanIdAttribute) {
  SaiVlanMemberTraits::Attributes::VlanId vi{42};
  std::string expected("VlanId: 42");
  EXPECT_EQ(expected, fmt::format("{}", vi));
}

TEST_F(VlanApiTest, formatVlanMemberTaggingModeAttribute) {
  SaiVlanMemberTraits::Attributes::VlanTaggingMode taggingMode{
      SAI_VLAN_TAGGING_MODE_TAGGED};
  std::string expected(
      "VlanTaggingMode: " + std::to_string(SAI_VLAN_TAGGING_MODE_TAGGED));
  EXPECT_EQ(expected, fmt::format("{}", taggingMode));
}

/*
 * This test doesn't compile -- it represents using a Traits that doesn't
 * belong to the API. (trying to create a next hop group with the vlan api)
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
TEST_F(VlanApiTest, doesntCompile) {
  auto vlanId = vlanApi->create<SaiNextHopGroupTraits>({10}, 0);
}
*/
