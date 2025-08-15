/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/BaseEcmpResourceMgrMergeGroupsTests.h"

namespace facebook::fboss {

class EcmpResourceMgrMergeGroupTest
    : public BaseEcmpResourceMgrMergeGroupsTest {
 public:
  void setupFlags() const override {
    FLAGS_enable_ecmp_resource_manager = true;
    FLAGS_ecmp_resource_percentage = 35;
  }
  void assertCost(
      const EcmpResourceManager::NextHopGroupIds& mergedGroups) const {
    auto consolidationInfo =
        sw_->getEcmpResourceManager()
            ->getMergeGroupConsolidationInfo(*mergedGroups.begin())
            .value();
    auto expectedPenalty = consolidationInfo.maxPenalty();
    for (auto gid : mergedGroups) {
      EXPECT_EQ(sw_->getEcmpResourceManager()->getCost(gid), expectedPenalty);
    }
  }
  void assertGroupsAreMerged(
      const EcmpResourceManager::NextHopGroupIds& mergedGroups) const {
    XLOG(DBG2) << " Asserting for merged group: " << "["
               << folly::join(", ", mergedGroups) << "]";
    std::for_each(mergedGroups.begin(), mergedGroups.end(), [this](auto gid) {
      // Groups from  merge set should no longer
      // be candidates.
      EXPECT_EQ(
          sw_->getEcmpResourceManager()
              ->getCandidateMergeConsolidationInfo(gid)
              .size(),
          0);
      EXPECT_TRUE(sw_->getEcmpResourceManager()
                      ->getMergeGroupConsolidationInfo(gid)
                      .has_value());
    });
  }
};

// Base class add 5 groups, which is within in the
// Ecmp resource mgr limit.
TEST_F(EcmpResourceMgrMergeGroupTest, init) {}

TEST_F(EcmpResourceMgrMergeGroupTest, addRouteAboveEcmpLimit) {
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  // Update a route pointing to new nhops. ECMP limit is breached during
  // update due to make before break. Then the reclaim step notices
  // a freed up primary ECMP group and reclaims it back. So in the
  // end no prefixes have back up ecmp group override set.
  auto nhopSets = nextNhopSets();
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto newRoute = makeRoute(nextPrefix(), *nhopSets.begin())->clone();
  newRoute->setResolved(
      RouteNextHopEntry(*nhopSets.begin(), kDefaultAdminDistance));
  fib6->addNode(newRoute);
  auto deltas = consolidate(newState);
  // Route delta + reclaim delta
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(newState, overflowPrefixes);
  assertCost(optimalMergeSet);
}

TEST_F(EcmpResourceMgrMergeGroupTest, addRouteAboveEcmpLimitAndRemove) {
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto overflowPrefixes = getPrefixesForGroups(
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet());
  EXPECT_EQ(overflowPrefixes.size(), 2);
  // Update a route pointing to new nhops. ECMP limit is breached during
  // update due to make before break. Then the reclaim step notices
  // a freed up primary ECMP group and reclaims it back. So in the
  // end no prefixes have back up ecmp group override set.
  auto nhopSets = nextNhopSets();
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto newRoute = makeRoute(nextPrefix(), *nhopSets.begin())->clone();
  newRoute->setResolved(
      RouteNextHopEntry(*nhopSets.begin(), kDefaultAdminDistance));
  fib6->addNode(newRoute);
  consolidate(newState);
  auto deltas = rmRoute(newRoute->prefix());
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), {});
}
} // namespace facebook::fboss
