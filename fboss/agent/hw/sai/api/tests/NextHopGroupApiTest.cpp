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
  auto nextHopGroupId = nextHopGroupApi->create<SaiNextHopGroupTraits>(
      {SAI_NEXT_HOP_GROUP_TYPE_ECMP}, 0);
  checkNextHopGroup(nextHopGroupId);
}

TEST_F(NextHopGroupApiTest, removeNextHopGroup) {
  auto nextHopGroupId = nextHopGroupApi->create<SaiNextHopGroupTraits>(
      {SAI_NEXT_HOP_GROUP_TYPE_ECMP}, 0);
  checkNextHopGroup(nextHopGroupId);
  nextHopGroupApi->remove(nextHopGroupId);
}

TEST_F(NextHopGroupApiTest, createNextHopGroupMember) {
  auto nextHopGroupId = nextHopGroupApi->create<SaiNextHopGroupTraits>(
      {SAI_NEXT_HOP_GROUP_TYPE_ECMP}, 0);
  checkNextHopGroup(nextHopGroupId);
  sai_object_id_t nextHopId = 42;
  sai_uint32_t nextHopWeight = 2;

  typename SaiNextHopGroupMemberTraits::CreateAttributes c{
      nextHopGroupId, nextHopId, nextHopWeight};
  auto nextHopGroupMemberId =
      nextHopGroupApi->create<SaiNextHopGroupMemberTraits>(c, 0);
  checkNextHopGroupMember(nextHopGroupId, nextHopGroupMemberId, nextHopWeight);
}

TEST_F(NextHopGroupApiTest, removeNextHopGroupMember) {
  auto nextHopGroupId = nextHopGroupApi->create<SaiNextHopGroupTraits>(
      {SAI_NEXT_HOP_GROUP_TYPE_ECMP}, 0);
  checkNextHopGroup(nextHopGroupId);
  sai_object_id_t nextHopId = 42;
  sai_uint32_t nextHopWeight = 2;
  typename SaiNextHopGroupMemberTraits::CreateAttributes c{
      nextHopGroupId, nextHopId, nextHopWeight};
  auto nextHopGroupMemberId =
      nextHopGroupApi->create<SaiNextHopGroupMemberTraits>(c, 0);
  checkNextHopGroupMember(nextHopGroupId, nextHopGroupMemberId, nextHopWeight);
  nextHopGroupApi->remove(nextHopGroupMemberId);
}

TEST_F(NextHopGroupApiTest, formatNextHopGroupAttributes) {
  SaiNextHopGroupTraits::Attributes::Type t{SAI_NEXT_HOP_GROUP_TYPE_ECMP};
  EXPECT_EQ("Type: 0", fmt::format("{}", t));
  SaiNextHopGroupTraits::Attributes::NextHopMemberList nhml{{42, 100, 3}};
  EXPECT_EQ("NextHopMemberList: {42, 100, 3}", fmt::format("{}", nhml));
}

TEST_F(NextHopGroupApiTest, formatNextHopGroupMemberAttributes) {
  SaiNextHopGroupMemberTraits::Attributes::NextHopGroupId nhgid{42};
  EXPECT_EQ("NextHopGroupId: 42", fmt::format("{}", nhgid));
  SaiNextHopGroupMemberTraits::Attributes::NextHopId nhid{100};
  EXPECT_EQ("NextHopId: 100", fmt::format("{}", nhid));
  SaiNextHopGroupMemberTraits::Attributes::Weight w{90};
  EXPECT_EQ("Weight: 90", fmt::format("{}", w));
}
