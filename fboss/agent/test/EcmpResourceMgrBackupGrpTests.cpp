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
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto newNhops = defaultNhopSets();
    CHECK_EQ(fib6->size(), newNhops.size());
    auto idx = 0;
    for (auto [_, route] : *fib6) {
      auto newRoute = route->clone();
      newRoute->setResolved(
          RouteNextHopEntry(newNhops[idx++], kDefaultAdminDistance));
      fib6->updateNode(newRoute);
    }
    consolidate(newState);
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
  EXPECT_EQ(*deltas.begin(), StateDelta(oldState, newState));
}

TEST_F(EcmpBackupGroupTypeTest, addRoutesAboveEcmpLimit) {
  // add new routes pointing to existing nhops. No limit is thus breached.
  auto nhopSets = nextNhopSets();
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  std::set<RouteNextHopSet> nhops;
  for (auto i = 0; i < numStartRoutes(); ++i) {
    auto route = makeRoute(makePrefix(routesBefore + i), nhopSets[i]);
    nhops.insert(nhopSets[i]);
    fib6->addNode(route);
  }
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), numStartRoutes());
}
} // namespace facebook::fboss
