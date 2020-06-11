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
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

class AclTableManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();

    // In our implementation, a table is always member of group
    // for corresponding SAI_ACL_STAGE_*.
    saiManagerTable->aclTableGroupManager().addAclTableGroup(
        SAI_ACL_STAGE_INGRESS);
  }
};

TEST_F(AclTableManagerTest, addAclTable) {
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();
  // Acl table is added ass part of sai switch init in test setup
  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableManagerTest, addTwoAclTable) {
  AclTableSaiId aclTableId =
      saiManagerTable->aclTableManager().addAclTable("AclTable1");

  AclTableSaiId aclTableId2 =
      saiManagerTable->aclTableManager().addAclTable("AclTable2");

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);

  auto stageGot2 = saiApiTable->aclApi().getAttribute(
      aclTableId2, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot2, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableManagerTest, addDupAclTable) {
  saiManagerTable->aclTableManager().addAclTable("AclTable1");
  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclTable("AclTable1"), FbossError);
}

TEST_F(AclTableManagerTest, getAclTable) {
  saiManagerTable->aclTableManager().addAclTable("AclTable1");
  auto handle =
      saiManagerTable->aclTableManager().getAclTableHandle("AclTable1");

  EXPECT_TRUE(handle);
  EXPECT_TRUE(handle->aclTable);
}

TEST_F(AclTableManagerTest, checkNonExistentAclTable) {
  saiManagerTable->aclTableManager().addAclTable("AclTable1");
  auto handle =
      saiManagerTable->aclTableManager().getAclTableHandle("AclTable2");

  EXPECT_FALSE(handle);
}
