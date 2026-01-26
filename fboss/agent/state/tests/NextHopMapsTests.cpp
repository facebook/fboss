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
#include "fboss/agent/state/SwitchState.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>
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
} // namespace facebook::fboss
