/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestWatermarkUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include <folly/logging/xlog.h>

extern "C" {
#include <bcm/cosq.h>
}

namespace facebook::fboss::utility {

std::vector<uint64_t>
getQueueWaterMarks(HwSwitchEnsemble* hw, PortID port, int highestQueueId) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw->getHwSwitch());
  auto rv = bcm_cosq_bst_stat_sync(
      bcmSwitch->getUnit(), (bcm_bst_stat_id_t)bcmBstStatIdUcast);
  bcmCheckError(rv, "failed to sync BST stats");
  auto portGport = bcmSwitch->getPortTable()->getBcmPort(port)->getBcmGport();
  std::vector<uint64_t> waterMarks;
  waterMarks.reserve(highestQueueId);
  for (auto cosq = 0; cosq <= highestQueueId; cosq++) {
    uint64_t value;
    uint32_t options = BCM_COSQ_STAT_CLEAR;
    auto rv = bcm_cosq_bst_stat_get(
        bcmSwitch->getUnit(),
        portGport,
        cosq,
        (bcm_bst_stat_id_t)bcmBstStatIdUcast,
        BCM_COSQ_STAT_CLEAR,
        &value);
    bcmCheckError(rv, "Failed to get BST stat");
    waterMarks.push_back(value);
  }
  return waterMarks;
}

} // namespace facebook::fboss::utility
