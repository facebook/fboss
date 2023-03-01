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

#include "fboss/agent/HwSwitch.h"

namespace facebook::fboss::utility {

bool validateFlowletSwitchingEnabled(
    const facebook::fboss::HwSwitch* hw,
    const cfg::FlowletSwitchingConfig& flowletCfg);

bool verifyEcmpForFlowletSwitching(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& routePrefix,
    const cfg::FlowletSwitchingConfig& flowletCfg);

} // namespace facebook::fboss::utility
