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
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <folly/IPAddress.h>

#include <string>

extern "C" {
#include <bcm/port.h>
#include <bcm/switch.h>
}

namespace facebook::fboss {
class BcmSwitch;
class SwitchState;

} // namespace facebook::fboss
/*
 * This utility is to provide utils for bcm test.
 */
namespace facebook::fboss::utility {
void checkSwHwAclMatch(
    BcmSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName);
void addMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    const cfg::MatchAction& matchAction);
void assertSwitchControl(bcm_switch_control_t type, int expectedValue);

} // namespace facebook::fboss::utility
