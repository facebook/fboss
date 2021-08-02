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

const int kAclStartPriority = 100000;

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

const std::string kGroup1 = "group1";
const std::string kGroup2 = "group2";

const std::string kAcl1a = "acl1a";
const std::string kAcl1b = "acl1b";
const std::string kAcl2a = "acl2a";
const std::string kAcl2b = "acl2b";
const std::string kAcl3a = "acl3a";

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
  auto table2 = std::make_shared<AclTable>(2, kTable1);
  table2->setAclMap(map2);

  EXPECT_NE(*table1, *table2);
  table2->setPriority(1);
  EXPECT_EQ(*table1, *table2);

  // test AclTableMap equality
  auto table3 = std::make_shared<AclTable>(3, kTable2);
  table3->setAclMap(map3);

  auto tableMap1 = std::make_shared<AclTableMap>();
  tableMap1->addTable(table1);
  tableMap1->addTable(table3);

  auto tableMap2 = std::make_shared<AclTableMap>();
  tableMap2->addTable(table2);
  tableMap2->addTable(table3);

  table2->setPriority(2);
  EXPECT_NE(*tableMap1, *tableMap2);
  table2->setPriority(1);
  tableMap2->removeTable(table3);
  EXPECT_NE(*tableMap1, *tableMap2);
  tableMap2->addTable(table3);
  EXPECT_EQ(*tableMap1, *tableMap2);

  // test AclTableGroup equality
  auto tableGroup1 = std::make_shared<AclTableGroup>(kGroup1);
  tableGroup1->setAclTableMap(tableMap1);
  auto tableGroup2 = std::make_shared<AclTableGroup>(kGroup1);
  tableGroup2->setAclTableMap(tableMap2);
  auto tableGroup3 = std::make_shared<AclTableGroup>(kGroup2);
  tableGroup3->setAclTableMap(tableMap1);

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

  auto serialized = map->toFollyDynamic();
  auto mapBack = AclMap::fromFollyDynamic(serialized);

  EXPECT_EQ(*map, *mapBack);
  EXPECT_EQ(mapBack->getEntryIf(entry1->getID())->getID(), kDscp1);
  EXPECT_EQ(mapBack->getEntryIf(entry2->getID())->getID(), kDscp2);
  EXPECT_EQ(mapBack->getEntryIf(entry1->getID())->getDscp(), kDscpVal1);
  EXPECT_EQ(mapBack->getEntryIf(entry2->getID())->getDscp(), kDscpVal2);

  // remove an entry
  map->removeEntry(entry1);
  EXPECT_FALSE(map->getEntryIf(entry1->getID()));

  serialized = map->toFollyDynamic();
  mapBack = AclMap::fromFollyDynamic(serialized);

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

  auto serialized = table->toFollyDynamic();
  auto tableBack = AclTable::fromFollyDynamic(serialized);

  EXPECT_EQ(*table, *tableBack);
  EXPECT_EQ(tableBack->getPriority(), 1);
  EXPECT_EQ(tableBack->getID(), kTable1);
  EXPECT_EQ(*(tableBack->getAclMap()), *map);

  // change the priority
  table->setPriority(2);
  EXPECT_EQ(table->getPriority(), 2);

  serialized = table->toFollyDynamic();
  tableBack = AclTable::fromFollyDynamic(serialized);

  EXPECT_EQ(*table, *tableBack);
  EXPECT_EQ(tableBack->getPriority(), 2);
  EXPECT_EQ(tableBack->getID(), kTable1);
  EXPECT_EQ(*(tableBack->getAclMap()), *map);
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

  auto tableMap = std::make_shared<AclTableMap>();
  tableMap->addTable(table1);
  tableMap->addTable(table2);

  auto serialized = tableMap->toFollyDynamic();
  auto tableMapBack = AclTableMap::fromFollyDynamic(serialized);

  EXPECT_EQ(*tableMap, *tableMapBack);
  EXPECT_EQ(tableMapBack->getTableIf(kTable1)->getID(), kTable1);
  EXPECT_EQ(tableMapBack->getTableIf(kTable2)->getID(), kTable2);

  // add a table
  auto entry4 = std::make_shared<AclEntry>(4, kDscp4);
  entry4->setDscp(kDscpVal4);
  auto map3 = std::make_shared<AclMap>();
  map3->addEntry(entry4);
  auto table3 = std::make_shared<AclTable>(3, "table3");
  table3->setAclMap(map3);
  tableMap->addTable(table3);

  serialized = tableMap->toFollyDynamic();
  tableMapBack = AclTableMap::fromFollyDynamic(serialized);

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

  auto tableMap = std::make_shared<AclTableMap>();
  tableMap->addTable(table1);
  tableMap->addTable(table2);

  auto tableGroup = std::make_shared<AclTableGroup>(kGroup1);
  tableGroup->setAclTableMap(tableMap);

  auto serialized = tableGroup->toFollyDynamic();
  auto tableGroupBack = AclTableGroup::fromFollyDynamic(serialized);

  EXPECT_EQ(*tableGroup, *tableGroupBack);
  EXPECT_EQ(tableGroupBack->getID(), kGroup1);
  EXPECT_NE(tableGroupBack->getAclTableMap(), nullptr);
  EXPECT_EQ(*(tableGroupBack->getAclTableMap()), *tableMap);
}

TEST(AclGroup, ApplyConfigColdbootMultipleAclTable) {
  FLAGS_enable_acl_table_group = true;
  int priority1 = kAclStartPriority;
  int priority2 = kAclStartPriority;

  auto platform = createMockPlatform();
  auto stateEmpty = make_shared<SwitchState>();

  // Config contains single acl table
  auto entry1a = make_shared<AclEntry>(priority1++, kAcl1a);
  entry1a->setActionType(cfg::AclActionType::DENY);
  auto entry1b = make_shared<AclEntry>(priority1++, kAcl1b);
  entry1b->setAclAction(MatchAction());
  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1a);
  map1->addEntry(entry1b);
  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);
  auto tableMap = make_shared<AclTableMap>();
  tableMap->addTable(table1);
  auto tableGroup = make_shared<AclTableGroup>(kGroup1);
  tableGroup->setAclTableMap(tableMap);

  cfg::AclTable cfgTable1;
  cfgTable1.name_ref() = kTable1;
  cfgTable1.priority_ref() = 1;
  cfgTable1.aclEntries_ref()->resize(2);
  cfgTable1.aclEntries_ref()[0].name_ref() = kAcl1a;
  cfgTable1.aclEntries_ref()[0].actionType_ref() = cfg::AclActionType::DENY;
  cfgTable1.aclEntries_ref()[1].name_ref() = kAcl1b;

  cfg::SwitchConfig config;
  cfg::AclTableGroup cfgTableGroup;
  config.aclTableGroup_ref() = cfgTableGroup;
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
  EXPECT_TRUE(stateV1->getAclTableGroup()->getAclTableMap()->getTableIf(
      table1->getID()));
  EXPECT_EQ(
      *(stateV1->getAclTableGroup()->getAclTableMap()->getTableIf(
          table1->getID())),
      *table1);
  EXPECT_EQ(stateV1->getAclTableGroup()->getID(), kGroup1);
  EXPECT_EQ(*(stateV1->getAclTableGroup()), *tableGroup);

  // Config contains 2 acl tables
  auto entry2a = make_shared<AclEntry>(priority2, kAcl2a);
  entry2a->setActionType(cfg::AclActionType::DENY);
  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry2a);
  auto table2 = std::make_shared<AclTable>(2, kTable2);
  table2->setAclMap(map2);
  tableGroup->getAclTableMap()->addTable(table2);

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
  EXPECT_TRUE(stateV2->getAclTableGroup()->getAclTableMap()->getTableIf(
      table1->getID()));
  EXPECT_EQ(
      *(stateV2->getAclTableGroup()->getAclTableMap()->getTableIf(
          table1->getID())),
      *table1);
  EXPECT_TRUE(stateV2->getAclTableGroup()->getAclTableMap()->getTableIf(
      table2->getID()));
  EXPECT_EQ(
      *(stateV2->getAclTableGroup()->getAclTableMap()->getTableIf(
          table2->getID())),
      *table2);
  EXPECT_EQ(*(stateV2->getAclTableGroup()), *tableGroup);
}

TEST(AclGroup, ApplyConfigWarmbootMultipleAclTable) {
  FLAGS_enable_acl_table_group = true;
  int priority1 = kAclStartPriority;
  int priority2 = kAclStartPriority;
  auto platform = createMockPlatform();

  // State unchanged
  auto entry1a = make_shared<AclEntry>(priority1++, kAcl1a);
  entry1a->setActionType(cfg::AclActionType::DENY);
  auto entry1b = make_shared<AclEntry>(priority1++, kAcl1b);
  entry1b->setActionType(cfg::AclActionType::DENY);
  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1a);
  map1->addEntry(entry1b);
  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);

  auto entry2a = make_shared<AclEntry>(priority2++, kAcl2a);
  entry2a->setActionType(cfg::AclActionType::DENY);
  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry2a);
  auto table2 = std::make_shared<AclTable>(2, kTable2);
  table2->setAclMap(map2);

  auto tableMap = make_shared<AclTableMap>();
  tableMap->addTable(table1);
  tableMap->addTable(table2);
  auto tableGroup = make_shared<AclTableGroup>(kGroup1);
  tableGroup->setAclTableMap(tableMap);

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
  config.aclTableGroup_ref()->aclTables_ref()->resize(2);
  config.aclTableGroup_ref()->aclTables_ref()[0] = cfgTable1;
  config.aclTableGroup_ref()->aclTables_ref()[1] = cfgTable2;

  auto stateV0 = make_shared<SwitchState>();
  stateV0->resetAclTableGroup(tableGroup);

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_EQ(nullptr, stateV1);

  // Add a table
  int priority3 = kAclStartPriority;
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
  EXPECT_NE(*(stateV2->getAclTableGroup()), *tableGroup);

  auto entry3a = make_shared<AclEntry>(priority3++, kAcl3a);
  entry3a->setActionType(cfg::AclActionType::DENY);
  auto map3 = std::make_shared<AclMap>();
  map3->addEntry(entry3a);
  auto table3 = std::make_shared<AclTable>(3, kTable3);
  table3->setAclMap(map3);
  tableGroup->getAclTableMap()->addTable(table3);

  EXPECT_EQ(*(stateV2->getAclTableGroup()), *tableGroup);

  // Remove a table
  config.aclTableGroup_ref()->aclTables_ref()->resize(2);

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  EXPECT_NE(nullptr, stateV3);
  EXPECT_NE(*(stateV3->getAclTableGroup()), *tableGroup);

  tableGroup->getAclTableMap()->removeTable(table3->getID());

  EXPECT_EQ(*(stateV3->getAclTableGroup()), *tableGroup);

  // Change the priority of a table
  config.aclTableGroup_ref()->aclTables_ref()[1].priority_ref() = 5;

  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  EXPECT_NE(nullptr, stateV4);
  EXPECT_NE(*(stateV4->getAclTableGroup()), *tableGroup);

  tableGroup->getAclTableMap()->getTable(table2->getID())->setPriority(5);

  EXPECT_EQ(*(stateV4->getAclTableGroup()), *tableGroup);

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
  EXPECT_NE(*(stateV5->getAclTableGroup()), *tableGroup);

  auto entry2b = make_shared<AclEntry>(priority2++, kAcl2b);
  entry2b->setActionType(cfg::AclActionType::DENY);
  tableGroup->getAclTableMap()
      ->getTable(table2->getID())
      ->getAclMap()
      ->addEntry(entry2b);

  EXPECT_EQ(*(stateV5->getAclTableGroup()), *tableGroup);

  // Remove an entry from a table
  config.aclTableGroup_ref()->aclTables_ref()[0].aclEntries_ref()->resize(1);

  auto stateV6 = publishAndApplyConfig(stateV5, &config, platform.get());
  EXPECT_NE(nullptr, stateV6);
  EXPECT_NE(*(stateV6->getAclTableGroup()), *tableGroup);

  tableGroup->getAclTableMap()
      ->getTable(table1->getID())
      ->getAclMap()
      ->removeEntry(entry1b->getID());

  EXPECT_EQ(*(stateV6->getAclTableGroup()), *tableGroup);

  // Change an entry in a table
  auto proto = 6;
  config.aclTableGroup_ref()
      ->aclTables_ref()[1]
      .aclEntries_ref()[0]
      .proto_ref() = proto;

  auto stateV7 = publishAndApplyConfig(stateV6, &config, platform.get());
  EXPECT_NE(nullptr, stateV7);
  EXPECT_NE(*(stateV7->getAclTableGroup()), *tableGroup);

  tableGroup->getAclTableMap()
      ->getTable(table2->getID())
      ->getAclMap()
      ->getEntryIf(entry2a->getID())
      ->setProto(proto);

  EXPECT_EQ(*(stateV7->getAclTableGroup()), *tableGroup);

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
  EXPECT_NE(*(stateV8->getAclTableGroup()), *tableGroup);

  tableGroup->getAclTableMap()
      ->getTable(table2->getID())
      ->getAclMap()
      ->removeEntry(entry2b->getID());
  tableGroup->getAclTableMap()
      ->getTable(table1->getID())
      ->getAclMap()
      ->addEntry(entry2b); // 2b will be the second entry in table1, so priority
                           // unchanged (originally second entry in table2)

  EXPECT_EQ(*(stateV8->getAclTableGroup()), *tableGroup);
}
