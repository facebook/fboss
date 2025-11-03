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

constexpr uint64_t kSetIdOffset = 1ULL << 63;

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

  auto [id1, allocated1] = manager_->getOrAllocateNextHopID(nh1);
  auto [id2, allocated2] = manager_->getOrAllocateNextHopID(nh2);
  auto [id3, allocated3] = manager_->getOrAllocateNextHopID(nh3);

  EXPECT_EQ(id1, NextHopID(1));
  EXPECT_TRUE(allocated1);
  EXPECT_EQ(id2, NextHopID(2));
  EXPECT_TRUE(allocated2);
  EXPECT_EQ(id3, NextHopID(3));
  EXPECT_TRUE(allocated3);

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);

  EXPECT_EQ(manager_->getIdToNextHop().at(id1), nh1);
  EXPECT_EQ(manager_->getIdToNextHop().at(id2), nh2);
  EXPECT_EQ(manager_->getIdToNextHop().at(id3), nh3);

  // Test reusing existing ID

  auto [id4, allocated4] = manager_->getOrAllocateNextHopID(nh1);
  EXPECT_EQ(id1, id4);
  EXPECT_FALSE(allocated4);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().at(id4), nh1);

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 2);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
}

TEST_F(NextHopIDManagerTest, getOrAllocateNextHopSetID) {
  // Test empty set throws exception
  NextHopIDSet emptySet;
  auto [nhID0, allocatedNhID0] = manager_->getOrAllocateNextHopSetID(emptySet);
  EXPECT_EQ(nhID0, NextHopSetID(kSetIdOffset));
  EXPECT_TRUE(allocatedNhID0);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(nhID0), emptySet);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);

  // Test that IDs are allocated sequentially starting from 2^63
  NextHop nh1 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  NextHop nh2 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.2", UCMP_DEFAULT_WEIGHT);
  NextHop nh3 =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.3", UCMP_DEFAULT_WEIGHT);

  auto [nhID1, allocatedNhID1] = manager_->getOrAllocateNextHopID(nh1);
  auto [nhID2, allocatedNhID2] = manager_->getOrAllocateNextHopID(nh2);
  auto [nhID3, allocatedNhID3] = manager_->getOrAllocateNextHopID(nh3);

  NextHopIDSet set1 = {nhID1};
  NextHopIDSet set2 = {nhID1, nhID2};
  NextHopIDSet set3 = {nhID1, nhID2, nhID3};

  auto [setID1, allocatedSetID1] = manager_->getOrAllocateNextHopSetID(set1);
  auto [setID2, allocatedSetID2] = manager_->getOrAllocateNextHopSetID(set2);
  auto [setID3, allocatedSetID3] = manager_->getOrAllocateNextHopSetID(set3);

  EXPECT_EQ(setID1, NextHopSetID(kSetIdOffset + 1));
  EXPECT_TRUE(allocatedSetID1);
  EXPECT_EQ(setID2, NextHopSetID(kSetIdOffset + 2));
  EXPECT_TRUE(allocatedSetID2);
  EXPECT_EQ(setID3, NextHopSetID(kSetIdOffset + 3));
  EXPECT_TRUE(allocatedSetID3);

  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID1), set1);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID2), set2);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID3), set3);

  EXPECT_EQ(manager_->getNextHopRefCount(nh1), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set1), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set2), 1);
  EXPECT_EQ(manager_->getNextHopIDSetRefCount(set3), 1);

  // Reuse existing ID
  auto [setID4, allocatedSetID4] = manager_->getOrAllocateNextHopSetID(set1);
  EXPECT_EQ(setID1, setID4);
  EXPECT_FALSE(allocatedSetID4);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 4);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID4), set1);

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

  auto [nhID1, allocatedNhID1] = manager_->getOrAllocateNextHopID(nh1);
  auto [nhID2, allocatedNhID2] = manager_->getOrAllocateNextHopID(nh2);
  auto [nhID3, allocatedNhID3] = manager_->getOrAllocateNextHopID(nh3);

  NextHopIDSet set1 = {nhID1, nhID2, nhID3};
  NextHopIDSet set2 = {nhID3, nhID1, nhID2};
  NextHopIDSet set3 = {nhID2, nhID3, nhID1};

  auto [setID1, allocatedSetID1] = manager_->getOrAllocateNextHopSetID(set1);
  auto [setID2, allocatedSetID2] = manager_->getOrAllocateNextHopSetID(set2);
  auto [setID3, allocatedSetID3] = manager_->getOrAllocateNextHopSetID(set3);

  EXPECT_EQ(setID1, setID2);
  EXPECT_EQ(setID2, setID3);
  EXPECT_TRUE(allocatedSetID1);
  EXPECT_FALSE(allocatedSetID2);
  EXPECT_FALSE(allocatedSetID3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID1), set1);

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
  auto [id2, allocated2] = manager_->getOrAllocateNextHopID(nh2);
  manager_->getOrAllocateNextHopID(nh3);
  auto [id4, allocated4] = manager_->getOrAllocateNextHopID(nh1);

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
  auto [id5, allocated5] = manager_->getOrAllocateNextHopID(nh2);
  EXPECT_EQ(id5, NextHopID(4));
  EXPECT_TRUE(allocated5);
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

  auto [nhID1, allocatedNhID1] = manager_->getOrAllocateNextHopID(nh1);
  auto [nhID2, allocatedNhID2] = manager_->getOrAllocateNextHopID(nh2);
  auto [nhID3, allocatedNhID3] = manager_->getOrAllocateNextHopID(nh3);

  NextHopIDSet set1 = {nhID1};
  NextHopIDSet set2 = {nhID1, nhID2};
  NextHopIDSet set3 = {nhID1, nhID2, nhID3};
  NextHopIDSet set4 = {};

  // Test throws exception when decrementing/deallocating non-existent set
  EXPECT_THROW(manager_->decrOrDeallocateNextHopIDSet(set1), FbossError);

  manager_->getOrAllocateNextHopSetID(set1);
  auto [setID2, allocatedSetID2] = manager_->getOrAllocateNextHopSetID(set2);
  manager_->getOrAllocateNextHopSetID(set3);
  auto [setID4, allocatedSetID4] = manager_->getOrAllocateNextHopSetID(set1);
  auto [setID5, allocatedSetID5] = manager_->getOrAllocateNextHopSetID(set4);

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
  auto [setID6, allocatedSetID6] = manager_->getOrAllocateNextHopSetID(set2);
  EXPECT_EQ(setID6, NextHopSetID(kSetIdOffset + 4));
  EXPECT_TRUE(allocatedSetID6);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID6), set2);
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

  NextHopSetID setID1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);

  EXPECT_EQ(setID1, NextHopSetID(kSetIdOffset));

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
  EXPECT_EQ(actualSetID1.value(), setID1);

  // Verify idToNextHopIdSet map
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID1), expectedIDSet1);

  // Make NhSet2 with 3 nexthops (nh1, nh2, nh3)
  RouteNextHopSet nhSet2 = {nh1, nh2, nh3};

  NextHopSetID setID2 = manager_->getOrAllocRouteNextHopSetID(nhSet2);

  EXPECT_EQ(setID2, NextHopSetID(kSetIdOffset + 1));

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
  EXPECT_EQ(actualSetID2.value(), setID2);

  // Verify idToNextHopIdSet map has 2 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID2), expectedIDSet2);

  // Add nhSet3 with empty set {}
  RouteNextHopSet nhSet3;

  // Call getOrAllocRouteNextHopSetID for empty set
  NextHopSetID setID3 = manager_->getOrAllocRouteNextHopSetID(nhSet3);

  EXPECT_EQ(setID3, NextHopSetID(kSetIdOffset + 2));

  // Verify no new NextHop IDs were allocated (empty set)
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);

  // Verify NextHopIDSet was created for empty nhSet3
  NextHopIDSet expectedIDSet3 = {};
  auto actualSetID3 = manager_->getNextHopSetID(expectedIDSet3);
  ASSERT_TRUE(actualSetID3.has_value());
  EXPECT_EQ(actualSetID3.value(), setID3);

  // Verify idToNextHopIdSet map has 3 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID3), expectedIDSet3);

  // Call getOrAllocRouteNextHopSetID on empty set again to verify ID reuse
  RouteNextHopSet nhSet4;
  NextHopSetID setID4 = manager_->getOrAllocRouteNextHopSetID(nhSet4);

  // Verify the same ID is returned
  EXPECT_EQ(setID3, setID4);

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

  NextHopSetID setID1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);

  EXPECT_EQ(setID1, NextHopSetID(kSetIdOffset));

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
  EXPECT_EQ(actualSetID1.value(), setID1);

  // Verify idToNextHopIdSet map
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID1), expectedIDSet1);

  // Make NhSet2 with 2 nexthops (nh1, nh2)
  RouteNextHopSet nhSet2 = {nh1, nh2};

  NextHopSetID setID2 = manager_->getOrAllocRouteNextHopSetID(nhSet2);

  EXPECT_EQ(setID2, NextHopSetID(kSetIdOffset + 1));

  // Verify no new NextHop IDs were allocated (reusing existing IDs)
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);

  // Verify NextHopIDSet was created for nhSet2
  NextHopIDSet expectedIDSet2 = {nhID1.value(), nhID2.value()};
  auto actualSetID2 = manager_->getNextHopSetID(expectedIDSet2);
  ASSERT_TRUE(actualSetID2.has_value());
  EXPECT_EQ(actualSetID2.value(), setID2);

  // Verify idToNextHopIdSet map has 2 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID2), expectedIDSet2);

  // Add nhSet3 with 4 nexthops (nh1, nh2, nh3, nh4)
  RouteNextHopSet nhSet3 = {nh1, nh2, nh3, nh4};

  NextHopSetID setID3 = manager_->getOrAllocRouteNextHopSetID(nhSet3);

  EXPECT_EQ(setID3, NextHopSetID(kSetIdOffset + 2));

  // Verify NextHop IDs - nh4 should get a new ID
  auto nhID4 = manager_->getNextHopID(nh4);
  ASSERT_TRUE(nhID4.has_value());
  EXPECT_EQ(nhID4.value(), NextHopID(4));

  // Verify idToNextHop map has 4 entries
  EXPECT_EQ(manager_->getIdToNextHop().size(), 4);
  EXPECT_EQ(manager_->getIdToNextHop().at(nhID4.value()), nh4);

  // Verify NextHopIDSet was created for nhSet3
  NextHopIDSet expectedIDSet3 = {
      nhID1.value(), nhID2.value(), nhID3.value(), nhID4.value()};
  auto actualSetID3 = manager_->getNextHopSetID(expectedIDSet3);
  ASSERT_TRUE(actualSetID3.has_value());
  EXPECT_EQ(actualSetID3.value(), setID3);

  // Verify idToNextHopIdSet map has 3 entries
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID3), expectedIDSet3);

  // Call getOrAllocRouteNextHopSetID on nhSet4 = nhSet2 again to verify ID
  // reuse and
  RouteNextHopSet nhSet4 = {nh1, nh2};
  NextHopSetID setID4 = manager_->getOrAllocRouteNextHopSetID(nhSet4);

  // Verify the same ID is returned
  EXPECT_EQ(setID2, setID4);

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
  NextHopSetID setID1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);

  // Allocate setID id2 for Nh1, nh2
  RouteNextHopSet nhSet2 = {nh1, nh2};
  NextHopSetID setID2 = manager_->getOrAllocRouteNextHopSetID(nhSet2);

  // Allocate setID id3 for Nh2, nh4
  RouteNextHopSet nhSet3 = {nh2, nh4};
  NextHopSetID setID3 = manager_->getOrAllocRouteNextHopSetID(nhSet3);

  // Allocate setID id4 for empty set {}
  RouteNextHopSet nhSet4;
  NextHopSetID setID4 = manager_->getOrAllocRouteNextHopSetID(nhSet4);

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
  bool setID3Deallocated = manager_->decrOrDeallocRouteNextHopSetID(setID3);
  EXPECT_TRUE(setID3Deallocated);

  // NextHopSetID id3 should be deallocated
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().count(setID3), 0);
  EXPECT_EQ(manager_->nextHopIdSetToIDInfo_.count(expectedIDSet3), 0);

  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);
  EXPECT_EQ(manager_->getIdToNextHop().count(nhID4.value()), 0);
  EXPECT_EQ(manager_->nextHopToIDInfo_.count(nh4), 0);
  EXPECT_EQ(manager_->getNextHopRefCount(nh2), 2);

  // Call delOrDecrRouteNextHopSetID on id2
  bool setID2Deallocated = manager_->decrOrDeallocRouteNextHopSetID(setID2);
  EXPECT_TRUE(setID2Deallocated);

  // NextHopSetID id2 should be deallocated
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);

  // All NextHops should still exist
  EXPECT_EQ(manager_->getIdToNextHop().size(), 3);

  // Allocate id1 again with same set of nexthops
  setID1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);

  // Call delOrDecrRouteNextHopSetID on id1 which has been allocated twice
  bool setID1Deallocated = manager_->decrOrDeallocRouteNextHopSetID(setID1);
  EXPECT_FALSE(setID1Deallocated);

  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 2);
  // Verify remaining NextHopSetIDs (id1 and id4 should still exist)
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(setID1), expectedIDSet1);
  EXPECT_EQ(manager_->nextHopIdSetToIDInfo_.count(expectedIDSet1), 1);

  // Deallocate setID4 (empty set) and verify state
  bool setID4Deallocated = manager_->decrOrDeallocRouteNextHopSetID(setID4);
  EXPECT_TRUE(setID4Deallocated);

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
  NextHopSetID setID1 = manager_->getOrAllocRouteNextHopSetID(nhSet1);
  EXPECT_EQ(setID1, NextHopSetID(kSetIdOffset));

  NextHopSetID updatedSetID1 =
      manager_->updateRouteNextHopSetID(setID1, nhSet1);
  EXPECT_EQ(updatedSetID1, NextHopSetID(kSetIdOffset + 1));
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 2);

  auto nhID1 = manager_->getNextHopID(nh1);
  auto nhID2 = manager_->getNextHopID(nh2);
  NextHopIDSet expectedIDSet1 = {nhID1.value(), nhID2.value()};
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(updatedSetID1), expectedIDSet1);

  // Update with completely different nexthops
  RouteNextHopSet nhSet2 = {nh3, nh4};
  NextHopSetID updatedSetID2 =
      manager_->updateRouteNextHopSetID(updatedSetID1, nhSet2);
  EXPECT_EQ(updatedSetID2, NextHopSetID(kSetIdOffset + 2));

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
  RouteNextHopSet nhSet3 = {nh3, nh5};
  NextHopSetID updatedSetID3 =
      manager_->updateRouteNextHopSetID(updatedSetID2, nhSet3);
  EXPECT_EQ(updatedSetID3, NextHopSetID(kSetIdOffset + 3));

  // nh3 should still exist (shared), nh4 should be deallocated
  nhID3 = manager_->getNextHopID(nh3);
  EXPECT_EQ(manager_->getNextHopRefCount(nh3), 1);
  EXPECT_EQ(manager_->getNextHopRefCount(nh4), 0);
  EXPECT_EQ(manager_->getIdToNextHop().count(nhID4.value()), 0);

  // nh5 should be newly allocated
  auto nhID5 = manager_->getNextHopID(nh5);
  ASSERT_TRUE(nhID5.has_value());
  EXPECT_EQ(manager_->getNextHopRefCount(nh5), 1);

  NextHopIDSet expectedIDSet3 = {nhID3.value(), nhID5.value()};
  EXPECT_EQ(manager_->getIdToNextHopIdSet().at(updatedSetID3), expectedIDSet3);
  EXPECT_EQ(manager_->getIdToNextHopIdSet().size(), 1);
  EXPECT_EQ(manager_->getIdToNextHop().size(), 2);

  // Update to empty set
  RouteNextHopSet emptySet;
  NextHopSetID updatedSetID4 =
      manager_->updateRouteNextHopSetID(updatedSetID3, emptySet);
  EXPECT_EQ(updatedSetID4, NextHopSetID(kSetIdOffset + 4));

  // Previous nexthops should be deallocated
  EXPECT_EQ(manager_->getIdToNextHop().size(), 0);

  NextHopIDSet expectedEmptySet = {};
  EXPECT_EQ(
      manager_->getIdToNextHopIdSet().at(updatedSetID4), expectedEmptySet);

  // Update from empty set
  RouteNextHopSet nhSet4 = {nh1, nh2};
  NextHopSetID updatedSetID5 =
      manager_->updateRouteNextHopSetID(updatedSetID4, nhSet4);
  EXPECT_EQ(updatedSetID5, NextHopSetID(kSetIdOffset + 5));

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
