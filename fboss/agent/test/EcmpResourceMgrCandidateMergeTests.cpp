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

class EcmpResourceMgrCandidateMergeTest
    : public EcmpResourceMgrMergeGroupsTest {};

TEST_F(EcmpResourceMgrCandidateMergeTest, incReference) {
  auto groupInfo = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(0).toCidrNetwork());
  auto nhopsId = sw_->getEcmpResourceManager()
                     ->getNhopsToId()
                     .find(groupInfo->getNhops())
                     ->second;
  auto beforeConsolidationInfo =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
  auto newPrefix = makePrefix(numStartRoutes() + 1);
  addRoute(newPrefix, groupInfo->getNhops());
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
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
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
  auto newPrefix = makePrefix(numStartRoutes() + 1);
  addRoute(newPrefix, groupInfo->getNhops());
  rmRoute(newPrefix);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
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
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
  EXPECT_EQ(beforeConsolidationInfo.size(), 4);
  rmRoute(pfx);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
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
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
  rmRoute(pfx);
  auto afterConsolidationInfo =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsId);
  EXPECT_EQ(afterConsolidationInfo.size(), 0);
  addRoute(pfx, nhops);
  auto newNhopsId =
      sw_->getEcmpResourceManager()->getNhopsToId().find(nhops)->second;

  // Same nhopId would be reused for new group
  EXPECT_EQ(nhopsId, newNhopsId);
  auto afterAddConsolidationInfo =
      sw_->getEcmpResourceManager()->getConsolidationInfo(newNhopsId);
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
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsIdGroup1);
  auto group2Info = sw_->getEcmpResourceManager()->getGroupInfo(
      RouterID(0), makePrefix(1).toCidrNetwork());
  auto nhopsIdGroup2 = sw_->getEcmpResourceManager()
                           ->getNhopsToId()
                           .find(group2Info->getNhops())
                           ->second;
  auto beforeConsolidationInfoGroup2 =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsIdGroup2);
  // First point new prefix to group1
  auto newPrefix = makePrefix(numStartRoutes() + 1);
  addRoute(newPrefix, group1Info->getNhops());
  // Now point new prefix to group 2
  updateRoute(newPrefix, group2Info->getNhops());

  auto afterConsolidationInfoGroup1 =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsIdGroup1);
  auto afterConsolidationInfoGroup2 =
      sw_->getEcmpResourceManager()->getConsolidationInfo(nhopsIdGroup2);
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

} // namespace facebook::fboss
