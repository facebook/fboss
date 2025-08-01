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

} // namespace facebook::fboss
