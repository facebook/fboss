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
