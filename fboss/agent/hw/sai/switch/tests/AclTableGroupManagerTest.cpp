/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

class AclTableGroupManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();
  }
};

TEST_F(AclTableGroupManagerTest, addAclTableGroup) {
  AclTableGroupSaiId aclTableGroupId =
      saiManagerTable->aclTableGroupManager().addAclTableGroup(
          SAI_ACL_STAGE_INGRESS);

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableGroupManagerTest, addTwoAclTableGroup) {
  AclTableGroupSaiId aclTableGroupId =
      saiManagerTable->aclTableGroupManager().addAclTableGroup(
          SAI_ACL_STAGE_INGRESS);

  AclTableGroupSaiId aclTableGroupId2 =
      saiManagerTable->aclTableGroupManager().addAclTableGroup(
          SAI_ACL_STAGE_EGRESS);

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);

  auto stageGot2 = saiApiTable->aclApi().getAttribute(
      aclTableGroupId2, SaiAclTableGroupTraits::Attributes::Stage());
  EXPECT_EQ(stageGot2, SAI_ACL_STAGE_EGRESS);
}

TEST_F(AclTableGroupManagerTest, addDupAclTableGroup) {
  saiManagerTable->aclTableGroupManager().addAclTableGroup(
      SAI_ACL_STAGE_INGRESS);

  EXPECT_THROW(
      saiManagerTable->aclTableGroupManager().addAclTableGroup(
          SAI_ACL_STAGE_INGRESS),
      FbossError);
}

TEST_F(AclTableGroupManagerTest, getAclTableGroup) {
  saiManagerTable->aclTableGroupManager().addAclTableGroup(
      SAI_ACL_STAGE_INGRESS);
  auto handle = saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
      SAI_ACL_STAGE_INGRESS);

  EXPECT_TRUE(handle);
  EXPECT_TRUE(handle->aclTableGroup);
}

TEST_F(AclTableGroupManagerTest, checkNonExistentAclTableGroup) {
  saiManagerTable->aclTableGroupManager().addAclTableGroup(
      SAI_ACL_STAGE_INGRESS);
  auto handle = saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
      SAI_ACL_STAGE_EGRESS);

  EXPECT_FALSE(handle);
}

TEST_F(AclTableGroupManagerTest, addAclTableGroupMember) {
  saiManagerTable->aclTableGroupManager().addAclTableGroup(
      SAI_ACL_STAGE_INGRESS);
  // When an ACL table is created, it is implicitly added to a group.
  AclTableSaiId aclTableId =
      saiManagerTable->aclTableManager().addAclTable("AclTable1");

  auto aclTableGroupHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
          SAI_ACL_STAGE_INGRESS);
  EXPECT_TRUE(aclTableGroupHandle);
  EXPECT_TRUE(aclTableGroupHandle->aclTableGroup);

  auto aclTableGroupMemberHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupMemberHandle(
          aclTableGroupHandle, "AclTable1");
  EXPECT_TRUE(aclTableGroupMemberHandle);
  EXPECT_TRUE(aclTableGroupMemberHandle->aclTableGroupMember);

  AclTableGroupMemberSaiId aclTableGroupMemberId =
      aclTableGroupMemberHandle->aclTableGroupMember->adapterKey();
  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::TableGroupId());
  EXPECT_EQ(tableIdGot, aclTableId);
}

TEST_F(AclTableGroupManagerTest, addTwoAclTableGroupMember) {
  saiManagerTable->aclTableGroupManager().addAclTableGroup(
      SAI_ACL_STAGE_INGRESS);
  // When an ACL table is created, it is implicitly added to a group.
  AclTableSaiId aclTableId =
      saiManagerTable->aclTableManager().addAclTable("AclTable1");

  AclTableSaiId aclTableId2 =
      saiManagerTable->aclTableManager().addAclTable("AclTable2");

  auto aclTableGroupHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
          SAI_ACL_STAGE_INGRESS);
  EXPECT_TRUE(aclTableGroupHandle);
  EXPECT_TRUE(aclTableGroupHandle->aclTableGroup);

  auto aclTableGroupMemberHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupMemberHandle(
          aclTableGroupHandle, "AclTable1");
  EXPECT_TRUE(aclTableGroupMemberHandle);
  EXPECT_TRUE(aclTableGroupMemberHandle->aclTableGroupMember);

  auto aclTableGroupMemberHandle2 =
      saiManagerTable->aclTableGroupManager().getAclTableGroupMemberHandle(
          aclTableGroupHandle, "AclTable2");
  EXPECT_TRUE(aclTableGroupMemberHandle2);
  EXPECT_TRUE(aclTableGroupMemberHandle2->aclTableGroupMember);

  AclTableGroupMemberSaiId aclTableGroupMemberId =
      aclTableGroupMemberHandle->aclTableGroupMember->adapterKey();
  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclTableGroupMemberId,
      SaiAclTableGroupMemberTraits::Attributes::TableGroupId());
  EXPECT_EQ(tableIdGot, aclTableId);

  AclTableGroupMemberSaiId aclTableGroupMemberId2 =
      aclTableGroupMemberHandle2->aclTableGroupMember->adapterKey();
  auto tableIdGot2 = saiApiTable->aclApi().getAttribute(
      aclTableGroupMemberId2,
      SaiAclTableGroupMemberTraits::Attributes::TableGroupId());
  EXPECT_EQ(tableIdGot2, aclTableId2);
}

TEST_F(AclTableGroupManagerTest, addDupAclTableGroupMember) {
  saiManagerTable->aclTableGroupManager().addAclTableGroup(
      SAI_ACL_STAGE_INGRESS);

  // When an ACL table is created, it is implicitly added to a group.
  AclTableSaiId aclTableId =
      saiManagerTable->aclTableManager().addAclTable("AclTable1");

  EXPECT_THROW(
      saiManagerTable->aclTableGroupManager().addAclTableGroupMember(
          SAI_ACL_STAGE_INGRESS, aclTableId, "AclTable1"),
      FbossError);
}

TEST_F(AclTableGroupManagerTest, checkNonExistentAclTableGroupMember) {
  saiManagerTable->aclTableGroupManager().addAclTableGroup(
      SAI_ACL_STAGE_INGRESS);

  auto aclTableGroupHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
          SAI_ACL_STAGE_INGRESS);
  EXPECT_TRUE(aclTableGroupHandle);
  EXPECT_TRUE(aclTableGroupHandle->aclTableGroup);

  auto aclTableGroupMemberHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupMemberHandle(
          aclTableGroupHandle, "AclTable1");
  EXPECT_FALSE(aclTableGroupMemberHandle);
}
