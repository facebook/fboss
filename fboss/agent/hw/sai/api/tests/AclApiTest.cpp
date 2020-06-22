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

  std::pair<sai_uint8_t, sai_uint8_t> kDscp() const {
    // TOS is 8-bits: 6-bit DSCP followed by 2-bit ECN.
    // mask of 0xFC to match on 6-bit DSCP
    return std::make_pair(10, 0xFC);
  }

  std::pair<sai_uint8_t, sai_uint8_t> kDscp2() const {
    // TOS is 8-bits: 6-bit DSCP followed by 2-bit ECN.
    // mask of 0xFC to match on 6-bit DSCP
    return std::make_pair(20, 0xFC);
  }

  std::pair<sai_uint32_t, sai_uint32_t> kRouteDstUserMeta() const {
    return std::make_pair(11, 0xFFFFFFFF);
  }

  const std::vector<sai_int32_t>& kActionTypeList() const {
    static const std::vector<sai_int32_t> actionTypeList = {
        SAI_ACL_ACTION_TYPE_PACKET_ACTION,
        SAI_ACL_ACTION_TYPE_MIRROR_INGRESS,
        SAI_ACL_ACTION_TYPE_MIRROR_EGRESS,
        SAI_ACL_ACTION_TYPE_SET_TC,
        SAI_ACL_ACTION_TYPE_SET_DSCP};

    return actionTypeList;
  }

  AclTableSaiId createAclTable() const {
    SaiAclTableTraits::Attributes::Stage aclTableStageAttribute{
        SAI_ACL_STAGE_INGRESS};
    std::vector<sai_int32_t> aclTableBindPointTypeListAttribute{
        SAI_ACL_BIND_POINT_TYPE_PORT};

    return aclApi->create<SaiAclTableTraits>(
        {
            aclTableStageAttribute,
            aclTableBindPointTypeListAttribute,
            kActionTypeList(),
            true, // srcIpv6
            true, // dstIpv6
            true, // l4SrcPort
            true, // l4DstPort
            true, // ipProtocol
            true, // tcpFlags
            true, // inPort
            true, // outPort
            true, // ipFrag
            true, // dscp
            true, // dstMac
            true, // ipType
            true // ttl
        },
        kSwitchID());
  }

  AclEntrySaiId createAclEntry(AclTableSaiId aclTableId) const {
    SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
    SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute{1};
    SaiAclEntryTraits::Attributes::FieldDscp aclFieldDscpAttribute{
        AclEntryFieldU8(kDscp())};
    SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta
        aclFieldRouteDstUserMetaAttribute{
            AclEntryFieldU32(kRouteDstUserMeta())};

    return aclApi->create<SaiAclEntryTraits>(
        {aclTableIdAttribute,
         aclPriorityAttribute,
         aclFieldDscpAttribute,
         aclFieldRouteDstUserMetaAttribute},
        kSwitchID());
  }

  void checkAclTable(AclTableSaiId aclTableId) const {
    EXPECT_EQ(aclTableId, fs->aclTableManager.get(aclTableId).id);
  }

  void checkAclEntry(AclTableSaiId aclTableId, AclEntrySaiId aclEntryId) const {
    EXPECT_EQ(aclEntryId, fs->aclTableManager.getMember(aclEntryId).id);
    EXPECT_EQ(aclTableId, fs->aclTableManager.getMember(aclEntryId).tableId);
  }

  AclTableGroupSaiId createAclTableGroup(
      const std::vector<sai_int32_t>& aclTableGroupBindPointTypeListAttribute =
          std::vector<sai_int32_t>{SAI_ACL_BIND_POINT_TYPE_PORT}) const {
    SaiAclTableGroupTraits::Attributes::Stage aclTableGroupStageAttribute{
        SAI_ACL_STAGE_INGRESS};
    SaiAclTableGroupTraits::Attributes::Type aclTableGroupTypeAttribute{
        SAI_ACL_TABLE_GROUP_TYPE_SEQUENTIAL};

    return aclApi->create<SaiAclTableGroupTraits>(
        {aclTableGroupStageAttribute,
         aclTableGroupBindPointTypeListAttribute,
         aclTableGroupTypeAttribute},
        kSwitchID());
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

  auto aclTableStageGot =
      aclApi->getAttribute(aclTableId, SaiAclTableTraits::Attributes::Stage());
  auto aclTableBindPointTypeListGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::BindPointTypeList());
  auto aclTableActionTypeListGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::ActionTypeList());
  auto aclTableEntryListGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::EntryList());
  auto aclTableFieldDscpGot = aclApi->getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldDscp());

  EXPECT_EQ(aclTableStageGot, SAI_ACL_STAGE_INGRESS);
  EXPECT_EQ(aclTableBindPointTypeListGot.size(), 1);
  EXPECT_EQ(aclTableBindPointTypeListGot[0], SAI_ACL_BIND_POINT_TYPE_PORT);
  EXPECT_TRUE(aclTableActionTypeListGot == kActionTypeList());

  EXPECT_EQ(aclTableEntryListGot.size(), 1);
  EXPECT_EQ(aclTableEntryListGot[0], static_cast<uint32_t>(aclEntryId));
  EXPECT_EQ(aclTableFieldDscpGot, true);
}

TEST_F(AclApiTest, getAclEntryAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  auto aclPriorityGot = aclApi->getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::Priority());
  auto aclFieldDscpGot = aclApi->getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::FieldDscp());

  EXPECT_EQ(aclPriorityGot, 1);
  EXPECT_EQ(aclFieldDscpGot.getDataAndMask(), kDscp());
}

TEST_F(AclApiTest, setAclTableAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  auto aclEntryId = createAclEntry(aclTableId);
  checkAclEntry(aclTableId, aclEntryId);

  // SAI spec does not support setting any attribute for ACL table post
  // creation.
  SaiAclTableTraits::Attributes::Stage stage{SAI_ACL_STAGE_EGRESS};
  SaiAclTableTraits::Attributes::BindPointTypeList bindPointTypeList{};
  SaiAclTableTraits::Attributes::ActionTypeList actionTypeList{};
  SaiAclTableTraits::Attributes::EntryList entryList{};
  SaiAclTableTraits::Attributes::FieldDscp fieldDscp{true};

  EXPECT_THROW(aclApi->setAttribute(aclTableId, stage), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableId, bindPointTypeList), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, actionTypeList), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, entryList), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableId, fieldDscp), SaiApiError);
}

TEST_F(AclApiTest, setAclEntryAttribute) {
  auto aclTableId = createAclTable();
  checkAclTable(aclTableId);

  SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute{aclTableId};
  SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute1{1};
  SaiAclEntryTraits::Attributes::FieldDscp aclFieldDscpAttribute{
      AclEntryFieldU8(kDscp())};
  SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta
      aclFieldRouteDstUserMetaAttribute{AclEntryFieldU32(kRouteDstUserMeta())};

  auto aclEntryId = aclApi->create<SaiAclEntryTraits>(
      {aclTableIdAttribute,
       aclPriorityAttribute1,
       aclFieldDscpAttribute,
       aclFieldRouteDstUserMetaAttribute},
      kSwitchID());
  checkAclEntry(aclTableId, aclEntryId);

  // SAI spec does not support setting tableId for ACL entry post
  // creation.
  SaiAclEntryTraits::Attributes::TableId aclTableIdAttribute2{2};
  EXPECT_THROW(
      aclApi->setAttribute(aclEntryId, aclTableIdAttribute2), SaiApiError);

  // SAI spec supports setting priority and fieldDscp for ACL entry post
  // creation.
  SaiAclEntryTraits::Attributes::Priority aclPriorityAttribute2{2};
  SaiAclEntryTraits::Attributes::FieldDscp aclFieldDscpAttribute2{
      AclEntryFieldU8(kDscp2())};

  aclApi->setAttribute(aclEntryId, aclPriorityAttribute2);
  aclApi->setAttribute(aclEntryId, aclFieldDscpAttribute2);

  auto aclPriorityGot = aclApi->getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::Priority());
  auto aclFieldDscpGot = aclApi->getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::FieldDscp());

  EXPECT_EQ(aclPriorityAttribute2, aclPriorityGot);
  EXPECT_EQ(
      aclFieldDscpAttribute2.value().getDataAndMask(),
      aclFieldDscpGot.getDataAndMask());
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

  auto aclTableGroupStageGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::Stage());
  auto aclTableGroupBindPointTypeListGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::BindPointTypeList());
  auto aclTableGroupTypeGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::Type());
  auto aclTableGroupMembersGot = aclApi->getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::MemberList());

  EXPECT_EQ(aclTableGroupStageGot, SAI_ACL_STAGE_INGRESS);
  EXPECT_EQ(aclTableGroupBindPointTypeListGot.size(), 1);
  EXPECT_EQ(aclTableGroupBindPointTypeListGot[0], SAI_ACL_BIND_POINT_TYPE_PORT);
  EXPECT_EQ(aclTableGroupTypeGot, SAI_ACL_TABLE_GROUP_TYPE_SEQUENTIAL);
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
  auto aclTableIdGot = aclApi->getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::TableId());
  auto aclPriorityGot = aclApi->getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::Priority());

  EXPECT_EQ(aclTableGroupIdGot, aclTableGroupId);
  EXPECT_EQ(aclTableIdGot, aclTableId);
  EXPECT_EQ(aclPriorityGot, 1);
}

TEST_F(AclApiTest, setAclTableGroupAttribute) {
  auto aclTableGroupId = createAclTableGroup();
  checkAclTableGroup(aclTableGroupId);

  // SAI spec does not support setting any attribute for ACL table group post
  // creation.
  SaiAclTableGroupTraits::Attributes::Stage stage{SAI_ACL_STAGE_EGRESS};
  SaiAclTableGroupTraits::Attributes::BindPointTypeList bindPointTypeList{};
  SaiAclTableGroupTraits::Attributes::Type type{
      SAI_ACL_TABLE_GROUP_TYPE_PARALLEL};
  SaiAclTableGroupTraits::Attributes::MemberList memberList{};

  EXPECT_THROW(aclApi->setAttribute(aclTableGroupId, stage), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupId, bindPointTypeList), SaiApiError);
  EXPECT_THROW(aclApi->setAttribute(aclTableGroupId, type), SaiApiError);
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
  SaiAclTableGroupMemberTraits::Attributes::TableId tableId{1};
  SaiAclTableGroupMemberTraits::Attributes::Priority priority{1};

  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupMemberId, tableGroupId), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupMemberId, tableId), SaiApiError);
  EXPECT_THROW(
      aclApi->setAttribute(aclTableGroupMemberId, priority), SaiApiError);
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
