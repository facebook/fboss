/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class AclApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    aclApi = std::make_unique<AclApi>();
  }

  sai_object_id_t kSwitchID() const {
    return 0;
  }

  AclTableSaiId createAclTable() const {
    return aclApi->create<SaiAclTableTraits>(
        {SAI_ACL_STAGE_INGRESS, std::nullopt, std::nullopt, std::nullopt},
        kSwitchID());
  }

  AclEntrySaiId createAclEntry(AclTableSaiId aclTableId) const {
    SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    return aclApi->create<SaiAclEntryTraits>(
        {aclTableIdAttribute, std::nullopt, std::nullopt}, kSwitchID());
  }

  void checkAclTable(AclTableSaiId aclTableId) const {
    EXPECT_EQ(aclTableId, fs->aclTableManager.get(aclTableId).id);
  }

  void checkAclEntry(AclTableSaiId aclTableId, AclEntrySaiId aclEntryId) const {
    EXPECT_EQ(aclEntryId, fs->aclTableManager.getMember(aclEntryId).id);
    EXPECT_EQ(aclTableId, fs->aclTableManager.getMember(aclEntryId).tableId);
  }

  AclTableGroupSaiId createAclTableGroup() const {
    return aclApi->create<SaiAclTableGroupTraits>(
        {SAI_ACL_STAGE_INGRESS, std::nullopt, std::nullopt}, kSwitchID());
  }

  AclTableGroupMemberSaiId createAclTableGroupMember(
      AclTableGroupSaiId aclTableGroupId,
      AclTableSaiId aclTableId) const {
    SaiAclTableGroupMemberTraits::Attributes::TableGroupId
        aclTableGroupTableGroupIdAttribute{aclTableGroupId};
    SaiAclTableGroupMemberTraits::Attributes::TableId
        aclTableGroupTableIdAttribute{aclTableId};
    SaiAclTableGroupMemberTraits::Attributes::Priority
        aclTableGroupPriorityAttribute{1};

    return aclApi->create<SaiAclTableGroupMemberTraits>(
        {aclTableGroupTableGroupIdAttribute,
         aclTableGroupTableIdAttribute,
         aclTableGroupPriorityAttribute},
        kSwitchID());
  }

  void checkAclTableGroup(AclTableGroupSaiId aclTableGroupId) const {
    EXPECT_EQ(
        aclTableGroupId, fs->aclTableGroupManager.get(aclTableGroupId).id);
  }

  void checkAclTableGroupMember(
      AclTableGroupSaiId aclTableGroupId,
      AclTableGroupMemberSaiId aclTableGroupMemberId) {
    EXPECT_EQ(
        aclTableGroupMemberId,
        fs->aclTableGroupManager.getMember(aclTableGroupMemberId).id);
    EXPECT_EQ(
        aclTableGroupId,
        fs->aclTableGroupManager.getMember(aclTableGroupMemberId).tableGroupId);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<AclApi> aclApi;
};

TEST_F(AclApiTest, createAclTable) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);
}

TEST_F(AclApiTest, removeAclTable) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);
  aclApi->remove(aclTableId);
}

TEST_F(AclApiTest, createAclEntry) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);
}

TEST_F(AclApiTest, mulipleAclTables) {
  auto aclTableId1 = createAclTable();
  auto aclTableId2 = createAclTable();
  checkAclTable(aclTableId1);
  checkAclTable(aclTableId2);
  EXPECT_NE(aclTableId1, aclTableId2);

  auto aclEntryId1 = createAclEntry(aclTableId1);
  auto aclEntryId2 = createAclEntry(aclTableId2);
  checkAclEntry(aclTableId1, aclEntryId1);
  checkAclEntry(aclTableId2, aclEntryId2);
}

TEST_F(AclApiTest, removeAclEntry) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  aclApi->remove(aclEntryId);
}

TEST_F(AclApiTest, multipleAclEntries) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId1 = createAclEntry(aclTableId);
  auto aclEntryId2 = createAclEntry(aclTableId);

  checkAclEntry(aclTableId, aclEntryId1);
  checkAclEntry(aclTableId, aclEntryId2);
  EXPECT_NE(aclEntryId1, aclEntryId2);
}

TEST_F(AclApiTest, getAclTableAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  auto aclEntriesGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::EntryList());
  EXPECT_EQ(aclEntriesGot.size(), 1);
  EXPECT_EQ(aclEntriesGot[0], static_cast<uint32_t>(aclEntryId));
}

TEST_F(AclApiTest, getAclEntryAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
  SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute{1};
  auto aclEntryId = aclApi->create<SaiAclEntryTraits>(
      {aclTableIdAttribute, aclPriorityAttribute, std::nullopt}, kSwitchID());
  checkAclEntry(aclTableId, aclEntryId);
  auto aclPriorityGot = aclApi->getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::Priority());
  EXPECT_EQ(aclPriorityAttribute, aclPriorityGot);
}

TEST_F(AclApiTest, setAclTableAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  // SAI spec does not support setting any attribute for ACL table post
  // creation.
  SaiAclTableTraits::Attributes::EntryList entryList{};
  EXPECT_THROW(aclApi->setAttribute(aclTableId, entryList), SaiApiError);
}

TEST_F(AclApiTest, setAclEntryAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
  SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute1{1};
  auto aclEntryId = aclApi->create<SaiAclEntryTraits>(
      {aclTableIdAttribute, aclPriorityAttribute1, std::nullopt}, kSwitchID());
  checkAclEntry(aclTableId, aclEntryId);

  SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute2{2};
  aclApi->setAttribute(aclEntryId, aclPriorityAttribute2);
  auto aclPriorityGot = aclApi->getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::Priority());
  EXPECT_EQ(aclPriorityAttribute2, aclPriorityGot);
}

TEST_F(AclApiTest, formatAclTableStageAttribute) {
  SaiAclTableTraits::Attributes::Stage stage{0};
  std::string expected("Stage: 0");
  EXPECT_EQ(expected, fmt::format("{}", stage));
}

TEST_F(AclApiTest, formatAclEntryAttribute) {
  SaiAclEntryTraits::Attributes::TableId tableId{0};
  std::string expected("TableId: 0");
  EXPECT_EQ(expected, fmt::format("{}", tableId));
}

TEST_F(AclApiTest, createAclTableGroup) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
}

TEST_F(AclApiTest, removeAclTableGroup) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  aclApi->remove(aclTableGroupId);
}

TEST_F(AclApiTest, createAclTableGroupMember) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);
}

TEST_F(AclApiTest, removeAclTableGroupMember) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  aclApi->remove(aclTableGroupMemberId);
}

TEST_F(AclApiTest, multipleAclTableGroupMember) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);

  auto aclTableId1 = createAclTable();
  checkAclTable(aclTableId1);
  auto aclTableId2 = createAclTable();
  checkAclTable(aclTableId2);

  auto aclTableGroupMemberId1 =
      createAclTableGroupMember(aclTableGroupId, aclTableId1);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId1);
  auto aclTableGroupMemberId2 =
      createAclTableGroupMember(aclTableGroupId, aclTableId1);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId2);
}

TEST_F(AclApiTest, getAclTableGroupAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  auto aclTableGroupMembersGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::MemberList());

  EXPECT_EQ(aclTableGroupMembersGot.size(), 1);
  EXPECT_EQ(aclTableGroupMembersGot[0], aclTableGroupMemberId);
}

TEST_F(AclApiTest, getAclTableGroupMemberAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  auto aclTableGroupIdGot = aclApi->getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::TableGroupId());

  EXPECT_EQ(aclTableGroupIdGot, aclTableGroupId);
}

TEST_F(AclApiTest, setAclTableGroupAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);

  // SAI spec does not support setting any attribute for ACL table group post
  // creation.
  SaiAclTableGroupTraits::Attributes::MemberList memberList{};
  EXPECT_THROW(aclApi->setAttribute(aclTableGroupId, memberList), SaiApiError);
}

TEST_F(AclApiTest, setAclTableGroupMemberAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);
  auto aclTableGroupMemberId =
      createAclTableGroupMember(aclTableGroupId, aclTableId);
  checkAclTableGroupMember(aclTableGroupId, aclTableGroupMemberId);

  // SAI spec does not support setting any attribute for ACL table group member
  // post creation.
  SaiAclTableGroupMemberTraits::Attributes::TableGroupId tableGroupId{1};
  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupMemberId, tableGroupId), SaiApiError);
}

TEST_F(AclApiTest, formatAclTableGroupStageAttribute) {
  SaiAclTableGroupTraits::Attributes::Stage stage{0};
  std::string expected("Stage: 0");
  EXPECT_EQ(expected, fmt::format("{}", stage));
}

TEST_F(AclApiTest, formatAclTableGroupMemberAttribute) {
  SaiAclTableGroupMemberTraits::Attributes::TableGroupId tableGroupId{0};
  std::string expected("TableGroupId: 0");
  EXPECT_EQ(expected, fmt::format("{}", tableGroupId));
}
