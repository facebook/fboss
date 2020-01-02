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

namespace facebook::fboss {

namespace utility {

uint64_t getCpuQueueOutPackets(HwSwitch* /*hwSwitch*/, int /*queueId*/) {
  // Required APIs not available in opennsl
  return 0;
}

std::map<cfg::PacketRxReason, int16_t> getCoppRxReasonToQueues() {
  return std::map<cfg::PacketRxReason, int16_t>{};
}
} // namespace utility

} // namespace facebook::fboss
