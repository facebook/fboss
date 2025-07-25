/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/EcmpResourceMgrMergeGroupsTests.h"

namespace facebook::fboss {

std::vector<RouteNextHopSet> EcmpResourceMgrMergeGroupsTest::defaultNhopSets()
    const {
  std::vector<RouteNextHopSet> defaultNhopGroups;
  auto beginNhops = defaultNhops();
  for (auto i = 0; i < numStartRoutes(); ++i) {
    defaultNhopGroups.push_back(beginNhops);
    beginNhops.erase(beginNhops.begin());
    CHECK_GT(beginNhops.size(), 1);
  }
  return defaultNhopGroups;
}

void EcmpResourceMgrMergeGroupsTest::setupFlags() const {
  FLAGS_enable_ecmp_resource_manager = true;
  FLAGS_ecmp_resource_percentage = 100;
}

void EcmpResourceMgrMergeGroupsTest::SetUp() {
  BaseEcmpResourceManagerTest::SetUp();
  XLOG(DBG2) << "EcmpResourceMgrMergeGroupsTest SetUp";
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
        sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsGrpId);
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
    }
  }
  XLOG(DBG2) << "EcmpResourceMgrBackupGrpTest SetUp done";
}
TEST_F(EcmpResourceMgrMergeGroupsTest, init) {}

TEST_F(EcmpResourceMgrMergeGroupsTest, reloadInvalidConfigs) {
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
