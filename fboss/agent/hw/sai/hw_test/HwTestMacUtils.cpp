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

namespace facebook::fboss {

namespace utility {

uint32_t getMacAgeTimerSeconds(const facebook::fboss::HwSwitch* /*hwSwitch*/) {
  // TBD
  return 0;
}

void setMacAgeTimerSeconds(
    const facebook::fboss::HwSwitch* /*hwSwitch*/,
    uint32_t /*seconds*/) {
  // TBD
}

} // namespace utility

} // namespace facebook::fboss
