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
}

TEST_F(AclTableManagerTest, addTwoAclTable) {
  // AclTable1 should already be added
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();
  AclTableSaiId aclTableId2 =
      saiManagerTable->aclTableManager().addAclTable(kAclTable2);

  auto stageGot = saiApiTable->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot, SAI_ACL_STAGE_INGRESS);

  auto stageGot2 = saiApiTable->aclApi().getAttribute(
      aclTableId2, SaiAclTableTraits::Attributes::Stage());
  EXPECT_EQ(stageGot2, SAI_ACL_STAGE_INGRESS);
}

TEST_F(AclTableManagerTest, addDupAclTable) {
  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclTable(SaiSwitch::kAclTable1),
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

TEST_F(AclTableManagerTest, addAclEntry) {
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();

  auto aclEntry = std::make_shared<AclEntry>(kPriority(), "AclEntry1");
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);

  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot, aclTableId);
}

TEST_F(AclTableManagerTest, addAclEntryWithCounter) {
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();

  auto counter = cfg::TrafficCounter();
  *counter.name_ref() = "stat0.c";
  MatchAction action = MatchAction();
  action.setTrafficCounter(counter);

  auto aclEntry = std::make_shared<AclEntry>(kPriority(), "AclEntry1");
  aclEntry->setDscp(kDscp());
  aclEntry->setAclAction(action);

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);

  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot, aclTableId);

  auto aclCounterIdGot =
      saiApiTable->aclApi()
          .getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::ActionCounter())
          .getData();

  auto tableIdGot2 = saiApiTable->aclApi().getAttribute(
      AclCounterSaiId(aclCounterIdGot),
      SaiAclCounterTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot2, aclTableId);
}

TEST_F(AclTableManagerTest, addTwoAclEntry) {
  auto aclTableId = saiManagerTable->aclTableManager()
                        .getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();

  auto aclEntry = std::make_shared<AclEntry>(kPriority(), "AclEntry1");
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  AclEntrySaiId aclEntryId = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);

  auto tableIdGot = saiApiTable->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot, aclTableId);

  auto aclEntry2 = std::make_shared<AclEntry>(kPriority2(), "AclEntry2");
  aclEntry2->setDscp(kDscp2());
  aclEntry2->setActionType(kActionType());

  AclEntrySaiId aclEntryId2 = saiManagerTable->aclTableManager().addAclEntry(
      aclEntry2, SaiSwitch::kAclTable1);

  auto tableIdGot2 = saiApiTable->aclApi().getAttribute(
      aclEntryId2, SaiAclEntryTraits::Attributes::TableId());
  EXPECT_EQ(tableIdGot2, aclTableId);
}

TEST_F(AclTableManagerTest, addDupAclEntry) {
  auto aclEntry = std::make_shared<AclEntry>(kPriority(), "AclEntry1");
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);

  auto dupAclEntry = std::make_shared<AclEntry>(kPriority(), "AclEntry1");
  dupAclEntry->setDscp(kDscp());
  dupAclEntry->setActionType(cfg::AclActionType::DENY);

  EXPECT_THROW(
      saiManagerTable->aclTableManager().addAclEntry(
          dupAclEntry, SaiSwitch::kAclTable1),
      FbossError);
}

TEST_F(AclTableManagerTest, getAclEntry) {
  auto aclEntry = std::make_shared<AclEntry>(kPriority(), "AclEntry1");
  aclEntry->setDscp(kDscp());
  aclEntry->setActionType(kActionType());

  saiManagerTable->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);

  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      SaiSwitch::kAclTable1);

  EXPECT_TRUE(aclTableHandle);
  EXPECT_TRUE(aclTableHandle->aclTable);

  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());

  EXPECT_TRUE(aclEntryHandle);
  EXPECT_TRUE(aclEntryHandle->aclEntry);
}

TEST_F(AclTableManagerTest, checkNonExistentAclEntry) {
  auto aclTableHandle = saiManagerTable->aclTableManager().getAclTableHandle(
      SaiSwitch::kAclTable1);

  EXPECT_TRUE(aclTableHandle);
  EXPECT_TRUE(aclTableHandle->aclTable);

  auto aclEntryHandle = saiManagerTable->aclTableManager().getAclEntryHandle(
      aclTableHandle, kPriority());
  EXPECT_FALSE(aclEntryHandle);
}
