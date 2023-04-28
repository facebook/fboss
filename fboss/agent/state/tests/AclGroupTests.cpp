/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTable.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

DECLARE_bool(enable_acl_table_group);

const std::string kDscp1 = "dscp1";
const std::string kDscp2 = "dscp2";
const std::string kDscp3 = "dscp3";
const std::string kDscp4 = "dscp4";
const uint8_t kDscpVal1 = 1;
const uint8_t kDscpVal2 = 2;
const uint8_t kDscpVal3 = 3;
const uint8_t kDscpVal4 = 4;

const std::string kTable1 = "table1";
const std::string kTable2 = "table2";
const std::string kTable3 = "table3";
const std::string kAclTable1 = "AclTable1";

const cfg::AclStage kAclStage1 = cfg::AclStage::INGRESS;
const cfg::AclStage kAclStage2 = cfg::AclStage::INGRESS_MACSEC;

const std::string kGroup1 = "group1";
const std::string kGroup2 = "group2";
const std::string kAclTableGroupName = "ingress-ACL-Table-Group";

const std::string kAcl1a = "acl1a";
const std::string kAcl1b = "acl1b";
const std::string kAcl2a = "acl2a";
const std::string kAcl2b = "acl2b";
const std::string kAcl3a = "acl3a";

const std::vector<cfg::AclTableActionType> kActionTypes = {
    cfg::AclTableActionType::PACKET_ACTION,
    cfg::AclTableActionType::COUNTER,
    cfg::AclTableActionType::SET_TC};

const std::vector<cfg::AclTableQualifier> kQualifiers = {
    cfg::AclTableQualifier::SRC_IPV6,
    cfg::AclTableQualifier::DST_IPV6,
    cfg::AclTableQualifier::SRC_IPV4,
    cfg::AclTableQualifier::DST_IPV4};

namespace {

std::shared_ptr<const AclMap> getAclMapFromState(
    std::shared_ptr<SwitchState> state) {
  auto aclMap = state->getAclsForTable(kAclStage1, kAclTable1);
  EXPECT_NE(nullptr, aclMap);
  return aclMap;
}

std::shared_ptr<SwitchState> thriftMultiAclSerializeDeserialize(
    const SwitchState& state,
    bool enableMultiAcl) {
  FLAGS_enable_acl_table_group = enableMultiAcl;
  auto thrifty = state.toThrift();
  FLAGS_enable_acl_table_group = !FLAGS_enable_acl_table_group;
  auto thriftIntrStateBack = SwitchState::fromThrift(thrifty);
  FLAGS_enable_acl_table_group = !FLAGS_enable_acl_table_group;
  return thriftIntrStateBack;
}

void verifyMultiAclSerialization(
    const SwitchState& state,
    bool enableMultiAcl) {
  FLAGS_enable_acl_table_group = enableMultiAcl;
  auto thriftIntrStateBack =
      thriftMultiAclSerializeDeserialize(state, enableMultiAcl);
  FLAGS_enable_acl_table_group = !FLAGS_enable_acl_table_group;
  auto thriftyIntr = thriftIntrStateBack->toThrift();
  FLAGS_enable_acl_table_group = !FLAGS_enable_acl_table_group;
  auto thriftStateBack = SwitchState::fromThrift(thriftyIntr);
  if (!enableMultiAcl) {
    EXPECT_EQ(
        state.getAcls()->toThrift(), thriftStateBack->getAcls()->toThrift());
  } else {
    EXPECT_EQ(
        state.cref<switch_state_tags::aclTableGroupMaps>()->toThrift(),
        thriftStateBack->cref<switch_state_tags::aclTableGroupMaps>()
            ->toThrift());
  }
}

void verifyAclHelper(
    std::shared_ptr<const AclMap> map,
    std::shared_ptr<AclEntry> entry1,
    std::shared_ptr<AclEntry> entry2,
    std::shared_ptr<AclEntry> entry3 = nullptr) {
  EXPECT_EQ(map->getEntryIf(entry1->getID())->getID(), kDscp1);
  EXPECT_EQ(map->getEntryIf(entry2->getID())->getID(), kDscp2);
  EXPECT_EQ(map->getEntryIf(entry1->getID())->getDscp(), kDscpVal1);
  EXPECT_EQ(map->getEntryIf(entry2->getID())->getDscp(), kDscpVal2);

  if (entry3 != nullptr) {
    EXPECT_EQ(map->getEntryIf(entry3->getID())->getID(), kDscp3);
    EXPECT_EQ(map->getEntryIf(entry3->getID())->getDscp(), kDscpVal3);
  }
}
} // namespace

TEST(AclGroup, TestEquality) {
  // test AclEntry equality
  auto entry1 = std::make_shared<AclEntry>(1, kDscp1);
  auto entry2 = std::make_shared<AclEntry>(2, kDscp2);
  auto entry1a = std::make_shared<AclEntry>(1, kDscp1);
  auto entry2a = std::make_shared<AclEntry>(2, kDscp2);

  EXPECT_EQ(*entry1, *entry1a);
  EXPECT_EQ(*entry2, *entry2a);
  EXPECT_NE(*entry1, *entry2);

  // test AclMap equality
  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1);
  map1->addEntry(entry2);

  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry1a);
  map2->addEntry(entry2a);

  auto map3 = std::make_shared<AclMap>();
  map3->addEntry(entry1);

  EXPECT_EQ(*map1, *map2);
  EXPECT_NE(*map1, *map3);

  map1->removeEntry(entry1);
  EXPECT_NE(*map1, *map2);
  map2->removeEntry(entry1a);
  EXPECT_EQ(*map1, *map2);
  map1->addEntry(entry1);
  map2->addEntry(entry1a);

  // test AclTable equality
  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);
  table1->setActionTypes(kActionTypes);
  table1->setQualifiers(kQualifiers);
  auto table2 = std::make_shared<AclTable>(2, kTable1);
  table2->setAclMap(map2);
  table2->setActionTypes(kActionTypes);
  table2->setQualifiers(kQualifiers);
  validateNodeSerialization(*table1);
  validateNodeSerialization(*table2);

  EXPECT_NE(*table1, *table2);
  table2->setPriority(1);
  EXPECT_EQ(*table1, *table2);

  // test AclTableMap equality
  auto table3 = std::make_shared<AclTable>(3, kTable2);
  table3->setAclMap(map3);
  validateNodeSerialization(*table3);

  auto tableMap1 = std::make_shared<AclTableMap>();
  tableMap1->addTable(table1);
  tableMap1->addTable(table3);
  validateThriftMapMapSerialization(*tableMap1);

  auto tableMap2 = std::make_shared<AclTableMap>();
  tableMap2->addTable(table2);
  tableMap2->addTable(table3);
  validateThriftMapMapSerialization(*tableMap2);

  table2->setPriority(2);
  EXPECT_NE(*tableMap1, *tableMap2);
  table2->setPriority(1);
  tableMap2->removeTable(table3);
  EXPECT_NE(*tableMap1, *tableMap2);
  tableMap2->addTable(table3);
  EXPECT_EQ(*tableMap1, *tableMap2);

  // test AclTableGroup equality
  auto tableGroup1 = std::make_shared<AclTableGroup>(kAclStage1);
  tableGroup1->setAclTableMap(tableMap1);
  tableGroup1->setName(kGroup1);
  auto tableGroup2 = std::make_shared<AclTableGroup>(kAclStage1);
  tableGroup2->setAclTableMap(tableMap1);
  tableGroup2->setName(kGroup1);
  auto tableGroup3 = std::make_shared<AclTableGroup>(kAclStage2);
  tableGroup3->setAclTableMap(tableMap1);
  tableGroup2->setName(kGroup1);
  validateNodeSerialization(*tableGroup1);
  validateNodeSerialization(*tableGroup2);
  validateNodeSerialization(*tableGroup3);

  EXPECT_EQ(*tableGroup1, *tableGroup2);
  EXPECT_NE(*tableGroup1, *tableGroup3);
}

TEST(AclGroup, SerializeAclMap) {
  auto entry1 = std::make_shared<AclEntry>(1, kDscp1);
  auto entry2 = std::make_shared<AclEntry>(2, kDscp2);
  entry1->setDscp(kDscpVal1);
  entry2->setDscp(kDscpVal2);

  auto map = std::make_shared<AclMap>();
  map->addEntry(entry1);
  map->addEntry(entry2);

  auto serialized = map->toThrift();
  auto mapBack = std::make_shared<AclMap>(serialized);

  EXPECT_EQ(*map, *mapBack);
  verifyAclHelper(mapBack, entry1, entry2);
  EXPECT_EQ(mapBack->getEntryIf(entry1->getID())->getID(), kDscp1);
  EXPECT_EQ(mapBack->getEntryIf(entry2->getID())->getID(), kDscp2);
  EXPECT_EQ(mapBack->getEntryIf(entry1->getID())->getDscp(), kDscpVal1);
  EXPECT_EQ(mapBack->getEntryIf(entry2->getID())->getDscp(), kDscpVal2);

  auto state = SwitchState();
  state.resetAcls(map);
  verifyMultiAclSerialization(state, false);

  auto thriftConvertedState = thriftMultiAclSerializeDeserialize(state, false);
  auto thriftConvertedMap = getAclMapFromState(thriftConvertedState);
  verifyAclHelper(thriftConvertedMap, entry1, entry2);

  // remove an entry
  map->removeEntry(entry1);
  EXPECT_FALSE(map->getEntryIf(entry1->getID()));

  serialized = map->toThrift();
  mapBack = std::make_shared<AclMap>(serialized);

  EXPECT_EQ(*map, *mapBack);
  EXPECT_FALSE(mapBack->getEntryIf(entry1->getID()));
  EXPECT_TRUE(mapBack->getEntryIf(entry2->getID()));
  EXPECT_EQ(mapBack->getEntryIf(entry2->getID())->getDscp(), kDscpVal2);
}

TEST(AclGroup, SerializeAclTable) {
  auto entry1 = std::make_shared<AclEntry>(1, kDscp1);
  auto entry2 = std::make_shared<AclEntry>(2, kDscp2);
  entry1->setDscp(kDscpVal1);
  entry2->setDscp(kDscpVal2);

  auto map = std::make_shared<AclMap>();
  map->addEntry(entry1);
  map->addEntry(entry2);

  auto table = std::make_shared<AclTable>(1, kTable1);
  table->setAclMap(map);
  table->setActionTypes(kActionTypes);
  table->setQualifiers(kQualifiers);
  validateNodeSerialization(*table);

  auto serialized = table->toThrift();
  auto tableBack = std::make_shared<AclTable>(serialized);

  EXPECT_EQ(*table, *tableBack);
  EXPECT_EQ(tableBack->getPriority(), 1);
  EXPECT_EQ(tableBack->getID(), kTable1);
  EXPECT_EQ(*(tableBack->getAclMap()), *map);
  EXPECT_EQ(tableBack->getActionTypes(), kActionTypes);
  EXPECT_EQ(tableBack->getQualifiers(), kQualifiers);

  // change the priority
  table->setPriority(2);
  EXPECT_EQ(table->getPriority(), 2);

  serialized = table->toThrift();
  tableBack = std::make_shared<AclTable>(serialized);

  EXPECT_EQ(*table, *tableBack);
  EXPECT_EQ(tableBack->getPriority(), 2);
  EXPECT_EQ(tableBack->getID(), kTable1);
  EXPECT_EQ(*(tableBack->getAclMap()), *map);
  EXPECT_EQ(tableBack->getActionTypes(), kActionTypes);
  EXPECT_EQ(tableBack->getQualifiers(), kQualifiers);
}

TEST(AclGroup, SerializeAclTableMap) {
  auto entry1 = std::make_shared<AclEntry>(1, kDscp1);
  auto entry2 = std::make_shared<AclEntry>(2, kDscp2);
  auto entry3 = std::make_shared<AclEntry>(3, kDscp3);
  entry1->setDscp(kDscpVal1);
  entry2->setDscp(kDscpVal2);
  entry3->setDscp(kDscpVal3);

  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1);
  map1->addEntry(entry2);
  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry3);

  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);
  auto table2 = std::make_shared<AclTable>(2, kTable2);
  table2->setAclMap(map2);
  validateNodeSerialization(*table1);
  validateNodeSerialization(*table2);

  auto tableMap = std::make_shared<AclTableMap>();
  tableMap->addTable(table1);
  tableMap->addTable(table2);
  validateThriftMapMapSerialization(*tableMap);

  auto serialized = tableMap->toThrift();
  auto tableMapBack = std::make_shared<AclTableMap>(serialized);

  EXPECT_EQ(*tableMap, *tableMapBack);
  EXPECT_EQ(tableMapBack->getTableIf(kTable1)->getID(), kTable1);
  EXPECT_EQ(tableMapBack->getTableIf(kTable2)->getID(), kTable2);

  // add a table
  auto entry4 = std::make_shared<AclEntry>(4, kDscp4);
  entry4->setDscp(kDscpVal4);
  auto map3 = std::make_shared<AclMap>();
  map3->addEntry(entry4);
  auto table3 = std::make_shared<AclTable>(3, kTable3);
  table3->setAclMap(map3);
  tableMap->addTable(table3);

  serialized = tableMap->toThrift();
  tableMapBack = std::make_shared<AclTableMap>(serialized);

  EXPECT_EQ(*tableMap, *tableMapBack);
  EXPECT_EQ(tableMapBack->getTableIf(kTable1)->getID(), kTable1);
  EXPECT_EQ(tableMapBack->getTableIf(kTable2)->getID(), kTable2);
  EXPECT_EQ(tableMapBack->getTableIf(kTable3)->getID(), kTable3);
}

TEST(AclGroup, SerializeAclTableGroup) {
  auto entry1 = std::make_shared<AclEntry>(1, kDscp1);
  auto entry2 = std::make_shared<AclEntry>(2, kDscp2);
  auto entry3 = std::make_shared<AclEntry>(3, kDscp3);
  entry1->setDscp(kDscpVal1);
  entry2->setDscp(kDscpVal2);
  entry3->setDscp(kDscpVal3);

  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1);
  map1->addEntry(entry2);
  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry3);

  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);
  auto table2 = std::make_shared<AclTable>(2, kTable2);
  table2->setAclMap(map2);
  validateThriftStructNodeSerialization(*table1);
  validateThriftStructNodeSerialization(*table2);

  auto tableMap = std::make_shared<AclTableMap>();
  tableMap->addTable(table1);
  tableMap->addTable(table2);
  validateThriftMapMapSerialization(*tableMap);

  auto tableGroup = std::make_shared<AclTableGroup>(kAclStage1);
  tableGroup->setAclTableMap(tableMap);
  tableGroup->setName(kGroup1);
  validateThriftStructNodeSerialization(*tableGroup);

  auto serialized = tableGroup->toThrift();
  auto tableGroupBack = std::make_shared<AclTableGroup>(serialized);

  EXPECT_EQ(*tableGroup, *tableGroupBack);
  EXPECT_EQ(tableGroupBack->getID(), kAclStage1);
  EXPECT_NE(tableGroupBack->getAclTableMap(), nullptr);
  EXPECT_EQ(tableGroupBack->getName(), kGroup1);
  EXPECT_EQ(*(tableGroupBack->getAclTableMap()), *tableMap);
}

TEST(AclGroup, SerializeMultiSwitchAclTableGroupMap) {
  /*
   * Simulate conditions similar to the default Acl Table Group
   * created in switch state and verify the non multi ACL to multi
   * ACL warmboot transition goes through
   */
  auto entry1 = std::make_shared<AclEntry>(1, kDscp1);
  auto entry2 = std::make_shared<AclEntry>(2, kDscp2);
  auto entry3 = std::make_shared<AclEntry>(3, kDscp3);
  entry1->setDscp(kDscpVal1);
  entry2->setDscp(kDscpVal2);
  entry3->setDscp(kDscpVal3);

  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1);
  map1->addEntry(entry2);
  map1->addEntry(entry3);

  auto table1 = std::make_shared<AclTable>(0, kAclTable1);
  table1->setAclMap(map1);

  auto tableMap = std::make_shared<AclTableMap>();
  tableMap->addTable(table1);

  auto tableGroup = std::make_shared<AclTableGroup>(kAclStage1);
  tableGroup->setAclTableMap(tableMap);
  tableGroup->setName(kAclTableGroupName);

  auto tableGroups = std::make_shared<AclTableGroupMap>();
  tableGroups->addAclTableGroup(tableGroup);

  auto state = SwitchState();
  state.resetAclTableGroups(tableGroups);
  verifyMultiAclSerialization(state, true);

  auto thriftConvertedState = thriftMultiAclSerializeDeserialize(state, true);
  auto thriftConvertedMap = thriftConvertedState->getAcls();
  verifyAclHelper(thriftConvertedMap, entry1, entry2, entry3);
}

TEST(AclGroup, ApplyConfigColdbootMultipleAclTable) {
  FLAGS_enable_acl_table_group = true;
  int priority1 = AclTable::kDataplaneAclMaxPriority;
  int priority2 = AclTable::kDataplaneAclMaxPriority;

  auto platform = createMockPlatform();
  auto stateEmpty = make_shared<SwitchState>();

  // Config contains single acl table
  auto entry1a = make_shared<AclEntry>(priority1++, kAcl1a);
  entry1a->setActionType(cfg::AclActionType::DENY);
  entry1a->setEnabled(true);
  auto entry1b = make_shared<AclEntry>(priority1++, kAcl1b);
  entry1b->setAclAction(MatchAction());
  entry1b->setEnabled(true);
  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1a);
  map1->addEntry(entry1b);

  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);
  table1->setActionTypes(kActionTypes);
  table1->setQualifiers(kQualifiers);
  validateNodeSerialization(*table1);

  auto tableMap = make_shared<AclTableMap>();
  tableMap->addTable(table1);
  auto tableGroup = std::make_shared<AclTableGroup>(kAclStage1);
  tableGroup->setAclTableMap(tableMap);
  tableGroup->setName(kGroup1);
  validateNodeSerialization(*tableGroup);

  cfg::AclTable cfgTable1;
  cfgTable1.name_ref() = kTable1;
  cfgTable1.priority_ref() = 1;
  cfgTable1.aclEntries_ref()->resize(2);
  cfgTable1.aclEntries_ref()[0].name_ref() = kAcl1a;
  cfgTable1.aclEntries_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  cfgTable1.aclEntries_ref()[1].name_ref() = kAcl1b;

  cfgTable1.actionTypes_ref()->resize(kActionTypes.size());
  cfgTable1.actionTypes_ref() = kActionTypes;

  cfgTable1.qualifiers_ref()->resize(kQualifiers.size());
  cfgTable1.qualifiers_ref() = kQualifiers;

  cfg::SwitchConfig config;
  cfg::AclTableGroup cfgTableGroup;
  config.aclTableGroup_ref() = cfgTableGroup;
  config.aclTableGroup_ref()->stage_ref() = kAclStage1;
  config.aclTableGroup_ref()->name_ref() = kGroup1;
  config.aclTableGroup_ref()->aclTables_ref()->resize(1);
  config.aclTableGroup_ref()->aclTables_ref()[0] = cfgTable1;
  // Make sure acl1b used so that it isn't ignored
  config.dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->resize(
      1, cfg::MatchToAction());
  *config.dataPlaneTrafficPolicy_ref()->matchToAction_ref()[0].matcher_ref() =
      kAcl1b;

  auto stateV1 = publishAndApplyConfig(stateEmpty, &config, platform.get());

  EXPECT_NE(nullptr, stateV1);
  EXPECT_TRUE(stateV1->getAclTableGroups()
                  ->getAclTableGroup(kAclStage1)
                  ->getAclTableMap()
                  ->getTableIf(table1->getID()));
  EXPECT_EQ(
      *(stateV1->getAclTableGroups()
            ->getAclTableGroup(kAclStage1)
            ->getAclTableMap()
            ->getTableIf(table1->getID())),
      *table1);
  EXPECT_EQ(
      stateV1->getAclTableGroups()->getAclTableGroup(kAclStage1)->getName(),
      kGroup1);
  EXPECT_EQ(
      *(stateV1->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup);
  EXPECT_EQ(
      stateV1->getAclTableGroups()
          ->getAclTableGroup(kAclStage1)
          ->getAclTableMap()
          ->getTableIf(table1->getID())
          ->getActionTypes(),
      kActionTypes);
  EXPECT_EQ(
      stateV1->getAclTableGroups()
          ->getAclTableGroup(kAclStage1)
          ->getAclTableMap()
          ->getTableIf(table1->getID())
          ->getQualifiers(),
      kQualifiers);

  // Config contains 2 acl tables
  auto entry2a = make_shared<AclEntry>(priority2, kAcl2a);
  entry2a->setActionType(cfg::AclActionType::DENY);
  entry2a->setEnabled(true);
  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry2a);
  auto table2 = std::make_shared<AclTable>(2, kTable2);
  table2->setAclMap(map2);
  auto newTableMap = tableGroup->getAclTableMap()->clone();
  newTableMap->addTable(table2);
  tableGroup->setAclTableMap(newTableMap);
  validateNodeSerialization(*table2);

  cfg::AclTable cfgTable2;
  cfgTable2.name_ref() = kTable2;
  cfgTable2.priority_ref() = 2;
  cfgTable2.aclEntries_ref()->resize(1);
  cfgTable2.aclEntries_ref()[0].name_ref() = kAcl2a;
  cfgTable2.aclEntries_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  config.aclTableGroup_ref()->aclTables_ref()->resize(2);
  config.aclTableGroup_ref()->aclTables_ref()[1] = cfgTable2;

  auto stateV2 = publishAndApplyConfig(stateEmpty, &config, platform.get());

  EXPECT_NE(nullptr, stateV2);
  EXPECT_TRUE(stateV2->getAclTableGroups()
                  ->getAclTableGroup(kAclStage1)
                  ->getAclTableMap()
                  ->getTableIf(table1->getID()));
  EXPECT_EQ(
      *(stateV2->getAclTableGroups()
            ->getAclTableGroup(kAclStage1)
            ->getAclTableMap()
            ->getTableIf(table1->getID())),
      *table1);
  EXPECT_TRUE(stateV2->getAclTableGroups()
                  ->getAclTableGroup(kAclStage1)
                  ->getAclTableMap()
                  ->getTableIf(table2->getID()));
  EXPECT_EQ(
      *(stateV2->getAclTableGroups()
            ->getAclTableGroup(kAclStage1)
            ->getAclTableMap()
            ->getTableIf(table2->getID())),
      *table2);
  EXPECT_EQ(
      *(stateV2->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup);
}

TEST(AclGroup, ApplyConfigWarmbootMultipleAclTable) {
  FLAGS_enable_acl_table_group = true;
  int priority1 = AclTable::kDataplaneAclMaxPriority;
  int priority2 = AclTable::kDataplaneAclMaxPriority;
  auto platform = createMockPlatform();

  // State unchanged
  auto entry1a = make_shared<AclEntry>(priority1++, kAcl1a);
  entry1a->setActionType(cfg::AclActionType::DENY);
  entry1a->setEnabled(true);
  auto entry1b = make_shared<AclEntry>(priority1++, kAcl1b);
  entry1b->setActionType(cfg::AclActionType::DENY);
  entry1b->setEnabled(true);
  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1a);
  map1->addEntry(entry1b);
  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);
  validateNodeSerialization(*table1);

  auto entry2a = make_shared<AclEntry>(priority2++, kAcl2a);
  entry2a->setActionType(cfg::AclActionType::DENY);
  entry2a->setEnabled(true);
  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry2a);
  auto table2 = std::make_shared<AclTable>(2, kTable2);
  table2->setAclMap(map2);
  validateNodeSerialization(*table2);

  auto tableMap = make_shared<AclTableMap>();
  tableMap->addTable(table1);
  tableMap->addTable(table2);
  auto tableGroup = make_shared<AclTableGroup>(kAclStage1);
  tableGroup->setAclTableMap(tableMap);
  tableGroup->setName(kGroup1);
  validateNodeSerialization(*tableGroup);

  auto tableGroups = make_shared<AclTableGroupMap>();
  tableGroups->addAclTableGroup(tableGroup);
  validateThriftMapMapSerialization(*tableGroups);

  cfg::AclTable cfgTable1;
  cfgTable1.name_ref() = kTable1;
  cfgTable1.priority_ref() = 1;
  cfgTable1.aclEntries_ref()->resize(2);
  cfgTable1.aclEntries_ref()[0].name_ref() = kAcl1a;
  cfgTable1.aclEntries_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  cfgTable1.aclEntries_ref()[1].name_ref() = kAcl1b;
  cfgTable1.aclEntries_ref()[1].actionType_ref() = cfg::AclActionType::DENY;
  cfg::AclTable cfgTable2;
  cfgTable2.name_ref() = kTable2;
  cfgTable2.priority_ref() = 2;
  cfgTable2.aclEntries_ref()->resize(1);
  cfgTable2.aclEntries_ref()[0].name_ref() = kAcl2a;
  cfgTable2.aclEntries_ref()[0].actionType_ref() = cfg::AclActionType::DENY;

  cfg::SwitchConfig config;
  cfg::AclTableGroup cfgTableGroup;
  config.aclTableGroup_ref() = cfgTableGroup;
  config.aclTableGroup_ref()->name_ref() = kGroup1;
  config.aclTableGroup_ref()->stage_ref() = kAclStage1;
  config.aclTableGroup_ref()->aclTables_ref()->resize(2);
  config.aclTableGroup_ref()->aclTables_ref()[0] = cfgTable1;
  config.aclTableGroup_ref()->aclTables_ref()[1] = cfgTable2;

  auto stateV0 = make_shared<SwitchState>();
  addSwitchInfo(stateV0);
  stateV0->resetAclTableGroups(tableGroups);

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_EQ(nullptr, stateV1);

  // Add a table
  int priority3 = AclTable::kDataplaneAclMaxPriority;
  cfg::AclTable cfgTable3;
  cfgTable3.name_ref() = kTable3;
  cfgTable3.priority_ref() = 3;
  cfgTable3.aclEntries_ref()->resize(1);
  cfgTable3.aclEntries_ref()[0].name_ref() = kAcl3a;
  cfgTable3.aclEntries_ref()[0].actionType_ref() = cfg::AclActionType::DENY;

  config.aclTableGroup_ref()->aclTables_ref()->resize(3);
  config.aclTableGroup_ref()->aclTables_ref()[2] = cfgTable3;

  auto stateV2 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  EXPECT_NE(
      *(stateV2->getAclTableGroups())->getAclTableGroup(kAclStage1),
      *tableGroup);

  auto entry3a = make_shared<AclEntry>(priority3++, kAcl3a);
  entry3a->setActionType(cfg::AclActionType::DENY);
  entry3a->setEnabled(true);
  auto map3 = std::make_shared<AclMap>();
  map3->addEntry(entry3a);
  auto table3 = std::make_shared<AclTable>(3, kTable3);
  table3->setAclMap(map3);
  auto tableMap1 = tableGroup->getAclTableMap()->clone();
  tableMap1->addTable(table3);
  /*
   * Directly getting tableMap and adding a new node will reflect only
   * in switchstate and not in thrift structure. That will cause tests to
   * fail. So clone a newtablegroup and do a setAclTableMap everytime its
   * updated so that the thrift structure is also updated
   */
  auto tableGroup1 = tableGroup->clone();
  tableGroup1->setAclTableMap(tableMap1);
  validateNodeSerialization(*table3);

  EXPECT_EQ(
      *(stateV2->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  // Remove a table
  config.aclTableGroup_ref()->aclTables_ref()->resize(2);

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  EXPECT_NE(nullptr, stateV3);
  EXPECT_NE(
      *(stateV3->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  auto tableMap2 = tableGroup1->getAclTableMap()->clone();
  tableMap2->removeTable(table3->getID());
  tableGroup1->setAclTableMap(tableMap2);

  EXPECT_EQ(
      *(stateV3->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  // Change the priority of a table
  config.aclTableGroup_ref()->aclTables_ref()[1].priority_ref() = 5;

  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  EXPECT_NE(nullptr, stateV4);
  EXPECT_NE(
      *(stateV4->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  auto tableMap3 = tableGroup1->getAclTableMap()->clone();
  tableMap3->updateNode(tableMap3->getTable(table2->getID())->clone());
  tableMap3->getTable(table2->getID())->setPriority(5);
  tableGroup1->setAclTableMap(tableMap3);

  EXPECT_EQ(
      *(stateV4->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  // Add an entry to a table
  config.aclTableGroup_ref()->aclTables_ref()[1].aclEntries_ref()->resize(2);
  config.aclTableGroup_ref()
      ->aclTables_ref()[1]
      .aclEntries_ref()[1]
      .name_ref() = kAcl2b;
  config.aclTableGroup_ref()
      ->aclTables_ref()[1]
      .aclEntries_ref()[1]
      .actionType_ref() = cfg::AclActionType::DENY;

  auto stateV5 = publishAndApplyConfig(stateV4, &config, platform.get());
  EXPECT_NE(nullptr, stateV5);
  EXPECT_NE(
      *(stateV5->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  auto entry2b = make_shared<AclEntry>(priority2++, kAcl2b);
  entry2b->setActionType(cfg::AclActionType::DENY);
  entry2b->setEnabled(true);
  auto map2Version2 = table2->getAclMap()->clone();
  map2Version2->addEntry(entry2b);
  /*
   * Directly getting AclMap and adding a new node will reflect only
   * in switchstate and not in thrift structure. That will cause tests to
   * fail. So do a setAclMap everytime its updated so that the thrift
   * structure is also updated
   */
  table2 = table2->clone();
  table2->setAclMap(map2Version2);
  auto tableMap4 = tableGroup1->getAclTableMap()->clone();
  tableGroup1 = tableGroup1->clone();
  tableGroup1->setAclTableMap(tableMap4);
  auto groups = stateV5->getAclTableGroups()->clone();
  groups->updateNode(tableGroup1);
  stateV5 = stateV5->clone();
  stateV5->resetAclTableGroups(groups);

  EXPECT_EQ(
      *(stateV5->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  // Remove an entry from a table
  config.aclTableGroup_ref()->aclTables_ref()[0].aclEntries_ref()->resize(1);

  auto stateV6 = publishAndApplyConfig(stateV5, &config, platform.get());
  EXPECT_NE(nullptr, stateV6);
  EXPECT_NE(
      *(stateV6->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  auto map1Version2 = table1->getAclMap()->clone();
  map1Version2->removeEntry(entry1b);
  table1 = table1->clone();
  table1->setAclMap(map1Version2);
  auto tableMap5 = tableGroup1->getAclTableMap()->clone();
  tableGroup1 = tableGroup1->clone();
  tableGroup1->setAclTableMap(tableMap5);
  groups = stateV6->getAclTableGroups()->clone();
  groups->updateNode(tableGroup1);
  stateV6 = stateV6->clone();
  stateV6->resetAclTableGroups(groups);

  EXPECT_EQ(
      *(stateV6->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  // Change an entry in a table
  auto proto = 6;
  config.aclTableGroup_ref()
      ->aclTables_ref()[1]
      .aclEntries_ref()[0]
      .proto_ref() = proto;

  auto stateV7 = publishAndApplyConfig(stateV6, &config, platform.get());
  EXPECT_NE(nullptr, stateV7);
  EXPECT_NE(
      *(stateV7->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  auto map2Version3 = table2->getAclMap()->clone();
  auto entry2a2 = entry2a->clone();
  entry2a2->setProto(proto);
  map2Version3->updateNode(entry2a2);
  table2 = table2->clone();
  table2->setAclMap(map2Version3);
  auto tableMap6 = tableGroup1->getAclTableMap()->clone();
  tableGroup1 = tableGroup1->clone();
  tableGroup1->setAclTableMap(tableMap6);
  groups = stateV7->getAclTableGroups()->clone();
  groups->updateNode(tableGroup1);
  stateV7 = stateV7->clone();
  stateV7->resetAclTableGroups(groups);
  EXPECT_EQ(
      *(stateV7->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  // Move an entry between tables
  config.aclTableGroup_ref()->aclTables_ref()[1].aclEntries_ref()->resize(
      1); // delete entry2b from table 2
  config.aclTableGroup_ref()->aclTables_ref()[0].aclEntries_ref()->resize(2);
  config.aclTableGroup_ref()
      ->aclTables_ref()[0]
      .aclEntries_ref()[1]
      .name_ref() = kAcl2b; // add entry2b to table 1
  config.aclTableGroup_ref()
      ->aclTables_ref()[0]
      .aclEntries_ref()[1]
      .actionType_ref() = cfg::AclActionType::DENY;

  auto stateV8 = publishAndApplyConfig(stateV7, &config, platform.get());
  EXPECT_NE(nullptr, stateV8);
  EXPECT_NE(
      *(stateV8->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);

  auto map2Version4 = table2->getAclMap()->clone();
  map2Version4->removeEntry(entry2b->getID());
  table2->setAclMap(map2Version4);
  auto map1Version3 = table1->getAclMap()->clone();
  map1Version3->addEntry(entry2b);
  table1->setAclMap(map1Version3);
  auto tableMap7 = tableGroup1->getAclTableMap()->clone();
  tableGroup1 = tableGroup1->clone();
  tableGroup1->setAclTableMap(
      tableMap7); // 2b will be the second entry in table1, so priority
                  // unchanged (originally second entry in table2)
  groups = stateV8->getAclTableGroups()->clone();
  groups->updateNode(tableGroup1);
  stateV8 = stateV8->clone();
  stateV8->resetAclTableGroups(groups);

  EXPECT_EQ(
      *(stateV8->getAclTableGroups()->getAclTableGroup(kAclStage1)),
      *tableGroup1);
}
