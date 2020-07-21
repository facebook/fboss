/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

namespace facebook::fboss {

namespace utility {

uint32_t getMacAgeTimerSeconds(const facebook::fboss::HwSwitch* hwSwitch) {
  auto& switchMgr =
      static_cast<const SaiSwitch*>(hwSwitch)->managerTable()->switchManager();
  return switchMgr.getMacAgingSeconds();
}

void setMacAgeTimerSeconds(
    facebook::fboss::HwSwitch* hwSwitch,
    uint32_t seconds) {
  auto& switchMgr =
      static_cast<SaiSwitch*>(hwSwitch)->managerTable()->switchManager();
  return switchMgr.setMacAgingSeconds(seconds);
}

} // namespace utility

} // namespace facebook::fboss
