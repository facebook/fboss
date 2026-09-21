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

#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss::utility {

HwPortStats getCpuQueueStats(HwSwitch* hwSwitch) {
  HwPortStats portStats;
  auto* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcmSwitch->getControlPlane()->updateQueueCounters(&portStats);
  return portStats;
}

HwPortStats getCpuQueueWatermarkStats(HwSwitch* hwSwitch) {
  HwPortStats portStats;
  auto* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcmSwitch->getControlPlane()->updateQueueWatermarks(&portStats);
  return portStats;
}

} // namespace facebook::fboss::utility
