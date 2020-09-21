/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTestCoppUtils.h"

#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss::utility {

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* /*hwSwitch*/,
    int /*queueId*/) {
  // Required APIs not available in bcm
  return std::make_pair(0, 0);
}

std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues(
    const HwAsic* /* hwAsic */) {
  return std::vector<cfg::PacketRxReasonToQueue>{};
}

} // namespace facebook::fboss::utility
