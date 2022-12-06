/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

namespace {
const std::string kAclTable2 = "AclTable2";
}

class AclTableGroupManagerTest : public ManagerTestBase {
  // INGRESS Acl table group and a single Acl table are
  // created on init, nothing to special to do in
  // setting up these test cases.
};

TEST_F(AclTableGroupManagerTest, addAclTableGroup) {
  // Acl table group ingress should already be added
  auto aclTableGroupId = saiManagerTable->aclTableGroupManager()
                             .getAclTableGroupHandle(SAI_ACL_STAGE_INGRESS)
                             ->aclTableGroup->adapterKey();

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableGroupId, SaiAclTableGroupTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableGroupManagerTest, addEgressMacSecAclTableGroup) {
  auto aclTableGroup2 =
      std::make_shared<AclTableGroup>(cfg::AclStage::EGRESS_MACSEC);
  AclTableGroupSaiId aclTableGroupId2 =
      saiManagerTable->aclTableGroupManager().addAclTableGroup(aclTableGroup2);

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableGroupId2, SaiAclTableGroupTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_EGRESS_MACSEC);
}

TEST_F(AclTableGroupManagerTest, addDupAclTableGroup) {
  auto aclTableGroup = std::make_shared<AclTableGroup>(cfg::AclStage::INGRESS);
  EXPECT_THROW(
      saiManagerTable->aclTableGroupManager().addAclTableGroup(aclTableGroup),
      FbossError);
}

TEST_F(AclTableGroupManagerTest, getAclTableGroup) {
  auto handle = saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
      SAI_ACL_STAGE_INGRESS);

  EXPECT_TRUE(handle);
  EXPECT_TRUE(handle->aclTableGroup);
}

TEST_F(AclTableGroupManagerTest, checkNonExistentAclTableGroup) {
  auto handle = saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
      SAI_ACL_STAGE_EGRESS);

  EXPECT_FALSE(handle);
}

TEST_F(AclTableGroupManagerTest, addAclTableGroupMember) {
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(kAclTable1)
                        ->aclTable->adapterKey();
  auto aclTableGroupHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
          SAI_ACL_STAGE_INGRESS);
  EXPECT_TRUE(aclTableGroupHandle);
  EXPECT_TRUE(aclTableGroupHandle->aclTableGroup);

  auto aclTableGroupMemberHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupMemberHandle(
          aclTableGroupHandle, kAclTable1);
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
  // When an ACL table is created, it is implicitly added to a group.
  auto table2 = std::make_shared<AclTable>(0, kAclTable2);
  AclTableSaiId aclTableId2 = saiManagerTable->aclTableManager().addAclTable(
      table2, cfg::AclStage::INGRESS);

  auto aclTableGroupHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
          SAI_ACL_STAGE_INGRESS);
  EXPECT_TRUE(aclTableGroupHandle);
  EXPECT_TRUE(aclTableGroupHandle->aclTableGroup);

  auto aclTableGroupMemberHandle2 =
      saiManagerTable->aclTableGroupManager().getAclTableGroupMemberHandle(
          aclTableGroupHandle, kAclTable2);
  EXPECT_TRUE(aclTableGroupMemberHandle2);
  EXPECT_TRUE(aclTableGroupMemberHandle2->aclTableGroupMember);

  AclTableGroupMemberSaiId aclTableGroupMemberId2 =
      aclTableGroupMemberHandle2->aclTableGroupMember->adapterKey();
  auto tableIdGot2 = saiApiTable->aclApi().getAttribute(
      aclTableGroupMemberId2,
      SaiAclTableGroupMemberTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot2, aclTableId2);
}

TEST_F(AclTableGroupManagerTest, addDupAclTableGroupMember) {
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(kAclTable1)
                        ->aclTable->adapterKey();
  EXPECT_THROW(
      saiManagerTable->aclTableGroupManager().addAclTableGroupMember(
          SAI_ACL_STAGE_INGRESS, aclTableId, kAclTable1),
      FbossError);
}

TEST_F(AclTableGroupManagerTest, checkNonExistentAclTableGroupMember) {
  auto aclTableGroupHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupHandle(
          SAI_ACL_STAGE_INGRESS);
  EXPECT_TRUE(aclTableGroupHandle);
  EXPECT_TRUE(aclTableGroupHandle->aclTableGroup);

  auto aclTableGroupMemberHandle =
      saiManagerTable->aclTableGroupManager().getAclTableGroupMemberHandle(
          aclTableGroupHandle, kAclTable2);
  EXPECT_FALSE(aclTableGroupMemberHandle);
}
