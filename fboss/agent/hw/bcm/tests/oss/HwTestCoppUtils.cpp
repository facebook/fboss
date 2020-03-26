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

uint64_t getCpuQueueOutPackets(HwSwitch* /*hwSwitch*/, int /*queueId*/) {
  // Required APIs not available in bcm
  return 0;
}

std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues() {
  return std::vector<cfg::PacketRxReasonToQueue>{};
}

} // namespace facebook::fboss::utility
