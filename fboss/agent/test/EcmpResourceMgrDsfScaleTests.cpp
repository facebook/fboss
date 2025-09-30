/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"
#include "fboss/agent/test/utils/EcmpResourceManagerTestUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss {
namespace {
auto constexpr kMaxHwEcmpGroups = 16;
auto constexpr kMaxRoutes = 50;
auto constexpr kAdjacentNextHopDeltaSize = 4;
auto constexpr kEcmpWidth = 2048;
std::unique_ptr<EcmpResourceManager> makeResourceMgr() {
  return std::make_unique<EcmpResourceManager>(
      kMaxHwEcmpGroups, 100 /*compressionPenaltyThresholdPct*/);
}
} // namespace
class EcmpResourceManagerDsfScaleTest : public ::testing::Test {
 public:
  int numStartRoutes() const {
    return kMaxHwEcmpGroups -
        FLAGS_ecmp_resource_manager_make_before_break_buffer;
  }
  RouteNextHopSet getNhops(int index) const {
    auto nhopStart = index * kAdjacentNextHopDeltaSize;
    CHECK_LT(nhopStart + kEcmpWidth, allNextHops_.size());
    return RouteNextHopSet(
        allNextHops_.begin() + nhopStart,
        allNextHops_.begin() + nhopStart + kEcmpWidth);
  }
  void SetUp() override;
  std::vector<StateDelta> consolidate(
      const std::shared_ptr<SwitchState>& newState);
  std::unique_ptr<EcmpResourceManager> ecmpResourceMgr_;
  std::shared_ptr<SwitchState> state_;
  RouteNextHopSet allNextHops_;
};

std::vector<StateDelta> EcmpResourceManagerDsfScaleTest::consolidate(
    const std::shared_ptr<SwitchState>& newState) {
  auto deltas = ecmpResourceMgr_->consolidate(StateDelta(state_, newState));
  state_ = deltas.back().newState();
  ecmpResourceMgr_->updateDone();
  assertResourceMgrCorrectness(*ecmpResourceMgr_, state_);
  auto newEcmpResourceMgr = makeResourceMgr();
  newEcmpResourceMgr->reconstructFromSwitchState(state_);
  assertResourceMgrCorrectness(*newEcmpResourceMgr, state_);

  return deltas;
}

void EcmpResourceManagerDsfScaleTest::SetUp() {
  FLAGS_ecmp_width = kEcmpWidth;
  ecmpResourceMgr_ = makeResourceMgr();
  state_ = std::make_shared<SwitchState>();
  addSwitchInfo(state_);
  state_ = setupMinAlpmRouteState(state_);
  state_->publish();
  allNextHops_ =
      makeNextHops(kEcmpWidth + kMaxRoutes * kAdjacentNextHopDeltaSize);
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (auto i = 0; i < numStartRoutes(); ++i) {
    fib6->addNode(makeRoute(makePrefix(i), getNhops(i)));
  }
  consolidate(newState);
}

TEST_F(EcmpResourceManagerDsfScaleTest, init) {}

TEST_F(EcmpResourceManagerDsfScaleTest, addOneRouteOverEcmpLimit) {
  auto optimalMergeSet = ecmpResourceMgr_->getOptimalMergeGroupSet();
  EXPECT_EQ(optimalMergeSet.size(), 2);
  auto toBeMergedPrefixes =
      getPrefixesForGroups(*ecmpResourceMgr_, optimalMergeSet);
  EXPECT_EQ(toBeMergedPrefixes.size(), 2);
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  fib6->addNode(
      makeRoute(makePrefix(numStartRoutes()), getNhops(numStartRoutes())));
  consolidate(newState);
  assertNumRoutesWithNhopOverrides(state_, toBeMergedPrefixes.size());
}

TEST_F(EcmpResourceManagerDsfScaleTest, addMaxScaleRoutesOverEcmpLimit) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (auto i = numStartRoutes(); i < kMaxRoutes; ++i) {
    fib6->addNode(makeRoute(makePrefix(i), getNhops(i)));
  }
  consolidate(newState);
  auto mergedGids = ecmpResourceMgr_->getMergedGids();
  EXPECT_GT(mergedGids.size(), 0);
  assertNumRoutesWithNhopOverrides(
      state_, getPrefixesForGroups(*ecmpResourceMgr_, mergedGids).size());
}

TEST_F(EcmpResourceManagerDsfScaleTest, addRemoveMaxScaleRoutes) {
  std::vector<RoutePrefixV6> addedRoutes;
  {
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    for (auto i = numStartRoutes(); i < kMaxRoutes; ++i) {
      addedRoutes.emplace_back(makePrefix(i));
      fib6->addNode(makeRoute(addedRoutes.back(), getNhops(i)));
    }
    consolidate(newState);
  }
  {
    // remote routes;
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    for (const auto& pfx : addedRoutes) {
      fib6->removeNode(pfx.str());
    }
    consolidate(newState);
  }
  EXPECT_EQ(ecmpResourceMgr_->getMergedGids().size(), 0);
}
} // namespace facebook::fboss
