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

#include <memory>
#include <string>

extern "C" {
#include <bcm/switch.h>
}

namespace facebook::fboss {
class BcmSwitch;
class SwitchState;

namespace utility {

void checkSwHwAclMatch(
    BcmSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName);

void assertSwitchControl(bcm_switch_control_t type, int expectedValue);
} // namespace utility
} // namespace facebook::fboss
