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

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

namespace facebook::fboss::utility {

std::vector<uint64_t>
getQueueWaterMarks(HwSwitchEnsemble* hw, PortID port, int highestQueueId) {
  std::vector<uint64_t> waterMarks;
  waterMarks.reserve(highestQueueId);
  auto portQueueWaterMarks =
      hw->getLatestPortStats({port})[port].queueWatermarkBytes_;
  for (auto i = 0; i < highestQueueId; ++i) {
    waterMarks.push_back(portQueueWaterMarks[i]);
  }
  return waterMarks;
}

} // namespace facebook::fboss::utility
