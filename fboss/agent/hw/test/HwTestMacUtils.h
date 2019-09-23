/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/state/PortDescriptor.h"

#include <folly/MacAddress.h>

namespace facebook {
namespace fboss {
class HwSwitch;
namespace utility {

uint32_t getMacAgeTimerSeconds(const facebook::fboss::HwSwitch* hwSwitch);
void setMacAgeTimerSeconds(
    const facebook::fboss::HwSwitch* hwSwitch,
    uint32_t seconds);

} // namespace utility
} // namespace fboss
} // namespace facebook
