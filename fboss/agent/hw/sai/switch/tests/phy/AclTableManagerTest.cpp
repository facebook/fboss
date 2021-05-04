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
constexpr auto kAclTable2 = "AclTable2";
}

class AclTableManagerTest : public ManagerTestBase {
 public:
  int kPriority() {
    return 1;
  }

  int kPriority2() {
    return 2;
  }

  uint8_t kDscp() {
    return 10;
  }

  uint8_t kDscp2() {
    return 20;
  }

  cfg::AclActionType kActionType() {
    return cfg::AclActionType::DENY;
  }
};

TEST_F(AclTableManagerTest, addAclTable) {
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();
  // Acl table is added as part of sai switch init in test setup
  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);
  // Enabled fields
  EXPECT_TRUE(saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldDstMac{}));
  EXPECT_TRUE(saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldEthertype{}));
  auto bindPoints = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::BindPointTypeList());
  EXPECT_EQ(1, bindPoints.size());
  EXPECT_EQ(SAI_ACL_BIND_POINT_TYPE_PORT, *bindPoints.begin());
  // Check a few disabled fields
  EXPECT_FALSE(saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldSrcIpV6{}));
  EXPECT_FALSE(saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::FieldL4DstPort{}));
}

TEST_F(AclTableManagerTest, addTwoAclTable) {
  // AclTable1 should already be added
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();
  AclTableSaiId aclTableId2 = saiManagerTable->aclTableManager().addAclTable(
      kAclTable2, SAI_ACL_STAGE_INGRESS);

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);

  auto stageGot2 = saiApiTable->aclApi().getAttribute(
      aclTableId2, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot2, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableManagerTest, addDupAclTable) {
  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclTable(
          SaiSwitch::kAclTable1, SAI_ACL_STAGE_INGRESS),
      FbossError);
}

TEST_F(AclTableManagerTest, getAclTable) {
  auto handle = saiManagerTable->aclTableManager().getAclTableHandle(
      SaiSwitch::kAclTable1);

  EXPECT_TRUE(handle);
  EXPECT_TRUE(handle->aclTable);
}

TEST_F(AclTableManagerTest, checkNonExistentAclTable) {
  auto handle =
      saiManagerTable->aclTableManager().getAclTableHandle(kAclTable2);

  EXPECT_FALSE(handle);
}
