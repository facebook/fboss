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

std::vector<RouteNextHopSet>
BaseEcmpResourceMgrMergeGroupsTest::defaultNhopSets() const {
  std::vector<RouteNextHopSet> defaultNhopGroups;
  auto beginNhops = defaultNhops();
  for (auto i = 0; i < numStartRoutes(); ++i) {
    defaultNhopGroups.push_back(beginNhops);
    beginNhops.erase(beginNhops.begin());
    CHECK_GT(beginNhops.size(), 1);
  }
  return defaultNhopGroups;
}

std::vector<RouteNextHopSet> BaseEcmpResourceMgrMergeGroupsTest::nextNhopSets(
    int numSets) const {
  auto nhopsCur = defaultNhopSets().back();
  std::vector<RouteNextHopSet> nhopsTo;
  auto allNhops = sw_->getEcmpResourceManager()->getNhopsToId();
  while (nhopsTo.size() < numSets) {
    CHECK_GT(nhopsCur.size(), 2);
    nhopsCur.erase(nhopsCur.begin());
    if (!allNhops.contains(nhopsCur)) {
      nhopsTo.push_back(nhopsCur);
    }
  }
  return nhopsTo;
}

void BaseEcmpResourceMgrMergeGroupsTest::setupFlags() const {
  FLAGS_enable_ecmp_resource_manager = true;
  FLAGS_ecmp_resource_percentage = 100;
}

void BaseEcmpResourceMgrMergeGroupsTest::SetUp() {
  BaseEcmpResourceManagerTest::SetUp();
  XLOG(DBG2) << "BaseEcmpResourceMgrMergeGroupsTest SetUp";
  CHECK(state_->isPublished());
  auto newNhops = defaultNhopSets();
  CHECK_EQ(getPostConfigResolvedRoutes(state_).size(), newNhops.size());
  auto idx = 0;
  for (const auto& route : getPostConfigResolvedRoutes(state_)) {
    CHECK(state_->isPublished());
    auto newState = state_->clone();
    auto fib6 = fib(newState);
    auto newRoute = route->clone();
    newRoute->setResolved(
        RouteNextHopEntry(newNhops[idx++], kDefaultAdminDistance));
    newRoute->publish();
    fib6->updateNode(newRoute);
    newState->publish();
    consolidate(newState);
  }
  const auto& nhops2Id = sw_->getEcmpResourceManager()->getNhopsToId();
  auto expectedGrpId = 1;
  for (const auto& nhops : defaultNhopSets()) {
    auto nhopsGrpId = nhops2Id.find(nhops)->second;
    EXPECT_EQ(expectedGrpId++, nhopsGrpId);
    auto grpIdsToConsolidationInfo =
        sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
            nhopsGrpId);
    for (EcmpResourceManager::NextHopGroupId otherGrpId = 1;
         otherGrpId <= defaultNhopSets().size();
         ++otherGrpId) {
      EcmpResourceManager::NextHopGroupIds mergeGroupIds{
          nhopsGrpId, otherGrpId};
      if (nhopsGrpId == otherGrpId) {
        CHECK(!grpIdsToConsolidationInfo.contains(mergeGroupIds));
        continue;
      }
      // For any grp Ids x, y - just assert for penalties when
      // x < y, since there is just a single entry for any x,y
      // in the candidate merge groups - i.e. for the set {x, y}
      // and not 2 distinct entries for x,y and y, x. Merging
      // x, y is the same as merging y, x
      if (nhopsGrpId > otherGrpId) {
        continue;
      }
      auto consolidationInfo =
          grpIdsToConsolidationInfo.find(mergeGroupIds)->second;
      int nhopsLost = otherGrpId - nhopsGrpId;
      XLOG(DBG2) << " Between grps: " << folly::join(", ", mergeGroupIds)
                 << " lost: " << nhopsLost;
      EXPECT_EQ(consolidationInfo.mergedNhops.size(), nhops.size() - nhopsLost);
      EXPECT_EQ(consolidationInfo.groupId2Penalty.find(otherGrpId)->second, 0);
      auto largerGroupPenalty = std::ceil(nhopsLost * 100.0 / nhops.size());
      EXPECT_EQ(
          consolidationInfo.groupId2Penalty.find(nhopsGrpId)->second,
          largerGroupPenalty);
    }
  }
  XLOG(DBG2) << "EcmpResourceMgrBackupGrpTest SetUp done";
}

std::vector<StateDelta> BaseEcmpResourceMgrMergeGroupsTest::addNextRoute() {
  auto nhopSets = nextNhopSets(1);
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto newRoute = makeRoute(nextPrefix(), *nhopSets.begin())->clone();
  newRoute->setResolved(
      RouteNextHopEntry(*nhopSets.begin(), kDefaultAdminDistance));
  fib6->addNode(newRoute);
  return consolidate(newState);
}

TEST_F(BaseEcmpResourceMgrMergeGroupsTest, init) {}

TEST_F(BaseEcmpResourceMgrMergeGroupsTest, reloadInvalidConfigs) {
  {
    // Both compression threshold and backup group type set
    auto newCfg = onePortPerIntfConfig(
        getEcmpCompressionThresholdPct(),
        cfg::SwitchingMode::PER_PACKET_RANDOM);
    EXPECT_THROW(sw_->applyConfig("Invalid config", newCfg), FbossError);
  }
  {
    // Changing compression threshold is not allowed
    auto newCfg = onePortPerIntfConfig(
        getEcmpCompressionThresholdPct() + 42, getBackupEcmpSwitchingMode());
    EXPECT_THROW(sw_->applyConfig("Invalid config", newCfg), FbossError);
  }
}
} // namespace facebook::fboss
