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

#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss {

namespace utility {

HwPortStats getCpuQueueStats(HwSwitch* hwSwitch) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  saiSwitch->updateStats();
  return saiSwitch->managerTable()->hostifManager().getCpuPortStats();
}

HwPortStats getCpuQueueWatermarkStats(HwSwitch* hwSwitch) {
  // CPU Queue stats include watermark stats as well
  return getCpuQueueStats(hwSwitch);
}

} // namespace utility
} // namespace facebook::fboss
