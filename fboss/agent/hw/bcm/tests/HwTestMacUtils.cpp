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

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <bcm/l2.h>
}

namespace facebook::fboss::utility {

uint32_t getMacAgeTimerSeconds(const facebook::fboss::HwSwitch* hwSwitch) {
  auto unit =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)->getUnit();
  int seconds{0};
  auto rv = bcm_l2_age_timer_get(unit, &seconds);
  bcmCheckError(rv, "Unable to get mac age timer value");
  return seconds;
}

void setMacAgeTimerSeconds(
    facebook::fboss::HwSwitch* hwSwitch,
    uint32_t seconds) {
  auto unit =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)->getUnit();
  auto rv = bcm_l2_age_timer_set(unit, seconds);
  bcmCheckError(rv, "Unable to set mac age timer value");
}

} // namespace facebook::fboss::utility
