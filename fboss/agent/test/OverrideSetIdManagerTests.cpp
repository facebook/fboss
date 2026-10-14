// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/OverrideSetIdManager.h"

namespace facebook::fboss {

class OverrideSetIdManagerTest : public ::testing::Test {
 protected:
  OverrideSetIdManager mgr_;
};

TEST_F(OverrideSetIdManagerTest, overrideRangeFollowsRibRange) {
  EXPECT_FALSE(OverrideSetIdManager::isOverrideSetID(NextHopSetID(1LL << 62)));
  EXPECT_FALSE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 60) - 1)));
  EXPECT_TRUE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 60))));
  EXPECT_TRUE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 61) - 1)));
  EXPECT_FALSE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 61))));
}

} // namespace facebook::fboss
