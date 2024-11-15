/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"
#include <gtest/gtest.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/utils/AqmTestUtils.h"

namespace facebook::fboss::utility {

int getRoundedBufferThreshold(
    HwSwitch* hwSwitch,
    int expectedThreshold,
    bool roundUp) {
  return getRoundedBufferThreshold(
      hwSwitch->getPlatform()->getAsic(), expectedThreshold, roundUp);
}

size_t getEffectiveBytesPerPacket(HwSwitch* hwSwitch, int pktSizeBytes) {
  return getEffectiveBytesPerPacket(
      hwSwitch->getPlatform()->getAsic(), pktSizeBytes);
}

double getAlphaFromScalingFactor(
    HwSwitch* hwSwitch,
    cfg::MMUScalingFactor scalingFactor) {
  return getAlphaFromScalingFactor(
      hwSwitch->getPlatform()->getAsic(), scalingFactor);
}

uint32_t getQueueLimitBytes(
    HwSwitch* hwSwitch,
    std::optional<cfg::MMUScalingFactor> queueScalingFactor) {
  uint32_t queueLimitBytes{};
  if (queueScalingFactor.has_value()) {
    // Dynamic queue limit for this platform!
    auto alpha =
        getAlphaFromScalingFactor(hwSwitch, queueScalingFactor.value());
    queueLimitBytes = static_cast<uint32_t>(
        getEgressSharedPoolLimitBytes(hwSwitch) * alpha / (1 + alpha));
  } else {
    queueLimitBytes =
        hwSwitch->getPlatform()->getAsic()->getStaticQueueLimitBytes();
  }
  return queueLimitBytes;
}

}; // namespace facebook::fboss::utility
