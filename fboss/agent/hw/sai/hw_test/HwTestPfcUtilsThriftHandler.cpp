/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestPfcUtils.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

bool HwTestThriftHandler::getPfcEnabled(int32_t portId, bool rx) {
  bool pfcRx = false;
  bool pfcTx = false;
  ::facebook::fboss::utility::getPfcEnabledStatus(
      hwSwitch_, PortID(portId), pfcRx, pfcTx);
  return rx ? pfcRx : pfcTx;
}

bool HwTestThriftHandler::pfcWatchdogProgrammingMatchesConfig(
    int32_t portId,
    bool watchdogEnabled,
    std::unique_ptr<cfg::PfcWatchdog> watchdog) {
  return ::facebook::fboss::utility::pfcWatchdogProgrammingMatchesConfig(
      hwSwitch_, PortID(portId), watchdogEnabled, *watchdog);
}

int32_t HwTestThriftHandler::getPfcWatchdogRecoveryAction(int32_t portId) {
  return static_cast<int32_t>(
      ::facebook::fboss::utility::getPfcWatchdogRecoveryAction(
          hwSwitch_, PortID(portId)));
}

} // namespace facebook::fboss::utility
