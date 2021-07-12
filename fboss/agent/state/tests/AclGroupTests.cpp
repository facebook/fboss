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
