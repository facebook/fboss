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

} // namespace facebook::fboss
