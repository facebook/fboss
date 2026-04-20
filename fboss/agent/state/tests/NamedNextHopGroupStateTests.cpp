/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/SwitchStateNextHopIdUpdater.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NextHopIdMaps.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

namespace {

HwSwitchMatcher createMatcher(int64_t switchId) {
  return HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(switchId)});
}

std::shared_ptr<FibInfo> createFibInfo() {
  auto fibInfo = std::make_shared<FibInfo>();
  auto fibsMap = std::make_shared<ForwardingInformationBaseMap>();
  auto fibContainer =
      std::make_shared<ForwardingInformationBaseContainer>(RouterID(0));
  fibsMap->updateForwardingInformationBaseContainer(fibContainer);
  fibInfo->resetFibsMap(fibsMap);
  return fibInfo;
}

} // namespace

class NamedNextHopGroupStateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    state_ = std::make_shared<SwitchState>();
  }

  std::shared_ptr<SwitchState> state_;
};

TEST_F(NamedNextHopGroupStateTest, SetAndGetNameToNextHopSetId) {
  auto fibInfo = createFibInfo();

  // Initially should return empty map
  auto emptyMap = fibInfo->getNameToNextHopSetId();
  EXPECT_TRUE(emptyMap.empty());

  // Set some name-to-ID mappings
  std::map<std::string, NextHopSetId> nameToSetId;
  constexpr NextHopSetId setId1 = (1ULL << 62) + 100;
  constexpr NextHopSetId setId2 = (1ULL << 62) + 200;
  nameToSetId["group1"] = setId1;
  nameToSetId["group2"] = setId2;
  fibInfo->setNameToNextHopSetId(nameToSetId);

  // Verify getNameToNextHopSetId returns the correct mappings
  auto retrieved = fibInfo->getNameToNextHopSetId();
  EXPECT_EQ(retrieved.size(), 2);
  EXPECT_EQ(retrieved["group1"], setId1);
  EXPECT_EQ(retrieved["group2"], setId2);
}

TEST_F(NamedNextHopGroupStateTest, GetNextHopSetIdIf) {
  auto fibInfo = createFibInfo();

  // Returns nullopt when no named groups exist
  EXPECT_EQ(fibInfo->getNextHopSetIdIf("nonexistent"), std::nullopt);

  // Set a named group
  constexpr NextHopSetId setId = (1ULL << 62) + 42;
  std::map<std::string, NextHopSetId> nameToSetId;
  nameToSetId["mygroup"] = setId;
  fibInfo->setNameToNextHopSetId(nameToSetId);

  // Should find existing group
  auto result = fibInfo->getNextHopSetIdIf("mygroup");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, setId);

  // Should return nullopt for non-existent group
  EXPECT_EQ(fibInfo->getNextHopSetIdIf("missing"), std::nullopt);
}

TEST_F(NamedNextHopGroupStateTest, GetNextHopSetIdThrowsOnMissing) {
  auto fibInfo = createFibInfo();
  EXPECT_THROW(fibInfo->getNextHopSetId("nonexistent"), FbossError);
}

TEST_F(NamedNextHopGroupStateTest, SetAndRemoveNextHopSetForName) {
  auto fibInfo = createFibInfo();

  constexpr NextHopSetId setId1 = (1ULL << 62) + 1;
  constexpr NextHopSetId setId2 = (1ULL << 62) + 2;

  // Add named groups one at a time
  fibInfo->setNextHopSetIdForName("group1", setId1);
  fibInfo->setNextHopSetIdForName("group2", setId2);

  auto result = fibInfo->getNameToNextHopSetId();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result["group1"], setId1);
  EXPECT_EQ(result["group2"], setId2);

  // Remove one group
  fibInfo->removeNextHopSetForName("group1");
  result = fibInfo->getNameToNextHopSetId();
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result.count("group1"), 0);
  EXPECT_EQ(result["group2"], setId2);
}

TEST_F(NamedNextHopGroupStateTest, ResolveNextHopSetFromName) {
  auto fibInfo = createFibInfo();

  // Set up the ID maps
  NextHopThrift nh1, nh2;
  nh1.address() = network::toBinaryAddress(folly::IPAddress("10.0.0.1"));
  nh1.address()->ifName() = "fboss1";
  *nh1.weight() = UCMP_DEFAULT_WEIGHT;
  nh2.address() = network::toBinaryAddress(folly::IPAddress("10.0.0.2"));
  nh2.address()->ifName() = "fboss2";
  *nh2.weight() = UCMP_DEFAULT_WEIGHT;

  NextHopId nhId1 = 1, nhId2 = 2;
  auto idToNextHopMap = std::make_shared<IdToNextHopMap>();
  idToNextHopMap->addNextHop(nhId1, nh1);
  idToNextHopMap->addNextHop(nhId2, nh2);
  fibInfo->setIdToNextHopMap(idToNextHopMap);

  constexpr NextHopSetId setId = (1ULL << 62) + 1;
  auto idToNextHopIdSetMap = std::make_shared<IdToNextHopIdSetMap>();
  idToNextHopIdSetMap->addNextHopIdSet(setId, {nhId1, nhId2});
  fibInfo->setIdToNextHopIdSetMap(idToNextHopIdSetMap);

  // Set the name-to-ID mapping
  fibInfo->setNextHopSetIdForName("mygroup", setId);

  // Resolve by name
  auto resolved = fibInfo->resolveNextHopSetFromName("mygroup");
  EXPECT_EQ(resolved.size(), 2);

  // Throws for unknown name
  EXPECT_THROW(fibInfo->resolveNextHopSetFromName("unknown"), FbossError);
}

TEST_F(
    NamedNextHopGroupStateTest,
    SwitchStateNextHopIdUpdaterSyncsNamedGroups) {
  // Set up a NextHopIDManager with named groups
  NextHopIDManager manager;
  RouteNextHopSet nhops1;
  nhops1.emplace(
      ResolvedNextHop(folly::IPAddress("10.0.0.1"), InterfaceID(1), 1));
  nhops1.emplace(
      ResolvedNextHop(folly::IPAddress("10.0.0.2"), InterfaceID(2), 1));

  RouteNextHopSet nhops2;
  nhops2.emplace(
      ResolvedNextHop(folly::IPAddress("10.0.0.3"), InterfaceID(3), 1));

  manager.allocateNamedNextHopGroup("ecmp-group", nhops1);
  manager.allocateNamedNextHopGroup("single-nh-group", nhops2);

  // Create switch state with a FibInfo
  auto fibInfo = createFibInfo();
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto matcher = createMatcher(0);
  fibInfoMap->addNode(matcher.matcherString(), fibInfo);
  state_->resetFibsInfoMap(fibInfoMap);

  // Run the updater
  SwitchStateNextHopIdUpdater updater(&manager);
  auto newState = updater(state_);

  // Verify named group mappings were synced to FibInfo
  auto updatedFibInfo =
      newState->getFibsInfoMap()->getNodeIf(matcher.matcherString());
  ASSERT_NE(updatedFibInfo, nullptr);

  auto nameToSetId = updatedFibInfo->getNameToNextHopSetId();
  EXPECT_EQ(nameToSetId.size(), 2);
  EXPECT_NE(nameToSetId.count("ecmp-group"), 0);
  EXPECT_NE(nameToSetId.count("single-nh-group"), 0);

  // Verify the set IDs match what the manager assigned
  auto expectedSetId1 = manager.getNextHopSetIDForName("ecmp-group");
  auto expectedSetId2 = manager.getNextHopSetIDForName("single-nh-group");
  ASSERT_TRUE(expectedSetId1.has_value());
  ASSERT_TRUE(expectedSetId2.has_value());
  EXPECT_EQ(
      nameToSetId["ecmp-group"], static_cast<NextHopSetId>(*expectedSetId1));
  EXPECT_EQ(
      nameToSetId["single-nh-group"],
      static_cast<NextHopSetId>(*expectedSetId2));
}

TEST_F(NamedNextHopGroupStateTest, SwitchStateNextHopIdUpdaterNoNamedGroups) {
  // Manager with no named groups
  NextHopIDManager manager;

  auto fibInfo = createFibInfo();
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto matcher = createMatcher(0);
  fibInfoMap->addNode(matcher.matcherString(), fibInfo);
  state_->resetFibsInfoMap(fibInfoMap);

  SwitchStateNextHopIdUpdater updater(&manager);
  auto newState = updater(state_);

  auto updatedFibInfo =
      newState->getFibsInfoMap()->getNodeIf(matcher.matcherString());
  ASSERT_NE(updatedFibInfo, nullptr);

  // Should have empty name-to-ID map
  auto nameToSetId = updatedFibInfo->getNameToNextHopSetId();
  EXPECT_TRUE(nameToSetId.empty());
}

TEST_F(NamedNextHopGroupStateTest, SerializationPreservesNamedGroups) {
  auto fibInfo = createFibInfo();

  // Set named group mappings
  constexpr NextHopSetId setId1 = (1ULL << 62) + 10;
  constexpr NextHopSetId setId2 = (1ULL << 62) + 20;
  std::map<std::string, NextHopSetId> nameToSetId;
  nameToSetId["group-a"] = setId1;
  nameToSetId["group-b"] = setId2;
  fibInfo->setNameToNextHopSetId(nameToSetId);

  // Serialize and deserialize
  auto serialized = fibInfo->toThrift();
  auto deserialized = std::make_shared<FibInfo>(serialized);

  // Verify preserved
  auto result = deserialized->getNameToNextHopSetId();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result["group-a"], setId1);
  EXPECT_EQ(result["group-b"], setId2);
}

} // namespace facebook::fboss
