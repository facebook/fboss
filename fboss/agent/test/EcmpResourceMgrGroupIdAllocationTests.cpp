/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"

namespace facebook::fboss {

class NextHopIdAllocatorTest : public BaseEcmpResourceManagerTest {};

TEST_F(NextHopIdAllocatorTest, init) {
  const auto& nhops2Id = consolidator_->getNhopsToId();
  EXPECT_EQ(nhops2Id.size(), 1);
  auto id = *getNhopId(defaultNhops());
  EXPECT_EQ(id, 1);
  // All routes point to same nhop group
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(id),
      numPostConfigResolvedRoutes(state_));
}

TEST_F(NextHopIdAllocatorTest, addRouteSameNhops) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  fib6->addNode(makeRoute(nextPrefix(), defaultNhops()));
  EXPECT_EQ(fib6->size(), routesBefore + 1);
  consolidate(newState);
  const auto& nhops2Id = consolidator_->getNhopsToId();
  EXPECT_EQ(nhops2Id.size(), 1);
  auto id = *getNhopId(defaultNhops());
  EXPECT_EQ(id, 1);
  // All routes point to same nhop group
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(id),
      numPostConfigResolvedRoutes(state_));
}

TEST_F(NextHopIdAllocatorTest, addRouteNewNhops) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  auto newNhops = defaultNhops();
  newNhops.erase(newNhops.begin());
  fib6->addNode(makeRoute(nextPrefix(), newNhops));
  EXPECT_EQ(fib6->size(), routesBefore + 1);
  consolidate(newState);
  const auto& nhops2Id = consolidator_->getNhopsToId();
  EXPECT_EQ(nhops2Id.size(), 2);
  auto idDefaultNhops = *getNhopId(defaultNhops());
  EXPECT_EQ(idDefaultNhops, 1);
  // All but one routes point to same nhop group
  EXPECT_EQ(
      numPostConfigResolvedRoutes(state_) - 1,
      consolidator_->getRouteUsageCount(idDefaultNhops));
  auto idNewNhops = *getNhopId(newNhops);
  EXPECT_EQ(idNewNhops, 2);
  // One route points to new nhop group
  EXPECT_EQ(consolidator_->getRouteUsageCount(2), 1);
}

TEST_F(NextHopIdAllocatorTest, addRemoveRouteNewNhopsUnresolved) {
  auto newState = state_->clone();
  const auto& nhops2Id = consolidator_->getNhopsToId();
  auto groupId = *getNhopId(defaultNhops());
  EXPECT_EQ(groupId, 1);
  EXPECT_EQ(nhops2Id.size(), 1);
  EXPECT_EQ(nhops2Id.find(defaultNhops())->second, groupId);
  auto newNhops = defaultNhops();
  newNhops.erase(newNhops.begin());
  auto newRoute = makeRoute(nextPrefix(), newNhops)->clone();
  newRoute->clearForward();
  {
    auto fib6 = fib(newState);
    auto routesBefore = fib6->size();
    // All routes point to same nhop group
    auto postConfigRoutesBefore = numPostConfigResolvedRoutes(newState);
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(groupId), postConfigRoutesBefore);
    fib6->addNode(newRoute);
    EXPECT_EQ(fib6->size(), routesBefore + 1);
    consolidate(newState);
    // New nhops don't get a id, since no resolved routes point to it
    EXPECT_FALSE(getNhopId(newNhops).has_value());
    // All routes point to same nhop group, new route is unresolved
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(groupId), postConfigRoutesBefore);
  }
  {
    auto newerState = newState->clone();
    auto fib6 = fib(newerState);
    auto routesBefore = fib6->size();
    // All resolved routes point to same nhop group
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(groupId),
        numPostConfigResolvedRoutes(newState));
    fib6->removeNode(newRoute);
    EXPECT_EQ(fib6->size(), routesBefore - 1);
    consolidate(newerState);
    const auto& nhops2Id = consolidator_->getNhopsToId();
    EXPECT_EQ(nhops2Id.size(), 1);
    EXPECT_EQ(*getNhopId(defaultNhops()), groupId);
    EXPECT_FALSE(getNhopId(newNhops).has_value());
    // All resolved routes point to same nhop group
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(groupId),
        numPostConfigResolvedRoutes(newerState));
  }
}

TEST_F(NextHopIdAllocatorTest, updateRouteNhops) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  auto newNhops = defaultNhops();
  newNhops.erase(newNhops.begin());
  fib6->updateNode(makeRoute(makePrefix(0), newNhops));
  EXPECT_EQ(fib6->size(), routesBefore);
  consolidate(newState);
  const auto& nhops2Id = consolidator_->getNhopsToId();
  EXPECT_EQ(nhops2Id.size(), 2);
  auto defaultNhopsId = *getNhopId(defaultNhops());
  auto newNhopsId = *getNhopId(newNhops);
  EXPECT_EQ(defaultNhopsId, 1);
  EXPECT_EQ(newNhopsId, 2);
  // All but one route point to defaultNhops
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(defaultNhopsId),
      numPostConfigResolvedRoutes(newState) - 1);
  EXPECT_EQ(consolidator_->getRouteUsageCount(newNhopsId), 1);
}

TEST_F(NextHopIdAllocatorTest, updateAllRoutes) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  auto newNhops = defaultNhops();
  newNhops.erase(newNhops.begin());
  for (const auto& route6 : getPostConfigResolvedRoutes(state_)) {
    auto newRoute = route6->clone();
    newRoute->setResolved(RouteNextHopEntry(newNhops, kDefaultAdminDistance));
    fib6->updateNode(newRoute);
  }
  EXPECT_EQ(fib6->size(), routesBefore);
  consolidate(newState);
  const auto& nhops2Id = consolidator_->getNhopsToId();
  EXPECT_EQ(nhops2Id.size(), 1);
  EXPECT_FALSE(getNhopId(defaultNhops()).has_value());
  auto newNhopsId = *getNhopId(newNhops);
  EXPECT_EQ(newNhopsId, 2);
  // All routes route point to newNhopsId
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(newNhopsId),
      numPostConfigResolvedRoutes(newState));
}

TEST_F(NextHopIdAllocatorTest, updateRouteNhopsMultipleTimes) {
  auto newNhops = defaultNhops();
  newNhops.erase(newNhops.begin());
  auto newerNhops = newNhops;
  newerNhops.erase(newerNhops.begin());

  {
    // Update first route to new nhops
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto routesBefore = fib6->size();
    fib6->updateNode(makeRoute(makePrefix(0), newNhops));
    EXPECT_EQ(fib6->size(), routesBefore);
    consolidate(newState);
    const auto& nhops2Id = consolidator_->getNhopsToId();
    EXPECT_EQ(nhops2Id.size(), 2);
    auto defaultNhopsId = *getNhopId(defaultNhops());
    auto newNhopsId = *getNhopId(newNhops);
    EXPECT_EQ(defaultNhopsId, 1);
    EXPECT_EQ(newNhopsId, 2);
    // All but one route point to defaultNhops
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(defaultNhopsId),
        numPostConfigResolvedRoutes(newState) - 1);
    EXPECT_EQ(consolidator_->getRouteUsageCount(newNhopsId), 1);
  }
  {
    // Update first route to newer nhops
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto routesBefore = fib6->size();
    fib6->updateNode(makeRoute(makePrefix(0), newerNhops));
    EXPECT_EQ(fib6->size(), routesBefore);
    consolidate(newState);
    const auto& nhops2Id = consolidator_->getNhopsToId();
    EXPECT_EQ(nhops2Id.size(), 2);
    auto defaultNhopsId = *getNhopId(defaultNhops());
    EXPECT_FALSE(getNhopId(newNhops).has_value());
    auto newerNhopsId = *getNhopId(newerNhops);
    EXPECT_EQ(defaultNhopsId, 1);
    EXPECT_EQ(newerNhopsId, 3);
    // All but one route point to defaultNhops
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(defaultNhopsId),
        numPostConfigResolvedRoutes(newState) - 1);
    EXPECT_EQ(consolidator_->getRouteUsageCount(newerNhopsId), 1);
  }
  {
    // Update first route back to default nhops
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto routesBefore = fib6->size();
    fib6->updateNode(makeRoute(makePrefix(0), defaultNhops()));
    EXPECT_EQ(fib6->size(), routesBefore);
    consolidate(newState);
    const auto& nhops2Id = consolidator_->getNhopsToId();
    EXPECT_EQ(nhops2Id.size(), 1);
    auto defaultNhopsId = *getNhopId(defaultNhops());
    EXPECT_FALSE(getNhopId(newNhops).has_value());
    EXPECT_FALSE(getNhopId(newerNhops).has_value());
    EXPECT_EQ(defaultNhopsId, 1);
    // All routes point to defaultNhops
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(defaultNhopsId),
        numPostConfigResolvedRoutes(newState));
  }
}

TEST_F(NextHopIdAllocatorTest, updateAllRouteNhops) {
  RouteNextHopSet curNhops = defaultNhops();
  {
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto resolvedRoutesBefore = numPostConfigResolvedRoutes(newState);
    // Previously nhop group Id set was {1} and we updated all 5 routes. Since
    // we account for both before and current nhop  group Ids when allocating
    // next IDs, the IDs generated will be 2 - 11
    std::set<NextHopGroupId> expectedNhopIds{2, 3, 4, 5, 6};
    for (auto i = 0; i < resolvedRoutesBefore; ++i) {
      curNhops.erase(curNhops.begin());
      CHECK(curNhops.size());
      fib6->updateNode(makeRoute(makePrefix(i), curNhops));
    }
    consolidate(newState);
    EXPECT_EQ(getNhopGroupIds(), expectedNhopIds);
  }
  {
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto resolvedRoutesBefore = numPostConfigResolvedRoutes(newState);
    // Previously nhop group Id set was {2, 3, 4, 5, 6}
    // and we updated all 10 routes. Since
    // we account for both before and current nhop  group Ids when allocating
    // next IDs, the IDs generated will be {1, 7 - 10}
    std::set<NextHopGroupId> expectedNhopIds{1, 7, 8, 9, 10};
    for (auto i = 0; i < resolvedRoutesBefore; ++i) {
      curNhops.erase(curNhops.begin());
      CHECK(curNhops.size());
      fib6->updateNode(makeRoute(makePrefix(i), curNhops));
    }
    consolidate(newState);
    EXPECT_EQ(getNhopGroupIds(), expectedNhopIds);
  }
}

TEST_F(NextHopIdAllocatorTest, updateRouteToUnresolved) {
  auto defaultNhopsId = *getNhopId(defaultNhops());
  EXPECT_EQ(defaultNhopsId, 1);
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  auto updatedRoute = fib6->exactMatch(makePrefix(0))->clone();
  updatedRoute->clearForward();
  auto resolvedRoutesBefore = numPostConfigResolvedRoutes(newState);
  // All routes point to defaultNhops
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(defaultNhopsId), resolvedRoutesBefore);
  fib6->updateNode(updatedRoute);
  EXPECT_EQ(fib6->size(), routesBefore);
  consolidate(newState);
  const auto& nhops2Id = consolidator_->getNhopsToId();
  EXPECT_EQ(nhops2Id.size(), 1);
  EXPECT_EQ(*getNhopId(defaultNhops()), defaultNhopsId);
  // All but newly unresolved route point to defaultNhops
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(defaultNhopsId),
      resolvedRoutesBefore - 1);
}

TEST_F(NextHopIdAllocatorTest, updateAndDeleteRoute) {
  auto newNhops = defaultNhops();
  newNhops.erase(newNhops.begin());
  {
    // Update first route to new nhops
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto routesBefore = fib6->size();
    auto resolvedRoutesBefore = numPostConfigResolvedRoutes(newState);
    fib6->updateNode(makeRoute(makePrefix(0), newNhops));
    EXPECT_EQ(fib6->size(), routesBefore);
    consolidate(newState);
    const auto& nhops2Id = consolidator_->getNhopsToId();
    EXPECT_EQ(nhops2Id.size(), 2);
    auto defaultNhopsId = *getNhopId(defaultNhops());
    auto newNhopsId = *getNhopId(newNhops);
    EXPECT_EQ(defaultNhopsId, 1);
    EXPECT_EQ(newNhopsId, 2);
    // All but one route point to defaultNhops
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(defaultNhopsId),
        resolvedRoutesBefore - 1);
    EXPECT_EQ(consolidator_->getRouteUsageCount(newNhopsId), 1);
  }
  {
    // delete route
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto routesBefore = fib6->size();

    fib6->removeNode(makeRoute(makePrefix(0), newNhops));
    EXPECT_EQ(fib6->size(), routesBefore - 1);
    consolidate(newState);
    const auto& nhops2Id = consolidator_->getNhopsToId();
    EXPECT_EQ(nhops2Id.size(), 1);
    auto defaultNhopsId = *getNhopId(defaultNhops());
    EXPECT_FALSE(getNhopId(newNhops).has_value());
    EXPECT_EQ(defaultNhopsId, 1);
    EXPECT_EQ(
        consolidator_->getRouteUsageCount(defaultNhopsId),
        numPostConfigResolvedRoutes(state_));
  }
}

TEST_F(NextHopIdAllocatorTest, deleteRoute) {
  // delete route
  const auto& nhops2Id = consolidator_->getNhopsToId();
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  auto defaultNhopsId = *getNhopId(defaultNhops());
  EXPECT_EQ(defaultNhopsId, 1);
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(defaultNhopsId),
      numPostConfigResolvedRoutes(newState));
  fib6->removeNode(makeRoute(makePrefix(0), defaultNhops()));
  EXPECT_EQ(fib6->size(), routesBefore - 1);
  consolidate(newState);
  EXPECT_EQ(nhops2Id.size(), 1);
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(defaultNhopsId),
      numPostConfigResolvedRoutes(newState));
}

TEST_F(NextHopIdAllocatorTest, deleteAllRoute) {
  // delete route
  const auto& nhops2Id = consolidator_->getNhopsToId();
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto defaultNhopsId = *getNhopId(defaultNhops());
  EXPECT_EQ(
      consolidator_->getRouteUsageCount(defaultNhopsId),
      numPostConfigResolvedRoutes(newState));
  fib6->clear();
  EXPECT_EQ(fib6->size(), 0);
  consolidate(newState);
  EXPECT_EQ(nhops2Id.size(), 0);
  EXPECT_FALSE(getNhopId(defaultNhops()).has_value());
}

TEST_F(NextHopIdAllocatorTest, addRouteAndSetBackupSwitchingMode) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  fib6->addNode(makeRoute(nextPrefix(), defaultNhops()));
  EXPECT_EQ(fib6->size(), routesBefore + 1);
  EXPECT_FALSE(consolidator_->getBackupEcmpSwitchingMode().has_value());
  // Set backup switching mode
  auto newFlowletSwitchingConfig =
      newState->getFlowletSwitchingConfig()->modify(&newState);
  newFlowletSwitchingConfig->setBackupSwitchingMode(
      cfg::SwitchingMode::FIXED_ASSIGNMENT);
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), 1);
  EXPECT_TRUE(consolidator_->getBackupEcmpSwitchingMode().has_value());
}

} // namespace facebook::fboss
