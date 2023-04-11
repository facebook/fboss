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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/Port.h"

namespace facebook::fboss::utility {

// Gets the PFC enabled/disabled status for RX/TX from HW
void getPfcEnabledStatus(
    const HwSwitch* hw,
    const PortID& portId,
    bool& pfcRx,
    bool& pfcTx);

// Verifies if the PFC watchdog config provided matches the one
// programmed in BCM HW
void pfcWatchdogProgrammingMatchesConfig(
    const HwSwitch* hw,
    const PortID& portId,
    const bool watchdogEnabled,
    const cfg::PfcWatchdog& watchdog);

void runPfcWatchdogGranularityTest(
    const cfg::PfcWatchdog& pfcWatchdogConfig,
    const int expectedGranularity);

int getPfcDeadlockDetectionTimerGranularity(int deadlockDetectionTimeMsec);

int getCosqPFCDeadlockTimerGranularity();

int getProgrammedPfcWatchdogControlParam(
    const HwSwitch* hw,
    const PortID& portId,
    int param);

int getPfcWatchdogRecoveryAction();

// Maps cfg::PfcWatchdogRecoveryAction to switch specific value
int pfcWatchdogRecoveryAction(
    cfg::PfcWatchdogRecoveryAction configuredRecoveryAction);

// Routine to validate if the SW and HW match for the PG cfg
void checkSwHwPgCfgMatch(
    const HwSwitch* hw,
    const std::shared_ptr<Port>& swPort,
    bool pfcEnable);

} // namespace facebook::fboss::utility
