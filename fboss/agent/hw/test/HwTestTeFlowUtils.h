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

bool validateTeFlowGroupEnabled(
    const facebook::fboss::HwSwitch* hw,
    int prefixLength);

int getNumTeFlowEntries(const facebook::fboss::HwSwitch* hw);

void checkSwHwTeFlowMatch(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    TeFlow flow);

} // namespace facebook::fboss::utility
