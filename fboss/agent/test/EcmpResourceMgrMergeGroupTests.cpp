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
      const EcmpResourceManager::NextHopGroupIds& unmergedGroups,
      std::optional<int> expectedCandidateMerges = std::nullopt) const {
    XLOG(DBG2) << " Asserting for ununmerged group: " << "["
               << folly::join(", ", unmergedGroups) << "]"
               << " Will check for existence of : "
               << (expectedCandidateMerges
                       ? folly::to<std::string>(*expectedCandidateMerges)
                       : "atleast 1")
               << "candidate merge with each unmerged group";
    std::for_each(
        unmergedGroups.begin(),
        unmergedGroups.end(),
        [this, expectedCandidateMerges](auto gid) {
          auto numCandidateMerges =
              sw_->getEcmpResourceManager()
                  ->getCandidateMergeConsolidationInfo(gid)
                  .size();
          if (expectedCandidateMerges) {
            EXPECT_EQ(numCandidateMerges, *expectedCandidateMerges);
          } else {
            EXPECT_GT(numCandidateMerges, 0);
          }
          // Groups from  unmerge set should no longer
          // be in merge sets.
          EXPECT_FALSE(sw_->getEcmpResourceManager()
                           ->getMergeGroupConsolidationInfo(gid)
                           .has_value());
        });
  }
  void assertGroupsAreRemoved(
      const EcmpResourceManager::NextHopGroupIds& removedGroups) const {
    XLOG(DBG2) << " Asserting for removed groups: " << "["
               << folly::join(", ", removedGroups) << "]";
    const auto& nhops2Id = sw_->getEcmpResourceManager()->getNhopsToId();
    std::for_each(
        nhops2Id.begin(),
        nhops2Id.end(),
        [&removedGroups](const auto& nhopsAndGid) {
          EXPECT_FALSE(removedGroups.contains(nhopsAndGid.second));
        });
  }
  void TearDown() override {
    auto allUnmergedGroups = sw_->getEcmpResourceManager()->getUnMergedGids();
    // All unmerged groups should have candidate merge sets with other unmerged
    // groups
    assertGroupsAreUnMerged(allUnmergedGroups, allUnmergedGroups.size() - 1);
    BaseEcmpResourceMgrMergeGroupsTest::TearDown();
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
  auto deltas = addNextRoute();
  // Route delta + merge delta
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), overflowPrefixes);
  assertGroupsAreMerged(optimalMergeSet);
  assertCost(optimalMergeSet);
}

// Post spillover convert a route to single nhop. Now no spillover.
TEST_F(EcmpResourceMgrMergeGroupTest, overflowThenConvertPrefixToSingleNhop) {
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  auto deltas = addNextRoute();
  // Route delta + merge delta
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), overflowPrefixes);
  auto pfxToMakeSingleHop = *getPrefixesWithoutOverrides().begin();
  deltas =
      updateRoute(pfxToMakeSingleHop, RouteNextHopSet{*defaultNhops().begin()});
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), {});
}

// Post spillover convert spilled over route to single nhop. Now no spillover.
TEST_F(
    EcmpResourceMgrMergeGroupTest,
    overflowThenConvertOverflowRouteToSingleNhop) {
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  auto deltas = addNextRoute();
  // Route delta + merge delta
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), overflowPrefixes);
  auto pfxToMakeSingleHop = *overflowPrefixes.begin();
  deltas =
      updateRoute(pfxToMakeSingleHop, RouteNextHopSet{*defaultNhops().begin()});
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), {});
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

TEST_F(
    EcmpResourceMgrMergeGroupTest,
    addRouteAboveEcmpLimitAndRemoveAllMergeGrpRefrences) {
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  addNextRoute();
  assertEndState(sw_->getState(), overflowPrefixes);
  assertGroupsAreMerged(optimalMergeSet);
  for (auto pfx : overflowPrefixes) {
    rmRoute(pfx);
  }
  assertEndState(sw_->getState(), {});
  assertGroupsAreRemoved(optimalMergeSet);
}

TEST_F(EcmpResourceMgrMergeGroupTest, removeAllMergeGrpRefrencesInOneUpdate) {
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  addNextRoute();
  assertEndState(sw_->getState(), overflowPrefixes);
  assertGroupsAreMerged(optimalMergeSet);
  rmRoutes({overflowPrefixes.begin(), overflowPrefixes.end()});
  assertEndState(sw_->getState(), {});
  assertGroupsAreRemoved(optimalMergeSet);
}

TEST_F(
    EcmpResourceMgrMergeGroupTest,
    removeAllMergeGrpRefrencesInOneUpdateMultipleRoutesToMergeSet) {
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  // Overflow and force a merge
  addNextRoute();
  auto addToMergeSet = nextPrefix();
  addRoute(addToMergeSet, getNextHops(*optimalMergeSet.begin()));
  overflowPrefixes.insert(addToMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 3);
  assertEndState(sw_->getState(), overflowPrefixes);
  assertGroupsAreMerged(optimalMergeSet);
  rmRoutes({overflowPrefixes.begin(), overflowPrefixes.end()});
  assertEndState(sw_->getState(), {});
  assertGroupsAreRemoved(optimalMergeSet);
}
} // namespace facebook::fboss
