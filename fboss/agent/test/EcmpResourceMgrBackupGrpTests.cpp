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

class EcmpBackupGroupTypeTest : public BaseEcmpResourceManagerTest {
 public:
  std::shared_ptr<EcmpResourceManager> makeResourceMgr() const override {
    static constexpr auto kEcmpGroupHwLimit = 7;
    return std::make_shared<EcmpResourceManager>(
        kEcmpGroupHwLimit,
        0 /*compressionPenaltyThresholdPct*/,
        cfg::SwitchingMode::PER_PACKET_RANDOM);
  }
  static constexpr auto kNumStartRoutes = 5;
  int numStartRoutes() const override {
    return kNumStartRoutes;
  }
  std::vector<RouteNextHopSet> defaultNhopSets() const {
    std::vector<RouteNextHopSet> defaultNhopGroups;
    auto beginNhops = defaultNhops();
    for (auto i = 0; i < numStartRoutes(); ++i) {
      defaultNhopGroups.push_back(beginNhops);
      beginNhops.erase(beginNhops.begin());
      CHECK_GT(beginNhops.size(), 1);
    }
    return defaultNhopGroups;
  }

  std::vector<RouteNextHopSet> nextNhopSets(int numSets = kNumStartRoutes) {
    auto nhopsStart = defaultNhopSets().back();
    std::vector<RouteNextHopSet> nhopsTo;
    for (auto i = 0; i < numSets; ++i) {
      nhopsStart.erase(nhopsStart.begin());
      CHECK_GT(nhopsStart.size(), 1);
      nhopsTo.push_back(nhopsStart);
    }
    return nhopsTo;
  }

  void SetUp() override {
    BaseEcmpResourceManagerTest::SetUp();
    XLOG(DBG2) << "EcmpResourceMgrBackupGrpTest SetUp";
    CHECK(state_->isPublished());
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto newNhops = defaultNhopSets();
    CHECK_EQ(fib6->size(), newNhops.size());
    auto idx = 0;
    for (auto [_, route] : *fib6) {
      auto newRoute = route->clone();
      newRoute->setResolved(
          RouteNextHopEntry(newNhops[idx++], kDefaultAdminDistance));
      newRoute->publish();
      fib6->updateNode(newRoute);
    }
    newState->publish();
    consolidate(newState);
    XLOG(DBG2) << "EcmpResourceMgrBackupGrpTest SetUp done";
  }
  void assertEndState(
      const std::shared_ptr<SwitchState>& endStatePrefixes,
      const std::set<RouteV6::Prefix>& overflowPrefixes) {
    EXPECT_EQ(state_->getFibs()->getNode(RouterID(0))->getFibV4()->size(), 0);
    for (auto [_, inRoute] : std::as_const(*cfib(endStatePrefixes))) {
      auto route = cfib(state_)->exactMatch(inRoute->prefix());
      ASSERT_TRUE(route->isResolved());
      ASSERT_NE(route, nullptr);
      if (overflowPrefixes.find(route->prefix()) != overflowPrefixes.end()) {
        EXPECT_TRUE(
            route->getForwardInfo().getOverrideEcmpSwitchingMode().has_value());
        EXPECT_EQ(
            route->getForwardInfo().getOverrideEcmpSwitchingMode(),
            consolidator_->getBackupEcmpSwitchingMode());
      }
    }
  }
};

TEST_F(EcmpBackupGroupTypeTest, addRoutesBelowEcmpLimit) {
  // add new routes pointing to existing nhops. No limit is thus breached.
  auto nhopSets = defaultNhopSets();
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  for (auto i = 0; i < numStartRoutes(); ++i) {
    auto route = makeRoute(makePrefix(routesBefore + i), nhopSets[i]);
    fib6->addNode(route);
  }
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), 1);
  assertEndState(newState, {});
}

TEST_F(EcmpBackupGroupTypeTest, addRoutesAboveEcmpLimit) {
  // Add new routes pointing to new nhops. ECMP limit is breached.
  auto nhopSets = nextNhopSets();
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  std::set<RouteNextHopSet> nhops;
  std::set<RouteV6::Prefix> overflowPrefixes;
  for (auto i = 0; i < numStartRoutes(); ++i) {
    auto route = makeRoute(makePrefix(routesBefore + i), nhopSets[i]);
    overflowPrefixes.insert(route->prefix());
    nhops.insert(nhopSets[i]);
    fib6->addNode(route);
  }
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), numStartRoutes());
  assertEndState(newState, overflowPrefixes);
}

TEST_F(EcmpBackupGroupTypeTest, updateRouteAboveEcmpLimit) {
  // Update a route pointing to new nhops. ECMP limit is breached.
  auto nhopSets = nextNhopSets();
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto newRoute = fib6->cbegin()->second->clone();
  std::set<RouteV6::Prefix> overflowPrefixes;
  newRoute->setResolved(
      RouteNextHopEntry(*nhopSets.begin(), kDefaultAdminDistance));
  overflowPrefixes.insert(newRoute->prefix());
  fib6->updateNode(newRoute);
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), 1);
  assertEndState(newState, overflowPrefixes);
}
} // namespace facebook::fboss
