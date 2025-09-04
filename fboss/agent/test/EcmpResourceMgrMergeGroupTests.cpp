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
  std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode()
      const override {
    return std::nullopt;
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
  assertMergedGroup(optimalMergeSet);
  assertCost(optimalMergeSet);
}

TEST_F(EcmpResourceMgrMergeGroupTest, addV4RouteAboveEcmpLimit) {
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 2);
  auto defaultNhops = defaultV4Nhops();
  auto newRoute = makeV4Route(makeV4Prefix(1), defaultNhops);
  auto newState = state_->clone();
  auto fib = fib4(newState);
  fib->addNode(newRoute->prefix().str(), std::move(newRoute));
  newState->publish();
  auto deltas = consolidate(newState);
  // Route delta + merge delta
  EXPECT_EQ(deltas.size(), 2);
  assertTargetState(
      sw_->getState(), newState, overflowPrefixes, nullptr, false);
  assertMergedGroup(optimalMergeSet);
  assertCost(optimalMergeSet);
}

TEST_F(EcmpResourceMgrMergeGroupTest, multiplePrefixesInEachMergeGrpMember) {
  for (const auto& [nhops, _] : sw_->getEcmpResourceManager()->getNhopsToId()) {
    auto deltas = addRoute(nextPrefix(), nhops);
    EXPECT_EQ(deltas.size(), 1);
  }
  // Cache prefixes to be affected by optimal merge grp selection.
  // We will later assert that these start pointing to merged groups.
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  auto overflowPrefixes = getPrefixesForGroups(optimalMergeSet);
  EXPECT_EQ(overflowPrefixes.size(), 4);
  auto deltas = addNextRoute();
  // Route delta + merge delta
  EXPECT_EQ(deltas.size(), 2);
  assertEndState(sw_->getState(), overflowPrefixes);
  assertMergedGroup(optimalMergeSet);
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
  assertMergedGroup(optimalMergeSet);
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
  assertMergedGroup(optimalMergeSet);
  auto nextOptimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  deltas = addNextRoute();
  EXPECT_EQ(deltas.size(), 2);
  assertMergedGroup(optimalMergeSet);
  assertMergedGroup(nextOptimalMergeSet);
  // Remove one route to make space for reclaiming one of the
  // merged groups
  deltas = rmRoute(*getPrefixesWithoutOverrides().begin());
  EXPECT_EQ(deltas.size(), 2);
  // Assert that highest cost - nextOptimalMergeSet gets reclaimed,
  // while the optimalMergeSet remains merged. That is, merge
  // is done in order of lowest to highest cost, while reclaim is
  // done in the reverse order.
  assertMergedGroup(optimalMergeSet);
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
  assertMergedGroup(optimalMergeSet);
  assertMergedGroup(nextOptimalMergeSet);
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
  assertMergedGroup(optimalMergeSet);
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
  assertMergedGroup(optimalMergeSet);
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
  assertMergedGroup(optimalMergeSet);
  rmRoutes({overflowPrefixes.begin(), overflowPrefixes.end()});
  assertEndState(sw_->getState(), {});
  assertGroupsAreRemoved(optimalMergeSet);
}

TEST_F(EcmpResourceMgrMergeGroupTest, exhaustAllPairwiseMerges) {
  auto nhopSets = nextNhopSets();
  std::set<RouteV6::Prefix> overflowPrefixes;
  std::vector<RouteV6::Prefix> addedPrefixes;
  auto removeRoutesAndCheck = [&addedPrefixes, this]() {
    // Now delete all the newly added prefixes. We should go back to
    // no overflow
    rmRoutes(addedPrefixes);
    assertEndState(sw_->getState(), {});
    // No merged groups
    EXPECT_EQ(sw_->getEcmpResourceManager()->getMergedGroups().size(), 0);
    EXPECT_EQ(getAllGroups(), getGroupsWithoutOverrides());
  };
  {
    // On the last route add we would have exhausted
    // all pairwise merges, and will now do a 3 group
    // merge
    for (auto i = 0; i < numStartRoutes(); ++i) {
      auto oldState = state_;
      auto newState = oldState->clone();
      auto fib6 = fib(newState);
      auto optimalMergeSet =
          sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
      auto prefixesForMergeSet = getPrefixesForGroups(optimalMergeSet);
      overflowPrefixes.insert(
          prefixesForMergeSet.begin(), prefixesForMergeSet.end());
      addedPrefixes.push_back(nextPrefix());
      auto route = makeRoute(addedPrefixes.back(), nhopSets[i]);
      fib6->addNode(route);
      auto deltas = consolidate(newState);
      EXPECT_EQ(deltas.size(), 2);
    }
    assertEndState(sw_->getState(), overflowPrefixes);
    auto allMergedGroups = sw_->getEcmpResourceManager()->getMergedGroups();
    EXPECT_EQ(
        std::count_if(
            allMergedGroups.begin(),
            allMergedGroups.end(),
            [](const auto& gids) { return gids.size() > 2; }),
        1);
  }
  removeRoutesAndCheck();
  {
    // Now add all the previously added routes in one shot. It should
    // result in the same end state as before
    auto oldState = state_;
    auto newState = oldState->clone();
    for (auto i = 0; i < numStartRoutes(); ++i) {
      auto fib6 = fib(newState);
      auto route = makeRoute(addedPrefixes[i], nhopSets[i]);
      fib6->addNode(route);
    }
    auto deltas = consolidate(newState);
    EXPECT_EQ(deltas.size(), addedPrefixes.size() + 1);
    assertEndState(sw_->getState(), overflowPrefixes);
    auto allMergedGroups = sw_->getEcmpResourceManager()->getMergedGroups();
    EXPECT_EQ(
        std::count_if(
            allMergedGroups.begin(),
            allMergedGroups.end(),
            [](const auto& gids) { return gids.size() > 2; }),
        1);
  }
  removeRoutesAndCheck();
}

TEST_F(EcmpResourceMgrMergeGroupTest, addRoutesAboveEcmpLimitAndReplay) {
  // Add new routes pointing to new nhops. ECMP limit is breached.
  auto nhopSets = nextNhopSets();
  std::set<RouteV6::Prefix> overflowPrefixes;
  auto replayRoutes =
      [this, &overflowPrefixes](const std::set<RouteV6::Prefix>& toReplay) {
        auto newerState = state_->clone();
        auto fib6 = fib(newerState);
        for (const auto& origRoute : getPostConfigResolvedRoutes(state_)) {
          auto route = fib6->getRouteIf(origRoute->prefix())->clone();
          if (toReplay.size() && !toReplay.contains(route->prefix())) {
            continue;
          }
          route->publish();
          fib6->updateNode(route);
        }
        auto deltas2 = consolidate(newerState);
        EXPECT_EQ(deltas2.size(), 1);
        assertEndState(sw_->getState(), overflowPrefixes);
      };
  {
    // On the last route add we would have exhausted
    // all pairwise merges, and will now do a 3 group
    // merge
    for (auto i = 0; i < numStartRoutes(); ++i) {
      auto oldState = state_;
      auto newState = oldState->clone();
      auto fib6 = fib(newState);
      auto optimalMergeSet =
          sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
      auto prefixesForMergeSet = getPrefixesForGroups(optimalMergeSet);
      overflowPrefixes.insert(
          prefixesForMergeSet.begin(), prefixesForMergeSet.end());
      auto route = makeRoute(nextPrefix(), nhopSets[i]);
      fib6->addNode(route);
      auto deltas = consolidate(newState);
      EXPECT_EQ(deltas.size(), 2);
    }
    assertEndState(sw_->getState(), overflowPrefixes);
  }
  {
    // Replay state with new pointers for the overflow routes,
    // should just return with the same prefixes marked for overflow
    XLOG(INFO) << " Replaying state with cloned overflow routes, "
               << "should get identical result";
    replayRoutes(overflowPrefixes);
  }
  {
    // Replay state with new pointers for all routes
    // should just return with the same prefixes marked for overflow
    XLOG(INFO) << " Replaying state with cloned non-overflow routes, "
               << "should get identical result";
    replayRoutes({});
  }
}

TEST_F(EcmpResourceMgrMergeGroupTest, updateAllRoutesToNewNhopsAboveEcmpLimit) {
  {
    // Add routes pointing to existing nhops. ECMP limit is not breached.
    auto oldState = state_;
    auto newState = oldState->clone();
    auto fib6 = fib(newState);
    auto existingNhopSets = defaultNhopSets();
    auto routesBefore = getPostConfigResolvedRoutes(oldState).size();
    for (auto i = 0; i < routesBefore; ++i) {
      auto newRoute =
          makeRoute(makePrefix(routesBefore + i), existingNhopSets[i]);
      fib6->addNode(newRoute);
    }
    auto deltas = consolidate(newState);
    EXPECT_EQ(deltas.size(), 1);
    assertEndState(newState, {});
  }
  {
    /*
     * Initial state
     * R1, R5 -> G1
     * R2, R6 -> G2
     * R3, R7 -> G3
     * R4, R8 -> G4
     * R0, R9 -> G5
     * New state
     * R1 -> G6
     * R2 -> G7
     * R3 -> G8
     * R4 -> G9
     * R5 -> G10
     * R6 -> G10
     * R7 -> G9
     * R8 -> G8
     * R9 -> G7
     * R0 -> G6
     */
    auto oldState = state_;
    auto newState = oldState->clone();
    auto fib6 = fib(newState);
    auto nhopSets = nextNhopSets();
    auto idx = 0;
    bool idxDirectionFwd = true;
    for (const auto& route : getPostConfigResolvedRoutes(oldState)) {
      auto newRoute = fib6->getRouteIf(route->prefix())->clone();
      newRoute->setResolved(RouteNextHopEntry(
          nhopSets[idxDirectionFwd ? idx++ : idx--], kDefaultAdminDistance));
      fib6->updateNode(newRoute);
      if (idx == nhopSets.size()) {
        idxDirectionFwd = false;
        --idx;
      }
    }
    auto deltas = consolidate(newState);
    /*
     * Each prefix update triggers a merge or reclaim. Last update triggers
     * a update and reclaim.
     */
    EXPECT_EQ(deltas.size(), getPostConfigResolvedRoutes(oldState).size() + 1);
    assertEndState(newState, {});
  }
}

TEST_F(EcmpResourceMgrMergeGroupTest, overflowRoutesInReverseOrderOfReplay) {
  std::set<RouteV6::Prefix> startPrefixes;
  for (const auto& route : getPostConfigResolvedRoutes(state_)) {
    startPrefixes.insert(route->prefix());
  }
  // clear all routes
  {
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    fib6->clear();
    auto deltas = consolidate(newState);
    EXPECT_EQ(deltas.size(), 1);
    assertEndState(newState, {});
  }
  /*
  State before
  - No routes
  State after
    R5 -> G1
    R6 -> G2
    R7 -> G3
    R8 -> G4
    R9 -> G5
  */
  {
    // Add routes pointing to default nhop sets ECMP limit is not breached.
    auto oldState = state_;
    auto newState = oldState->clone();
    auto fib6 = fib(newState);
    auto defaultNhops = defaultNhopSets();
    for (auto i = 0; i < startPrefixes.size(); ++i) {
      auto newRoute =
          makeRoute(makePrefix(startPrefixes.size() + i), defaultNhops[i]);
      fib6->addNode(newRoute);
    }
    auto deltas = consolidate(newState);
    EXPECT_EQ(deltas.size(), 1);
    assertEndState(newState, {});
  }
  /*
  State before
    R5 -> G1
    R6 -> G2
    R7 -> G3
    R8 -> G4
    R9 -> G5
  State after
    R0 -> G6
    R1 -> G7
    R2 -> G8
    R3 -> G9
    R4 -> G10
    R5 -> G1
    R6 -> G2
    R7 -> G3
    R8 -> G4
    R9 -> G5
  */
  {
    // Add new routes. These will trigger mergess
    auto oldState = state_;
    auto newState = oldState->clone();
    auto fib6 = fib(newState);
    auto newNhops = nextNhopSets();
    auto idx = 0;
    for (const auto& pfx : startPrefixes) {
      auto newRoute = makeRoute(pfx, newNhops[idx++]);
      fib6->addNode(newRoute);
    }
    // BaseEcmpResourceManagerTest::consolidate does a replay
    // of newState and asserts that same set of prefixes gets
    // marked for backup ecmp group. Even though had we played
    // the current start from start routes from R5-R9 would have
    // ended up with backup ecmp group type nhops
    auto deltas = consolidate(newState);
    EXPECT_EQ(deltas.size(), startPrefixes.size() + 1);
  }
}

TEST_F(EcmpResourceMgrMergeGroupTest, reclaimOnReplay) {
  // Add new routes pointing to new nhops. ECMP limit is breached.
  auto nhopSets = nextNhopSets();
  auto routesBefore = getPostConfigResolvedRoutes(sw_->getState()).size();
  std::set<RouteV6::Prefix> overflowPrefixes;
  for (auto i = 0; i < numStartRoutes(); ++i) {
    auto oldState = state_;
    auto newState = oldState->clone();
    auto fib6 = fib(newState);
    auto optimalMergeSet =
        sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
    auto prefixesForMergeSet = getPrefixesForGroups(optimalMergeSet);
    overflowPrefixes.insert(
        prefixesForMergeSet.begin(), prefixesForMergeSet.end());
    auto route = makeRoute(makePrefix(routesBefore + i), nhopSets[i]);
    fib6->addNode(route);
    auto deltas = consolidate(newState);
    EXPECT_EQ(deltas.size(), 2);
  }
  assertEndState(sw_->getState(), overflowPrefixes);
  auto newConsolidator = makeResourceMgrWithEcmpLimit(
      cfib(state_)->size() +
      FLAGS_ecmp_resource_manager_make_before_break_buffer);
  auto replayDeltas = newConsolidator->reconstructFromSwitchState(state_);
  ASSERT_EQ(replayDeltas.size(), 1);
  // No overflow, since we increased the limit to cover all the prefixes
  assertTargetState(
      replayDeltas.back().newState(),
      state_,
      {},
      newConsolidator.get(),
      false /*checkStats*/);
}
} // namespace facebook::fboss
