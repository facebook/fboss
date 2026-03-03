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
#include "fboss/agent/state/NextHopIdMaps.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

namespace {

NextHopThrift createNextHopThrift(const std::string& ip, int weight = 1) {
  NextHopThrift nextHop;
  nextHop.address() = network::toBinaryAddress(folly::IPAddress(ip));
  nextHop.weight() = weight;
  return nextHop;
}

} // namespace

class IdToNextHopMapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    map_ = std::make_shared<IdToNextHopMap>();
  }

  std::shared_ptr<IdToNextHopMap> map_;
};

TEST_F(IdToNextHopMapTest, AddGetNextHopAndIds) {
  auto nextHop1 = createNextHopThrift("10.0.0.1");
  auto nextHop2 = createNextHopThrift("10.0.0.2", 2);

  // Add and retrieve
  map_->addNextHop(1, nextHop1);
  map_->addNextHop(2, nextHop2);
  EXPECT_EQ(map_->size(), 2);
  EXPECT_EQ(map_->getNextHop(1)->toThrift(), nextHop1);
  EXPECT_EQ(*map_->getNextHop(2)->toThrift().weight(), 2);

  // GetIf returns nullptr for non-existent, doesn't throw
  EXPECT_NE(map_->getNextHopIf(1), nullptr);
  EXPECT_EQ(map_->getNextHopIf(999), nullptr);

  // Throwing behavior
  EXPECT_THROW(map_->getNextHop(999), FbossError);
  EXPECT_THROW(map_->addNextHop(1, nextHop1), FbossError);
  EXPECT_THROW(map_->removeNextHop(999), FbossError);

  // RemoveIf returns node or nullptr
  EXPECT_EQ(map_->removeNextHopIf(999), nullptr);
  EXPECT_NE(map_->removeNextHopIf(1), nullptr);
  EXPECT_EQ(map_->size(), 1);

  // Remove remaining
  map_->removeNextHop(2);
  EXPECT_EQ(map_->size(), 0);
}

class IdToNextHopIdSetMapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    nextHopIdSetMap_ = std::make_shared<IdToNextHopIdSetMap>();
  }

  std::shared_ptr<IdToNextHopIdSetMap> nextHopIdSetMap_;
};

TEST_F(IdToNextHopIdSetMapTest, AddGetNextHopIDSetAndSetIds) {
  std::set<NextHopId> nextHopIds1{1, 2, 3};
  std::set<NextHopId> nextHopIds2{4, 5};
  std::set<NextHopId> emptySet;

  // Add and retrieve
  nextHopIdSetMap_->addNextHopIdSet(100, nextHopIds1);
  nextHopIdSetMap_->addNextHopIdSet(200, nextHopIds2);
  nextHopIdSetMap_->addNextHopIdSet(300, emptySet);
  EXPECT_EQ(nextHopIdSetMap_->size(), 3);
  EXPECT_EQ(nextHopIdSetMap_->getNextHopIdSet(100)->toThrift(), nextHopIds1);
  EXPECT_EQ(nextHopIdSetMap_->getNextHopIdSet(200)->toThrift(), nextHopIds2);
  EXPECT_TRUE(nextHopIdSetMap_->getNextHopIdSet(300)->toThrift().empty());

  // GetIf returns nullptr for non-existent, doesn't throw
  EXPECT_NE(nextHopIdSetMap_->getNextHopIdSetIf(100), nullptr);
  EXPECT_EQ(nextHopIdSetMap_->getNextHopIdSetIf(999), nullptr);

  // Throwing behavior
  EXPECT_THROW(nextHopIdSetMap_->getNextHopIdSet(999), FbossError);
  EXPECT_THROW(nextHopIdSetMap_->removeNextHopIdSet(999), FbossError);

  // RemoveIf returns true/false
  EXPECT_FALSE(nextHopIdSetMap_->removeNextHopIdSetIf(999));
  EXPECT_TRUE(nextHopIdSetMap_->removeNextHopIdSetIf(100));
  EXPECT_EQ(nextHopIdSetMap_->size(), 2);

  // Remove remaining
  nextHopIdSetMap_->removeNextHopIdSet(200);
  nextHopIdSetMap_->removeNextHopIdSet(300);
  EXPECT_EQ(nextHopIdSetMap_->size(), 0);
}

} // namespace facebook::fboss
