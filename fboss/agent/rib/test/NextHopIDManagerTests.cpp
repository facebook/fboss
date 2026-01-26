/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

constexpr int64_t kSetIdOffset = 1LL << 62;

class NextHopIDManagerTest : public ::testing::Test {
 public:
  NextHopIDManagerTest() = default;

  void SetUp() override {
    manager_ = std::make_unique<NextHopIDManager>();
  }

 protected:
  std::unique_ptr<NextHopIDManager> manager_;
};

TEST_F(NextHopIDManagerTest, getOrAllocateNextHopID) {
  // Test that IDs are allocated sequentially
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);

  auto iter1 = manager_->getOrAllocateNextHopID(nh1);
  auto id1 = iter1->second.id;
  auto iter2 = manager_->getOrAllocateNextHopID(nh2);
  auto id2 = iter2->second.id;
  auto iter3 = manager_->getOrAllocateNextHopID(nh3);
  auto id3 = iter3->second.id;

  EXPECT_EQ(id1, NextHopID(1));
  EXPECT_EQ(id2, NextHopID(2));
  EXPECT_EQ(id3, NextHopID(3));

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);

  EXPECT_EQ(manager_->getIdToNextHop().at(id1), nh1);
  EXPECT_EQ(manager_->getIdToNextHop().at(id2), nh2);
  EXPECT_EQ(manager_->getIdToNextHop().at(id3), nh3);

  // Test reusing existing ID

  auto iter4 = manager_->getOrAllocateNextHopID(nh1);
  auto id4 = iter4->second.id;
  EXPECT_EQ(id1, id4);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().at(id4), nh1);

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 2);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
}

TEST_F(NextHopIDManagerTest, getOrAllocateNextHopSetID) {
  // Test empty set throws exception
  NextHopIDSet emptySet;
  auto emptySetIter = manager_->getOrAllocateNextHopSetID(emptySet);
  EXPECT_EQ(emptySetIter->second.id, NextHopSetID(kSetIdOffset));
  EXPECT_EQ(emptySetIter->first, emptySet);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);

  // Test that IDs are allocated sequentially starting from 2^63
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);

  auto nhIter1 = manager_->getOrAllocateNextHopID(nh1);
  auto nhID1 = nhIter1->second.id;
  auto nhIter2 = manager_->getOrAllocateNextHopID(nh2);
  auto nhID2 = nhIter2->second.id;
  auto nhIter3 = manager_->getOrAllocateNextHopID(nh3);
  auto nhID3 = nhIter3->second.id;

  NextHopIDSet set1 = {nhID1};
  NextHopIDSet set2 = {nhID1, nhID2};
  NextHopIDSet set3 = {nhID1, nhID2, nhID3};

  auto setIter1 = manager_->getOrAllocateNextHopSetID(set1);
  auto setIter2 = manager_->getOrAllocateNextHopSetID(set2);
  auto setIter3 = manager_->getOrAllocateNextHopSetID(set3);

  EXPECT_EQ(setIter1->second.id, NextHopSetID(kSetIdOffset + 1));
  EXPECT_EQ(setIter2->second.id, NextHopSetID(kSetIdOffset + 2));
  EXPECT_EQ(setIter3->second.id, NextHopSetID(kSetIdOffset + 3));

  EXPECT_EQ(setIter1->first, set1);
  EXPECT_EQ(setIter2->first, set2);
  EXPECT_EQ(setIter3->first, set3);

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set1), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set2), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set3), 1);

  // Reuse existing ID
  auto setIter4 = manager_->getOrAllocateNextHopSetID(set1);
  EXPECT_EQ(setIter1->second.id, setIter4->second.id);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 4);
  EXPECT_EQ(setIter4->first, set1);

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set1), 2);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set2), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set3), 1);
}

TEST_F(NextHopIDManagerTest, getOrAllocateNextHopSetIDOrderIndependence) {
  // Test that sets with same elements but different order get same ID
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);

  auto nhIter1 = manager_->getOrAllocateNextHopID(nh1);
  auto nhID1 = nhIter1->second.id;
  auto nhIter2 = manager_->getOrAllocateNextHopID(nh2);
  auto nhID2 = nhIter2->second.id;
  auto nhIter3 = manager_->getOrAllocateNextHopID(nh3);
  auto nhID3 = nhIter3->second.id;

  NextHopIDSet set1 = {nhID1, nhID2, nhID3};
  NextHopIDSet set2 = {nhID3, nhID1, nhID2};
  NextHopIDSet set3 = {nhID2, nhID3, nhID1};

  auto setIter1 = manager_->getOrAllocateNextHopSetID(set1);
  auto setIter2 = manager_->getOrAllocateNextHopSetID(set2);
  auto setIter3 = manager_->getOrAllocateNextHopSetID(set3);

  EXPECT_EQ(setIter1->second.id, setIter2->second.id);
  EXPECT_EQ(setIter2->second.id, setIter3->second.id);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(setIter1->first, set1);

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set1), 3);
}

TEST_F(NextHopIDManagerTest, decrOrDeallocateNextHop) {
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);

  // Test throws exception when decrementing/deallocating non-existent ID
  EXPECT_THROW(manager_->decrOrDeallocateNextHop(nh1), FbossError);

  // allocate IDs to Nexthops
  manager_->getOrAllocateNextHopID(nh1);
  auto iter2 = manager_->getOrAllocateNextHopID(nh2);
  auto id2 = iter2->second.id;
  manager_->getOrAllocateNextHopID(nh3);
  auto iter4 = manager_->getOrAllocateNextHopID(nh1);
  auto id4 = iter4->second.id;

  // Test decrementing ref count works
  manager_->decrOrDeallocateNextHop(nh1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 1);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().at(id4), nh1);

  // Test deallocation works
  manager_->decrOrDeallocateNextHop(nh2);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 0);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 2);
  EXPECT_EQ(manager_->getIdToNextHop().count(id2), 0);
  EXPECT_EQ(manager_->nextHopToIDInfo_.count(nh2), 0);

  // Test allocating new ID after deallocating continues from last ID
  auto iter5 = manager_->getOrAllocateNextHopID(nh2);
  auto id5 = iter5->second.id;
  EXPECT_EQ(id5, NextHopID(4));
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().at(id5), nh2);
}

TEST_F(NextHopIDManagerTest, decrOrDeallocateNextHopIDSet) {
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);

  auto nhIter1 = manager_->getOrAllocateNextHopID(nh1);
  auto nhID1 = nhIter1->second.id;
  auto nhIter2 = manager_->getOrAllocateNextHopID(nh2);
  auto nhID2 = nhIter2->second.id;
  auto nhIter3 = manager_->getOrAllocateNextHopID(nh3);
  auto nhID3 = nhIter3->second.id;

  NextHopIDSet set1 = {nhID1};
  NextHopIDSet set2 = {nhID1, nhID2};
  NextHopIDSet set3 = {nhID1, nhID2, nhID3};
  NextHopIDSet set4 = {};

  // Test throws exception when decrementing/deallocating non-existent set
  EXPECT_THROW(manager_->decrOrDeallocateNextHopIDSet(set1), FbossError);

  manager_->getOrAllocateNextHopSetID(set1);
  auto setIter2 = manager_->getOrAllocateNextHopSetID(set2);
  auto setID2 = setIter2->second.id;
  manager_->getOrAllocateNextHopSetID(set3);
  auto setIter4 = manager_->getOrAllocateNextHopSetID(set1);
  auto setID4 = setIter4->second.id;
  auto setIter5 = manager_->getOrAllocateNextHopSetID(set4);
  auto setID5 = setIter5->second.id;

  // Test decrementing ref count works
  manager_->decrOrDeallocateNextHopIDSet(set1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set1), 1);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 4);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID4), set1);

  // Test deallocation works
  manager_->decrOrDeallocateNextHopIDSet(set2);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set2), 0);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().count(setID2), 0);
  EXPECT_EQ(manager_->nextHopIdSetToIDInfo_.count(set2), 0);

  // Test deallocating empty set works
  manager_->decrOrDeallocateNextHopIDSet(set4);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set4), 0);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().count(setID5), 0);
  EXPECT_EQ(manager_->nextHopIdSetToIDInfo_.count(set4), 0);

  // Test allocating new ID after deallocating continues from last ID
  auto setIter6 = manager_->getOrAllocateNextHopSetID(set2);
  EXPECT_EQ(setIter6->second.id, NextHopSetID(kSetIdOffset + 4));
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(setIter6->first, set2);
}

TEST_F(NextHopIDManagerTest, getOrAllocRouteNextHopSetIDWithEmptySet) {
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);

  // Add nhSet1 with 2 nexthops (nh1, nh2)
  RouteNextHopSet nhSet1 = {nh1, nh2};

  auto result1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);

  EXPECT_EQ(result1.nextHopIdSetIter->second.id, NextHopSetID(kSetIdOffset));
  // Verify 2 new NextHopIDs were allocated
  EXPECT_EQ(result1.addedNextHopIds.size(), 2);

  // Verify individual NextHop IDs were allocated
  auto nhID1 = manager_->getNextHopID(nh1);
  auto nhID2 = manager_->getNextHopID(nh2);
  ASSERT_TRUE(nhID1.has_value());
  ASSERT_TRUE(nhID2.has_value());
  EXPECT_EQ(nhID1.value(), NextHopID(1));
  EXPECT_EQ(nhID2.value(), NextHopID(2));

  // Verify idToNextHop map
  EXPECT_EQ(manager_->getIdToNextHop().size(), 2);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID1.value()), nh1);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID2.value()), nh2);

  // Verify NextHopIDSet was created
  NextHopIDSet expectedIDSet1 = {nhID1.value(), nhID2.value()};
  auto actualSetID1 = manager_->getNextHopSetID(expectedIDSet1);
  ASSERT_TRUE(actualSetID1.has_value());
  EXPECT_EQ(actualSetID1.value(), result1.nextHopIdSetIter->second.id);

  // Verify idToNextHopIdSet map
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(result1.nextHopIdSetIter->second.id),
      expectedIDSet1);

  // Make NhSet2 with 3 nexthops (nh1, nh2, nh3)
  RouteNextHopSet nhSet2 = {nh1, nh2, nh3};

  auto result2 = manager_->getOrAllocRouteNextHopSetID(nhSet2);

  EXPECT_EQ(
      result2.nextHopIdSetIter->second.id, NextHopSetID(kSetIdOffset + 1));
  // Verify only 1 new NextHopID was allocated (nh3)
  EXPECT_EQ(result2.addedNextHopIds.size(), 1);
  EXPECT_EQ(result2.addedNextHopIds[0], NextHopID(3));

  // Verify NextHop IDs - nh3 should get a new ID
  auto nhID3 = manager_->getNextHopID(nh3);
  ASSERT_TRUE(nhID3.has_value());
  EXPECT_EQ(nhID3.value(), NextHopID(3));

  // Verify idToNextHop map now has 3 entries
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID3.value()), nh3);

  // Verify NextHopIDSet was created for nhSet2
  NextHopIDSet expectedIDSet2 = {nhID1.value(), nhID2.value(), nhID3.value()};
  auto actualSetID2 = manager_->getNextHopSetID(expectedIDSet2);
  ASSERT_TRUE(actualSetID2.has_value());
  EXPECT_EQ(actualSetID2.value(), result2.nextHopIdSetIter->second.id);

  // Verify idToNextHopIdSet map has 2 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(result2.nextHopIdSetIter->second.id),
      expectedIDSet2);

  // Add nhSet3 with empty set {}
  RouteNextHopSet nhSet3;

  // Call getOrAllocRouteNextHopSetID for empty set
  auto result3 = manager_->getOrAllocRouteNextHopSetID(nhSet3);

  EXPECT_EQ(
      result3.nextHopIdSetIter->second.id, NextHopSetID(kSetIdOffset + 2));
  // Verify no new NextHopIDs were allocated
  EXPECT_EQ(result3.addedNextHopIds.size(), 0);

  // Verify no new NextHop IDs were allocated (empty set)
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);

  // Verify NextHopIDSet was created for empty nhSet3
  NextHopIDSet expectedIDSet3 = {};
  auto actualSetID3 = manager_->getNextHopSetID(expectedIDSet3);
  ASSERT_TRUE(actualSetID3.has_value());
  EXPECT_EQ(actualSetID3.value(), result3.nextHopIdSetIter->second.id);

  // Verify idToNextHopIdSet map has 3 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(result3.nextHopIdSetIter->second.id),
      expectedIDSet3);

  // Call getOrAllocRouteNextHopSetID on empty set again to verify ID reuse
  RouteNextHopSet nhSet4;
  auto result4 = manager_->getOrAllocRouteNextHopSetID(nhSet4);

  // Verify the same ID is returned
  EXPECT_EQ(
      result3.nextHopIdSetIter->second.id, result4.nextHopIdSetIter->second.id);
  // Verify no new NextHopIDs were allocated
  EXPECT_EQ(result4.addedNextHopIds.size(), 0);

  // Verify total number of NextHopIDSets remains 3
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
}

TEST_F(
    NextHopIDManagerTest,
    getOrAllocRouteNextHopSetIDSubSetSuperSetNextHops) {
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);
  NextHop nh4 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.4", UCMP_DEFAULT_WEIGHT);

  // Add nhSet1 with 3 nexthops (nh1, nh2, nh3)
  RouteNextHopSet nhSet1 = {nh1, nh2, nh3};

  auto result1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);

  EXPECT_EQ(result1.nextHopIdSetIter->second.id, NextHopSetID(kSetIdOffset));
  // Verify 3 new NextHopIDs were allocated
  EXPECT_EQ(result1.addedNextHopIds.size(), 3);

  // Verify individual NextHop IDs were allocated
  auto nhID1 = manager_->getNextHopID(nh1);
  auto nhID2 = manager_->getNextHopID(nh2);
  auto nhID3 = manager_->getNextHopID(nh3);

  // Verify idToNextHop map
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID1.value()), nh1);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID2.value()), nh2);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID3.value()), nh3);

  // Verify NextHopIDSet was created
  NextHopIDSet expectedIDSet1 = {nhID1.value(), nhID2.value(), nhID3.value()};
  auto actualSetID1 = manager_->getNextHopSetID(expectedIDSet1);
  ASSERT_TRUE(actualSetID1.has_value());
  EXPECT_EQ(actualSetID1.value(), result1.nextHopIdSetIter->second.id);

  // Verify idToNextHopIdSet map
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(result1.nextHopIdSetIter->second.id),
      expectedIDSet1);

  // Make NhSet2 with 2 nexthops (nh1, nh2) - subset of nhSet1
  RouteNextHopSet nhSet2 = {nh1, nh2};

  auto result2 = manager_->getOrAllocRouteNextHopSetID(nhSet2);

  EXPECT_EQ(
      result2.nextHopIdSetIter->second.id, NextHopSetID(kSetIdOffset + 1));
  // Verify no new NextHopIDs were allocated (reusing existing IDs)
  EXPECT_EQ(result2.addedNextHopIds.size(), 0);

  // Verify no new NextHop IDs were allocated (reusing existing IDs)
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);

  // Verify NextHopIDSet was created for nhSet2
  NextHopIDSet expectedIDSet2 = {nhID1.value(), nhID2.value()};
  auto actualSetID2 = manager_->getNextHopSetID(expectedIDSet2);
  ASSERT_TRUE(actualSetID2.has_value());
  EXPECT_EQ(actualSetID2.value(), result2.nextHopIdSetIter->second.id);

  // Verify idToNextHopIdSet map has 2 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(result2.nextHopIdSetIter->second.id),
      expectedIDSet2);

  // Add nhSet3 with 4 nexthops (nh1, nh2, nh3, nh4) - superset of nhSet1
  RouteNextHopSet nhSet3 = {nh1, nh2, nh3, nh4};

  auto result3 = manager_->getOrAllocRouteNextHopSetID(nhSet3);

  EXPECT_EQ(
      result3.nextHopIdSetIter->second.id, NextHopSetID(kSetIdOffset + 2));
  // Verify only 1 new NextHopID was allocated (nh4)
  EXPECT_EQ(result3.addedNextHopIds.size(), 1);

  // Verify NextHop IDs - nh4 should get a new ID
  auto nhID4 = manager_->getNextHopID(nh4);
  ASSERT_TRUE(nhID4.has_value());
  EXPECT_EQ(nhID4.value(), NextHopID(4));
  EXPECT_EQ(result3.addedNextHopIds[0], NextHopID(4));

  // Verify idToNextHop map has 4 entries
  EXPECT_EQ(manager_->getIdToNextHop().size(), 4);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID4.value()), nh4);

  // Verify NextHopIDSet was created for nhSet3
  NextHopIDSet expectedIDSet3 = {
      nhID1.value(), nhID2.value(), nhID3.value(), nhID4.value()};
  auto actualSetID3 = manager_->getNextHopSetID(expectedIDSet3);
  ASSERT_TRUE(actualSetID3.has_value());
  EXPECT_EQ(actualSetID3.value(), result3.nextHopIdSetIter->second.id);

  // Verify idToNextHopIdSet map has 3 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(result3.nextHopIdSetIter->second.id),
      expectedIDSet3);

  // Call getOrAllocRouteNextHopSetID on nhSet4 = nhSet2 again to verify ID
  // reuse
  RouteNextHopSet nhSet4 = {nh1, nh2};
  auto result4 = manager_->getOrAllocRouteNextHopSetID(nhSet4);

  // Verify the same ID is returned
  EXPECT_EQ(
      result2.nextHopIdSetIter->second.id, result4.nextHopIdSetIter->second.id);
  // Verify no new NextHopIDs were allocated
  EXPECT_EQ(result4.addedNextHopIds.size(), 0);

  // Verify total number of NextHopIDSets remains 3
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);

  // Verify total number of NextHops remains 4
  EXPECT_EQ(manager_->getIdToNextHop().size(), 4);
}

TEST_F(NextHopIDManagerTest, delOrDecrRouteNextHopSetID) {
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);
  NextHop nh4 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.4", UCMP_DEFAULT_WEIGHT);

  //  Allocate setID id1 for Nh1, nh2, nh3
  RouteNextHopSet nhSet1 = {nh1, nh2, nh3};
  auto result1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);
  NextHopSetID setID1 = result1.nextHopIdSetIter->second.id;

  // Allocate setID id2 for Nh1, nh2
  RouteNextHopSet nhSet2 = {nh1, nh2};
  auto result2 = manager_->getOrAllocRouteNextHopSetID(nhSet2);
  NextHopSetID setID2 = result2.nextHopIdSetIter->second.id;

  // Allocate setID id3 for Nh2, nh4
  RouteNextHopSet nhSet3 = {nh2, nh4};
  auto result3 = manager_->getOrAllocRouteNextHopSetID(nhSet3);
  NextHopSetID setID3 = result3.nextHopIdSetIter->second.id;

  // Allocate setID id4 for empty set {}
  RouteNextHopSet nhSet4;
  auto result4 = manager_->getOrAllocRouteNextHopSetID(nhSet4);
  NextHopSetID setID4 = result4.nextHopIdSetIter->second.id;

  // Verify initial state
  EXPECT_EQ(manager_->getIdToNextHop().size(), 4);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 4);

  // Verify NextHop reference counts
  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 2);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 3);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh4), 1);

  // Get NextHopIDs for verification
  auto nhID1 = manager_->getNextHopID(nh1);
  auto nhID2 = manager_->getNextHopID(nh2);
  auto nhID3 = manager_->getNextHopID(nh3);
  auto nhID4 = manager_->getNextHopID(nh4);

  NextHopIDSet expectedIDSet1 = {nhID1.value(), nhID2.value(), nhID3.value()};
  NextHopIDSet expectedIDSet2 = {nhID1.value(), nhID2.value()};
  NextHopIDSet expectedIDSet3 = {nhID2.value(), nhID4.value()};
  NextHopIDSet expectedIDSet4 = {};

  // Verify idToNextHopIdSet map
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID1), expectedIDSet1);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID2), expectedIDSet2);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID3), expectedIDSet3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID4), expectedIDSet4);

  //  Call decrOrDeallocRouteNextHopSetID on id3
  auto deallocResult3 = manager_->decrOrDeallocRouteNextHopSetID(setID3);

  // Verify the deallocation result
  EXPECT_TRUE(deallocResult3.removedSetId.has_value());
  EXPECT_EQ(deallocResult3.removedSetId.value(), setID3);
  // nh4 should be deallocated (refcount was 1)
  EXPECT_EQ(deallocResult3.removedNextHopIds.size(), 1);
  EXPECT_EQ(deallocResult3.removedNextHopIds[0], nhID4.value());

  // NextHopSetID id3 should be deallocated
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().count(setID3), 0);
  EXPECT_EQ(manager_->nextHopIdSetToIDInfo_.count(expectedIDSet3), 0);

  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().count(nhID4.value()), 0);
  EXPECT_EQ(manager_->nextHopToIDInfo_.count(nh4), 0);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 2);

  // Call delOrDecrRouteNextHopSetID on id2
  auto deallocResult2 = manager_->decrOrDeallocRouteNextHopSetID(setID2);

  // Verify the deallocation result
  EXPECT_TRUE(deallocResult2.removedSetId.has_value());
  EXPECT_EQ(deallocResult2.removedSetId.value(), setID2);
  // No NextHops should be deallocated (nh1 and nh2 still have refcounts from
  // set1)
  EXPECT_EQ(deallocResult2.removedNextHopIds.size(), 0);

  // NextHopSetID id2 should be deallocated
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);

  // All NextHops should still exist
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);

  // Allocate id1 again with same set of nexthops
  auto result1Again = manager_->getOrAllocRouteNextHopSetID(nhSet1);
  EXPECT_EQ(result1Again.nextHopIdSetIter->second.id, setID1);

  // Call delOrDecrRouteNextHopSetID on id1 which has been allocated twice
  auto deallocResult1 = manager_->decrOrDeallocRouteNextHopSetID(setID1);

  // Set was not deallocated (refcount decremented but not to 0)
  EXPECT_FALSE(deallocResult1.removedSetId.has_value());
  // No NextHops should be deallocated
  EXPECT_EQ(deallocResult1.removedNextHopIds.size(), 0);

  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);
  // Verify remaining NextHopSetIDs (id1 and id4 should still exist)
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID1), expectedIDSet1);
  EXPECT_EQ(manager_->nextHopIdSetToIDInfo_.count(expectedIDSet1), 1);

  // Deallocate setID4 (empty set) and verify state
  auto deallocResult4 = manager_->decrOrDeallocRouteNextHopSetID(setID4);

  // Verify the deallocation result
  EXPECT_TRUE(deallocResult4.removedSetId.has_value());
  EXPECT_EQ(deallocResult4.removedSetId.value(), setID4);
  // No NextHops to deallocate (empty set)
  EXPECT_EQ(deallocResult4.removedNextHopIds.size(), 0);

  // NextHopSetID id4 should be deallocated
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().count(setID4), 0);
  EXPECT_EQ(manager_->nextHopIdSetToIDInfo_.count(expectedIDSet4), 0);

  // No NextHops should be affected (still 3 NextHops)
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);

  // Call decrOrDeallocRouteNextHopSetID for non-existent id and check it
  // throws FbossError
  NextHopSetID nonExistentID = NextHopSetID(999999);
  EXPECT_THROW(
      manager_->decrOrDeallocRouteNextHopSetID(nonExistentID), FbossError);
}

TEST_F(NextHopIDManagerTest, updateRouteNextHopSetID) {
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);
  NextHop nh4 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.4", UCMP_DEFAULT_WEIGHT);
  NextHop nh5 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.5", UCMP_DEFAULT_WEIGHT);

  // Update with same nexthop set
  RouteNextHopSet nhSet1 = {nh1, nh2};
  auto result1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);
  NextHopSetID setID1 = result1.nextHopIdSetIter->second.id;
  EXPECT_EQ(setID1, NextHopSetID(kSetIdOffset));
  // Verify initial allocation
  EXPECT_EQ(result1.addedNextHopIds.size(), 2);

  auto updateResult1 = manager_->updateRouteNextHopSetID(setID1, nhSet1);
  NextHopSetID updatedSetID1 =
      updateResult1.allocation.nextHopIdSetIter->second.id;
  EXPECT_EQ(updatedSetID1, NextHopSetID(kSetIdOffset + 1));
  // Old set was deallocated
  EXPECT_TRUE(updateResult1.deallocation.removedSetId.has_value());
  EXPECT_EQ(updateResult1.deallocation.removedSetId.value(), setID1);
  // NextHops ARE deallocated first (dealloc happens before alloc)
  EXPECT_EQ(updateResult1.deallocation.removedNextHopIds.size(), 2);
  // NextHops are reallocated (since they were deallocated first)
  EXPECT_EQ(updateResult1.allocation.addedNextHopIds.size(), 2);

  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 2);

  auto nhID1 = manager_->getNextHopID(nh1);
  auto nhID2 = manager_->getNextHopID(nh2);
  NextHopIDSet expectedIDSet1 = {nhID1.value(), nhID2.value()};
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(updatedSetID1), expectedIDSet1);

  // Update with completely different nexthops
  RouteNextHopSet nhSet2 = {nh3, nh4};
  auto updateResult2 = manager_->updateRouteNextHopSetID(updatedSetID1, nhSet2);
  NextHopSetID updatedSetID2 =
      updateResult2.allocation.nextHopIdSetIter->second.id;
  EXPECT_EQ(updatedSetID2, NextHopSetID(kSetIdOffset + 2));

  // Verify deallocation result
  EXPECT_TRUE(updateResult2.deallocation.removedSetId.has_value());
  EXPECT_EQ(updateResult2.deallocation.removedSetId.value(), updatedSetID1);
  // nh1 and nh2 should be deallocated (refcounts were 1)
  EXPECT_EQ(updateResult2.deallocation.removedNextHopIds.size(), 2);

  // Verify allocation result
  // nh3 and nh4 should be newly allocated
  EXPECT_EQ(updateResult2.allocation.addedNextHopIds.size(), 2);

  // Old nexthops should be deallocated
  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 0);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 0);
  EXPECT_EQ(manager_->getIdToNextHop().count(nhID1.value()), 0);
  EXPECT_EQ(manager_->getIdToNextHop().count(nhID2.value()), 0);

  // New nexthops should be allocated
  auto nhID3 = manager_->getNextHopID(nh3);
  auto nhID4 = manager_->getNextHopID(nh4);
  ASSERT_TRUE(nhID3.has_value());
  ASSERT_TRUE(nhID4.has_value());

  NextHopIDSet expectedIDSet2 = {nhID3.value(), nhID4.value()};
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(updatedSetID2), expectedIDSet2);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);

  // Update with partial overlap
  // Note: Since dealloc happens before alloc, nh3 will be deallocated then
  // reallocated
  RouteNextHopSet nhSet3 = {nh3, nh5};
  auto updateResult3 = manager_->updateRouteNextHopSetID(updatedSetID2, nhSet3);
  NextHopSetID updatedSetID3 =
      updateResult3.allocation.nextHopIdSetIter->second.id;
  EXPECT_EQ(updatedSetID3, NextHopSetID(kSetIdOffset + 3));

  // Verify deallocation result - both nh3 and nh4 deallocated (dealloc before
  // alloc)
  EXPECT_TRUE(updateResult3.deallocation.removedSetId.has_value());
  EXPECT_EQ(updateResult3.deallocation.removedNextHopIds.size(), 2);

  // Verify allocation result - both nh3 and nh5 allocated
  EXPECT_EQ(updateResult3.allocation.addedNextHopIds.size(), 2);

  // nh3 gets a new ID (was deallocated, then reallocated)
  auto nhID3New = manager_->getNextHopID(nh3);
  ASSERT_TRUE(nhID3New.has_value());
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh4), 0);

  // nh5 should be newly allocated
  auto nhID5 = manager_->getNextHopID(nh5);
  ASSERT_TRUE(nhID5.has_value());
  EXPECT_EQ(manager_->getNextHopRefCount(nh5), 1);

  NextHopIDSet expectedIDSet3 = {nhID3New.value(), nhID5.value()};
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(updatedSetID3), expectedIDSet3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 2);

  // Update to empty set
  RouteNextHopSet emptySet;
  auto updateResult4 =
      manager_->updateRouteNextHopSetID(updatedSetID3, emptySet);
  NextHopSetID updatedSetID4 =
      updateResult4.allocation.nextHopIdSetIter->second.id;
  EXPECT_EQ(updatedSetID4, NextHopSetID(kSetIdOffset + 4));

  // Verify deallocation result
  EXPECT_TRUE(updateResult4.deallocation.removedSetId.has_value());
  // nh3 and nh5 should be deallocated
  EXPECT_EQ(updateResult4.deallocation.removedNextHopIds.size(), 2);

  // Verify allocation result - empty set allocated
  EXPECT_EQ(updateResult4.allocation.addedNextHopIds.size(), 0);

  // Previous nexthops should be deallocated
  EXPECT_EQ(manager_->getIdToNextHop().size(), 0);

  NextHopIDSet expectedEmptySet = {};
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(updatedSetID4), expectedEmptySet);

  // Update from empty set
  RouteNextHopSet nhSet4 = {nh1, nh2};
  auto updateResult5 = manager_->updateRouteNextHopSetID(updatedSetID4, nhSet4);
  NextHopSetID updatedSetID5 =
      updateResult5.allocation.nextHopIdSetIter->second.id;
  EXPECT_EQ(updatedSetID5, NextHopSetID(kSetIdOffset + 5));

  // Verify deallocation result - empty set deallocated
  EXPECT_TRUE(updateResult5.deallocation.removedSetId.has_value());
  EXPECT_EQ(updateResult5.deallocation.removedNextHopIds.size(), 0);

  // Verify allocation result
  EXPECT_EQ(updateResult5.allocation.addedNextHopIds.size(), 2);

  // New nexthops should be allocated
  nhID1 = manager_->getNextHopID(nh1);
  nhID2 = manager_->getNextHopID(nh2);
  ASSERT_TRUE(nhID1.has_value());
  ASSERT_TRUE(nhID2.has_value());
  EXPECT_EQ(manager_->getIdToNextHop().size(), 2);

  NextHopIDSet expectedIDSet5 = {nhID1.value(), nhID2.value()};
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(updatedSetID5), expectedIDSet5);

  // Update with invalid/non-existent old NextHopSetID
  NextHopSetID nonExistentID = NextHopSetID(999999);
  RouteNextHopSet nhSet8 = {nh1, nh2};
  EXPECT_THROW(
      manager_->updateRouteNextHopSetID(nonExistentID, nhSet8), FbossError);
}

} // namespace facebook::fboss
