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

class EcmpResourceMgrCandidateMergeTest
    : public BaseEcmpResourceMgrMergeGroupsTest {
 public:
  int maxPenalty(const EcmpResourceManager::NextHopGroupIds& groups) const {
    auto consolidationInfo =
        sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
            *groups.begin());
    return consolidationInfo.find(groups)->second.maxPenalty();
  }
};

TEST_F(EcmpResourceMgrCandidateMergeTest, incReference) {
  auto groupInfo = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(0).toCidrNetwork());
  auto nhopsId = sw_->getEcmpResourceManager()
                     ->getNhopsToId()
                     .find(groupInfo->getNhops())
                     ->second;
  auto beforeConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  auto newPrefix = makePrefix(numStartRoutes() + 1);
  addRoute(newPrefix, groupInfo->getNhops());
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  EXPECT_EQ(beforeConsolidationInfo.size(), afterConsolidationInfo.size());
  for (const auto& [groupIds, consolidationInfo] : beforeConsolidationInfo) {
    auto newConsolidationInfo = afterConsolidationInfo.find(groupIds)->second;
    EXPECT_EQ(
        2 * consolidationInfo.groupId2Penalty.find(nhopsId)->second,
        newConsolidationInfo.groupId2Penalty.find(nhopsId)->second);
  }
}

TEST_F(EcmpResourceMgrCandidateMergeTest, incDecReference) {
  auto groupInfo = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(0).toCidrNetwork());
  auto nhopsId = sw_->getEcmpResourceManager()
                     ->getNhopsToId()
                     .find(groupInfo->getNhops())
                     ->second;
  auto beforeConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  auto newPrefix = makePrefix(numStartRoutes() + 1);
  addRoute(newPrefix, groupInfo->getNhops());
  rmRoute(newPrefix);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  EXPECT_EQ(beforeConsolidationInfo, afterConsolidationInfo);
}

TEST_F(EcmpResourceMgrCandidateMergeTest, deleteNhopGroup) {
  auto pfx = makePrefix(0);
  auto groupInfo = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), pfx.toCidrNetwork());

  auto nhopsId = sw_->getEcmpResourceManager()
                     ->getNhopsToId()
                     .find(groupInfo->getNhops())
                     ->second;
  auto beforeConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  EXPECT_EQ(beforeConsolidationInfo.size(), 4);
  rmRoute(pfx);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  EXPECT_EQ(afterConsolidationInfo.size(), 0);
}

TEST_F(EcmpResourceMgrCandidateMergeTest, deleteAndAddNhopGroup) {
  auto pfx = makePrefix(0);
  auto groupInfo = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), pfx.toCidrNetwork());
  auto nhops = groupInfo->getNhops();

  auto nhopsId = sw_->getEcmpResourceManager()
                     ->getNhopsToId()
                     .find(groupInfo->getNhops())
                     ->second;
  auto beforeConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  rmRoute(pfx);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsId);
  EXPECT_EQ(afterConsolidationInfo.size(), 0);
  addRoute(pfx, nhops);
  auto newNhopsId =
      sw_->getEcmpResourceManager()->getNhopsToId().find(nhops)->second;

  // Same nhopId would be reused for new group
  EXPECT_EQ(nhopsId, newNhopsId);
  auto afterAddConsolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          newNhopsId);
  EXPECT_EQ(beforeConsolidationInfo, afterAddConsolidationInfo);
}

TEST_F(EcmpResourceMgrCandidateMergeTest, updateRouteNhops) {
  auto group1Info = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(0).toCidrNetwork());
  auto nhopsIdGroup1 = sw_->getEcmpResourceManager()
                           ->getNhopsToId()
                           .find(group1Info->getNhops())
                           ->second;
  auto beforeConsolidationInfoGroup1 =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsIdGroup1);
  auto group2Info = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(1).toCidrNetwork());
  auto nhopsIdGroup2 = sw_->getEcmpResourceManager()
                           ->getNhopsToId()
                           .find(group2Info->getNhops())
                           ->second;
  auto beforeConsolidationInfoGroup2 =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsIdGroup2);
  // First point new prefix to group1
  auto newPrefix = makePrefix(numStartRoutes() + 1);
  addRoute(newPrefix, group1Info->getNhops());
  // Now point new prefix to group 2
  updateRoute(newPrefix, group2Info->getNhops());

  auto afterConsolidationInfoGroup1 =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsIdGroup1);
  auto afterConsolidationInfoGroup2 =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsIdGroup2);
  // before and after should be equal for  group 1 - modulo merge with group2
  // (since group2 ref count got updated to 2)
  for (auto [groupIds, consolidationInfo] : beforeConsolidationInfoGroup1) {
    auto newConsolidationInfo =
        afterConsolidationInfoGroup1.find(groupIds)->second;
    // Erase nhopsIdGroup2 from penalties, since we don't want
    // to compare its penalty
    newConsolidationInfo.groupId2Penalty.erase(nhopsIdGroup2);
    consolidationInfo.groupId2Penalty.erase(nhopsIdGroup2);
    EXPECT_EQ(consolidationInfo, newConsolidationInfo);
  }

  // After penalty should double for group2
  for (const auto& [groupIds, consolidationInfo] :
       beforeConsolidationInfoGroup2) {
    auto newConsolidationInfo =
        afterConsolidationInfoGroup2.find(groupIds)->second;
    EXPECT_EQ(
        2 * consolidationInfo.groupId2Penalty.find(nhopsIdGroup2)->second,
        newConsolidationInfo.groupId2Penalty.find(nhopsIdGroup2)->second);
  }
}

TEST_F(EcmpResourceMgrCandidateMergeTest, updateAndDeleteRouteNhops) {
  auto group1Info = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(0).toCidrNetwork());
  auto nhopsIdGroup1 = sw_->getEcmpResourceManager()
                           ->getNhopsToId()
                           .find(group1Info->getNhops())
                           ->second;
  auto group2Info = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(1).toCidrNetwork());
  auto nhopsIdGroup2 = sw_->getEcmpResourceManager()
                           ->getNhopsToId()
                           .find(group2Info->getNhops())
                           ->second;
  auto beforeConsolidationInfoGroup2 =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsIdGroup2);
  // Point prefix 0 to group2. This will cause nhop group1 to get
  // deleted, since its ref count dropped to 0
  updateRoute(makePrefix(0), group2Info->getNhops());

  auto afterConsolidationInfoGroup1 =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsIdGroup1);
  EXPECT_TRUE(afterConsolidationInfoGroup1.empty());
  auto afterConsolidationInfoGroup2 =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
          nhopsIdGroup2);

  // After penalty should double for group2
  for (const auto& [groupIds, consolidationInfo] :
       beforeConsolidationInfoGroup2) {
    if (groupIds.contains(nhopsIdGroup1)) {
      EXPECT_FALSE(afterConsolidationInfoGroup2.contains(groupIds));
      continue;
    }
    auto newConsolidationInfo =
        afterConsolidationInfoGroup2.find(groupIds)->second;
    EXPECT_EQ(
        2 * consolidationInfo.groupId2Penalty.find(nhopsIdGroup2)->second,
        newConsolidationInfo.groupId2Penalty.find(nhopsIdGroup2)->second);
  }
}

TEST_F(EcmpResourceMgrCandidateMergeTest, maxPenalty) {
  auto consolidationInfo =
      sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(5);
  EXPECT_EQ(maxPenalty({1, 5}), 20);
  EXPECT_EQ(maxPenalty({2, 5}), 16);
  EXPECT_EQ(maxPenalty({3, 5}), 12);
  EXPECT_EQ(maxPenalty({4, 5}), 6);
}

TEST_F(EcmpResourceMgrCandidateMergeTest, optimalMergeSet) {
  auto optimalMergeSet =
      sw_->getEcmpResourceManager()->getOptimalMergeGroupSet();
  EXPECT_EQ(optimalMergeSet, EcmpResourceManager::NextHopGroupIds({1, 2}));
}
} // namespace facebook::fboss
