/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class NextHopGroupApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    nextHopGroupApi = std::make_unique<NextHopGroupApi>();
  }

  sai_object_id_t kArsObjectId() const {
    return 50;
  }

  NextHopGroupSaiId createNextHopGroup(
      const sai_next_hop_group_type_t nextHopGroupType) const {
    return nextHopGroupApi->create<SaiNextHopGroupTraits>(
        {nextHopGroupType, kArsObjectId()}, 0);
  }

  NextHopGroupMemberSaiId createNextHopGroupMember(
      const sai_object_id_t nextHopGroupId,
      const sai_object_id_t nextHopId,
      const sai_uint32_t nextHopWeight) const {
    return nextHopGroupApi->create<SaiNextHopGroupMemberTraits>(
        {nextHopGroupId, nextHopId, nextHopWeight}, 0);
  }

  void checkNextHopGroup(const sai_object_id_t& nextHopGroupId) const {
    EXPECT_EQ(nextHopGroupId, fs->nextHopGroupManager.get(nextHopGroupId).id);
  }
  void checkNextHopGroupMember(
      const sai_object_id_t& nextHopGroupId,
      const sai_object_id_t& nextHopGroupMemberId,
      const std::optional<sai_uint32_t>& weight) const {
    EXPECT_EQ(
        nextHopGroupMemberId,
        fs->nextHopGroupManager.getMember(nextHopGroupMemberId).id);
    EXPECT_EQ(
        weight, fs->nextHopGroupManager.getMember(nextHopGroupMemberId).weight);
    EXPECT_EQ(
        nextHopGroupId,
        fs->nextHopGroupManager.getMember(nextHopGroupMemberId).nextHopGroupId);
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<NextHopGroupApi> nextHopGroupApi;
};

TEST_F(NextHopGroupApiTest, createNextHopGroup) {
  auto nextHopGroupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(nextHopGroupId);
}

TEST_F(NextHopGroupApiTest, removeNextHopGroup) {
  auto nextHopGroupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(nextHopGroupId);
  nextHopGroupApi->remove(nextHopGroupId);
}

TEST_F(NextHopGroupApiTest, getNextHopGroupAttributes) {
  auto nextHopGroupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(nextHopGroupId);

  sai_uint32_t nextHopWeight = 2;
  auto nextHopGroupMemberId =
      createNextHopGroupMember(nextHopGroupId, 42, nextHopWeight);
  checkNextHopGroupMember(nextHopGroupId, nextHopGroupMemberId, nextHopWeight);

  auto nextHopMemberListGot = nextHopGroupApi->getAttribute(
      nextHopGroupId, SaiNextHopGroupTraits::Attributes::NextHopMemberList());
  auto nextHopTypeGot = nextHopGroupApi->getAttribute(
      nextHopGroupId, SaiNextHopGroupTraits::Attributes::Type());
  auto nextHopArsObjectIdGot = nextHopGroupApi->getAttribute(
      nextHopGroupId, SaiNextHopGroupTraits::Attributes::ArsObjectId());

  EXPECT_EQ(nextHopMemberListGot.size(), 1);
  EXPECT_EQ(nextHopTypeGot, SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  EXPECT_EQ(nextHopArsObjectIdGot, kArsObjectId());
}

// SAI spec does not support setting any attribute for next hop group post
// creation.
TEST_F(NextHopGroupApiTest, setNextHopGroupAttributes) {
  auto nextHopGroupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(nextHopGroupId);

  SaiNextHopGroupTraits::Attributes::Type nextHopGroupType{
      SAI_NEXT_HOP_GROUP_TYPE_PROTECTION};
  SaiNextHopGroupTraits::Attributes::NextHopMemberList NextHopMemberList{};
  SaiNextHopGroupTraits::Attributes::ArsObjectId ArsObjectId{60};

  EXPECT_THROW(
      nextHopGroupApi->setAttribute(nextHopGroupId, nextHopGroupType),
      SaiApiError);
  EXPECT_THROW(
      nextHopGroupApi->setAttribute(nextHopGroupId, NextHopMemberList),
      SaiApiError);
  nextHopGroupApi->setAttribute(nextHopGroupId, ArsObjectId);
  auto nextHopArsObjectIdGot = nextHopGroupApi->getAttribute(
      nextHopGroupId, SaiNextHopGroupTraits::Attributes::ArsObjectId());
  EXPECT_EQ(nextHopArsObjectIdGot, 60);
}

TEST_F(NextHopGroupApiTest, createNextHopGroupMember) {
  auto nextHopGroupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(nextHopGroupId);

  sai_uint32_t nextHopWeight = 2;
  auto nextHopGroupMemberId =
      createNextHopGroupMember(nextHopGroupId, 42, nextHopWeight);
  checkNextHopGroupMember(nextHopGroupId, nextHopGroupMemberId, nextHopWeight);
}

TEST_F(NextHopGroupApiTest, removeNextHopGroupMember) {
  auto nextHopGroupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(nextHopGroupId);

  sai_uint32_t nextHopWeight = 2;
  auto nextHopGroupMemberId =
      createNextHopGroupMember(nextHopGroupId, 42, nextHopWeight);
  checkNextHopGroupMember(nextHopGroupId, nextHopGroupMemberId, 2);
  nextHopGroupApi->remove(nextHopGroupMemberId);
}

TEST_F(NextHopGroupApiTest, getNextHopGroupMemberAttributes) {
  auto nextHopGroupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(nextHopGroupId);

  sai_uint32_t nextHopWeight = 2;
  auto nextHopGroupMemberId =
      createNextHopGroupMember(nextHopGroupId, 42, nextHopWeight);
  checkNextHopGroupMember(nextHopGroupId, nextHopGroupMemberId, nextHopWeight);

  auto nextHopIdGot = nextHopGroupApi->getAttribute(
      nextHopGroupMemberId,
      SaiNextHopGroupMemberTraits::Attributes::NextHopId());
  auto nextHopGroupIdGot = nextHopGroupApi->getAttribute(
      nextHopGroupMemberId,
      SaiNextHopGroupMemberTraits::Attributes::NextHopGroupId());
  auto nextHopWeightGot = nextHopGroupApi->getAttribute(
      nextHopGroupMemberId, SaiNextHopGroupMemberTraits::Attributes::Weight());

  EXPECT_EQ(nextHopIdGot, 42);
  EXPECT_EQ(nextHopGroupIdGot, nextHopGroupId);
  EXPECT_EQ(nextHopWeightGot, nextHopWeight);
}

TEST_F(NextHopGroupApiTest, setNextHopGroupMemberAttributes) {
  auto groupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(groupId);

  sai_uint32_t nextHopWeight = 2;
  auto nextHopGroupMemberId =
      createNextHopGroupMember(groupId, 42, nextHopWeight);
  checkNextHopGroupMember(groupId, nextHopGroupMemberId, nextHopWeight);

  SaiNextHopGroupMemberTraits::Attributes::NextHopId nextHopId{42};
  SaiNextHopGroupMemberTraits::Attributes::NextHopGroupId nextHopGroupId{
      groupId};
  SaiNextHopGroupMemberTraits::Attributes::Weight weight{nextHopWeight};

  EXPECT_THROW(
      nextHopGroupApi->setAttribute(nextHopGroupMemberId, nextHopId),
      SaiApiError);
  EXPECT_THROW(
      nextHopGroupApi->setAttribute(nextHopGroupMemberId, nextHopGroupId),
      SaiApiError);
  EXPECT_NO_THROW(nextHopGroupApi->setAttribute(nextHopGroupMemberId, weight));
  auto nextHopWeightGot = nextHopGroupApi->getAttribute(
      nextHopGroupMemberId, SaiNextHopGroupMemberTraits::Attributes::Weight());
  EXPECT_EQ(nextHopWeightGot, nextHopWeight);
}

TEST_F(NextHopGroupApiTest, bulkSetNextHopGroupMemberAttributes) {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  auto groupId = createNextHopGroup(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
  checkNextHopGroup(groupId);

  int numMembers = 2;
  sai_uint32_t nextHopWeight = 2;
  std::vector<SaiNextHopGroupMemberTraits::AdapterKey> memberAdapterKeys;
  for (auto idx = 0; idx < numMembers; idx++) {
    memberAdapterKeys.emplace_back(
        createNextHopGroupMember(groupId, idx, nextHopWeight));
    checkNextHopGroupMember(groupId, memberAdapterKeys[idx], nextHopWeight);
  }

  std::vector<sai_uint32_t> newWeights = {3, 4};
  std::vector<SaiNextHopGroupMemberTraits::Attributes::Weight> weights;
  for (const auto weight : newWeights) {
    weights.emplace_back(weight);
  }
  EXPECT_NO_THROW(
      nextHopGroupApi->bulkSetAttributes(memberAdapterKeys, weights));
  for (auto idx = 0; idx < memberAdapterKeys.size(); idx++) {
    EXPECT_EQ(
        nextHopGroupApi->getAttribute(
            memberAdapterKeys[idx],
            SaiNextHopGroupMemberTraits::Attributes::Weight()),
        newWeights[idx]);
  }
#endif
}

TEST_F(NextHopGroupApiTest, formatNextHopGroupAttributes) {
  SaiNextHopGroupTraits::Attributes::Type t{SAI_NEXT_HOP_GROUP_TYPE_ECMP};
  EXPECT_EQ("Type: 0", fmt::format("{}", t));
  SaiNextHopGroupTraits::Attributes::NextHopMemberList nhml{{42, 100, 3}};
  EXPECT_EQ("NextHopMemberList: [42, 100, 3]", fmt::format("{}", nhml));
  SaiNextHopGroupTraits::Attributes::ArsObjectId a{60};
  EXPECT_EQ("ArsObjectId: 60", fmt::format("{}", a));
}

TEST_F(NextHopGroupApiTest, formatNextHopGroupMemberAttributes) {
  SaiNextHopGroupMemberTraits::Attributes::NextHopGroupId nhgid{42};
  EXPECT_EQ("NextHopGroupId: 42", fmt::format("{}", nhgid));
  SaiNextHopGroupMemberTraits::Attributes::NextHopId nhid{100};
  EXPECT_EQ("NextHopId: 100", fmt::format("{}", nhid));
  SaiNextHopGroupMemberTraits::Attributes::Weight w{90};
  EXPECT_EQ("Weight: 90", fmt::format("{}", w));
}
