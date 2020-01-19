/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmTestBstUtils.h"
#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

namespace facebook::fboss::utility {

namespace {

void verifyBst(
    BcmSwitch* hw,
    bcm_port_t port,
    const std::vector<int>& trafficQueueIds,
    bool bstEnabled) {
  auto queueBstStats = utility::clearAndGetAllBstStats(
      hw->getUnit(),
      port,
      *(std::max_element(trafficQueueIds.begin(), trafficQueueIds.end())));

  for (const auto& queueBstStat : queueBstStats) {
    auto queueId = queueBstStat.first;
    auto bstStatVal = queueBstStat.second;

    XLOG(DBG0) << "queueId: " << queueId << " BST stat value: " << bstStatVal;

    // If BST is enabled, and if the queueId is one of the queue ids that can
    // process traffic (e.g. a WRR queue may not process traffic if starved by
    // SP), then the BST stat value will be > 0, otherwise 0.
    EXPECT_TRUE(
        bstEnabled &&
                std::find(
                    trafficQueueIds.begin(), trafficQueueIds.end(), queueId) !=
                    trafficQueueIds.end()
            ? bstStatVal > 0
            : bstStatVal == 0);
  }
}

} // namespace

void verifyBstStatsReported(
    BcmSwitch* hw,
    bcm_port_t port,
    const std::vector<int>& trafficQueueIds) {
  // BST does not work as expected with 6.4.10 (Broadcom case: CS5938424)
  XLOG(DBG0) << "BST enabled:";
  hw->getBstStatsMgr()->startBufferStatCollection();
  verifyBst(hw, port, trafficQueueIds, true /* bstEnabled */);

  XLOG(DBG0) << "BST disabled:";
  hw->getBstStatsMgr()->stopBufferStatCollection();
  verifyBst(hw, port, trafficQueueIds, false /* bstEnabled */);
}

} // namespace facebook::fboss::utility
