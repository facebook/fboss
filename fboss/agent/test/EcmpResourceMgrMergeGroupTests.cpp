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
  void assertGroupsAreUnMerged(
      const EcmpResourceManager::NextHopGroupIds& unmergedGroups) const {
    XLOG(DBG2) << " Asserting for ununmerged group: " << "["
               << folly::join(", ", unmergedGroups) << "]";
    std::for_each(
        unmergedGroups.begin(), unmergedGroups.end(), [this](auto gid) {
          // Groups from  unmerge set should no longer
          // be candidates.
          EXPECT_GT(
              sw_->getEcmpResourceManager()
                  ->getCandidateMergeConsolidationInfo(gid)
                  .size(),
              0);
          EXPECT_FALSE(sw_->getEcmpResourceManager()
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
  // Add a route pointing to new nhops. ECMP limit is breached during
  // update due to make before break. Then the reclaim step notices
  // a freed up primary ECMP group and reclaims it back. So in the
  // end no prefixes have back up ecmp group override set.
  auto deltas = addNextRoute();
  // Route delta + reclaim delta
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), overflowPrefixes);
  assertGroupsAreMerged(optimalMergeSet);
  assertCost(optimalMergeSet);
}

TEST_F(EcmpResourceMgrMergeGroupTest, incReferenceToMergedGroup) {
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto deltas = addNextRoute();
  EXPECT_EQ(deltas.size(), 2);
  auto gid = *optimalMergeSet.begin();
  auto beforeConsolidationInfo =
      sw_->getEcmpResourceManager()->getMergeGroupConsolidationInfo(gid);
  auto gidPfx =
      *sw_->getEcmpResourceManager()->getGroupIdToPrefix()[gid].begin();
  auto newPrefix = nextPrefix();
  deltas = addRoute(
      newPrefix,
      sw_->getEcmpResourceManager()
          ->getGroupInfo(gidPfx.first, gidPfx.second)
          ->getNhops());
  EXPECT_EQ(deltas.size(), 1);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getMergeGroupConsolidationInfo(gid);
  EXPECT_EQ(
      beforeConsolidationInfo->groupId2Penalty.size(),
      afterConsolidationInfo->groupId2Penalty.size());
  for (auto [group, beforePenalty] : beforeConsolidationInfo->groupId2Penalty) {
    if (group == gid) {
      EXPECT_EQ(
          afterConsolidationInfo->groupId2Penalty[group], 2 * beforePenalty);
    } else {
      EXPECT_EQ(afterConsolidationInfo->groupId2Penalty[group], beforePenalty);
    }
  }
}

TEST_F(EcmpResourceMgrMergeGroupTest, addRouteAboveEcmpLimitAndRemove) {
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto beforeConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          *optimalMergeSet.begin());
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  auto newPrefix = nextPrefix();
  addNextRoute();
  assertGroupsAreMerged(optimalMergeSet);
  auto deltas = rmRoute(newPrefix);
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), {});
  assertGroupsAreUnMerged(optimalMergeSet);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          *optimalMergeSet.begin());
  EXPECT_EQ(beforeConsolidationInfo, afterConsolidationInfo);
}

TEST_F(EcmpResourceMgrMergeGroupTest, reclaimPrioritizesGroupsWithHigherCost) {
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto deltas = addNextRoute();
  EXPECT_EQ(deltas.size(), 2);
  assertGroupsAreMerged(optimalMergeSet);
  auto nextOptimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  deltas = addNextRoute();
  EXPECT_EQ(deltas.size(), 2);
  assertGroupsAreMerged(optimalMergeSet);
  assertGroupsAreMerged(nextOptimalMergeSet);
  // Remove one route to make space for reclaiming one of the
  // merged groups
  deltas = rmRoute(*getPrefixesWithoutOverrides().begin());
  EXPECT_EQ(deltas.size(), 2);
  // Assert that highest cost - nextOptimalMergeSet gets reclaimed,
  // while the optimalMergeSet remains merged. That is, merge
  // is done in order of lowest to highest cost, while reclaim is
  // done in the reverse order.
  assertGroupsAreMerged(optimalMergeSet);
  assertGroupsAreUnMerged(nextOptimalMergeSet);
  // Remove one more prefix and assert that all merged groups are
  // reclaimed
  deltas = rmRoute(*getPrefixesWithoutOverrides().begin());
  EXPECT_EQ(deltas.size(), 2);
  assertGroupsAreUnMerged(optimalMergeSet);
}

TEST_F(EcmpResourceMgrMergeGroupTest, reclaimMultipleMergeGroups) {
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  std::vector<RoutePrefixV6> addedPrefixes;
  addedPrefixes.push_back(nextPrefix());
  auto deltas = addNextRoute();
  EXPECT_EQ(deltas.size(), 2);
  auto nextOptimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  addedPrefixes.push_back(nextPrefix());
  deltas = addNextRoute();
  EXPECT_EQ(deltas.size(), 2);
  assertGroupsAreMerged(optimalMergeSet);
  assertGroupsAreMerged(nextOptimalMergeSet);
  XLOG(DBG2) << " Added prefixes : " << folly::join(", ", addedPrefixes);
  std::for_each(
      addedPrefixes.begin(), addedPrefixes.end(), [this](const auto& pfx) {
        auto grpInfo = sw_->getEcmpResourceManager()->getGroupInfo(
            RouterID(0), pfx.toCidrNetwork());
        ASSERT_FALSE(grpInfo->hasOverrides());
      });
  XLOG(DBG2) << " Will delete " << folly::join(", ", addedPrefixes);
  deltas = rmRoutes(addedPrefixes);
  EXPECT_EQ(deltas.size(), 3);
  assertGroupsAreUnMerged(optimalMergeSet);
  assertGroupsAreUnMerged(nextOptimalMergeSet);
}
} // namespace facebook::fboss
