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
#include <gtest/gtest.h>
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/types.h"

namespace {

constexpr int kDefaultPfcDeadlockDetectionAndRecoveryEnable = 0;
constexpr int kDefaultPfcDeadlockTimerGranularity =
    bcmCosqPFCDeadlockTimerInterval1MiliSecond;

constexpr int kPfcDeadlockDetectionTimerLimit = 15;

/*
 * XXX: This is to be removed in a future commit, consolidating
 * PFC config additions to a single API for migration to
 * utility::addPfcConfig(), which will be integrated in one of
 * the next commits. Both kLosslessPgs and addPfcConfig will
 * need to be replaced with utility:: equivalents.
 */
std::vector<int> kLosslessPgs() {
  return {0, 7};
}
} // namespace

namespace facebook::fboss::utility {

// Gets the PFC enabled/disabled status for RX/TX from HW
void getPfcEnabledStatus(
    const HwSwitch* hw,
    const PortID& portId,
    bool& pfcRx,
    bool& pfcTx) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto bcmPort = bcmSwitch->getPortTable()->getBcmPort(portId);
  int rx = 0;
  int tx = 0;
  bcmPort->getProgrammedPfcState(&rx, &tx);
  pfcRx = rx ? true : false;
  pfcTx = tx ? true : false;
  XLOG(DBG0) << "Read PFC rx " << rx << " tx " << tx;
}

// Get the values expected to be programmed in HW for a specific PFC watchdog
// configuration
void getExpectedPfcWatchdogHwConfigParams(
    const HwSwitch* hwSwitch,
    std::map<bcm_cosq_pfc_deadlock_control_t, int>& watchdogPrams,
    bool watchdogEnabled,
    const cfg::PfcWatchdog& watchdogConfig) {
  auto asicType = hwSwitch->getPlatform()->getAsic()->getAsicType();
  if (watchdogEnabled) {
    int configuredDetectionTime = *watchdogConfig.detectionTimeMsecs();
    watchdogPrams[bcmCosqPFCDeadlockTimerGranularity] =
        utility::getPfcDeadlockDetectionTimerGranularity(
            configuredDetectionTime);
    watchdogPrams[bcmCosqPFCDeadlockDetectionTimer] =
        utility::getAdjustedPfcDeadlockDetectionTimerValue(
            configuredDetectionTime);
    watchdogPrams[bcmCosqPFCDeadlockRecoveryTimer] =
        utility::getAdjustedPfcDeadlockRecoveryTimerValue(
            asicType, *watchdogConfig.recoveryTimeMsecs());
    watchdogPrams[bcmCosqPFCDeadlockDetectionAndRecoveryEnable] = 1;
  } else {
    watchdogPrams[bcmCosqPFCDeadlockTimerGranularity] =
        kDefaultPfcDeadlockTimerGranularity;
    watchdogPrams[bcmCosqPFCDeadlockDetectionTimer] =
        utility::getDefaultPfcDeadlockDetectionTimer(asicType);
    watchdogPrams[bcmCosqPFCDeadlockRecoveryTimer] =
        utility::getDefaultPfcDeadlockRecoveryTimer(asicType);
    watchdogPrams[bcmCosqPFCDeadlockDetectionAndRecoveryEnable] =
        kDefaultPfcDeadlockDetectionAndRecoveryEnable;
  }
}

// Verifies if the PFC watchdog config provided matches the one
// programmed in BCM HW
void pfcWatchdogProgrammingMatchesConfig(
    const HwSwitch* hw,
    const PortID& portId,
    const bool watchdogEnabled,
    const cfg::PfcWatchdog& watchdog) {
  auto compareExpectedAndProgrammedPfcWatchdogParams =
      [&](std::map<bcm_cosq_pfc_deadlock_control_t, int>& config1,
          std::map<bcm_cosq_pfc_deadlock_control_t, int>& config2) {
        EXPECT_EQ(config1.size(), config2.size());
        for (auto iter = config1.begin(); iter != config1.end(); ++iter) {
          // The same keys are expected in both the maps
          if (iter->second != config2[iter->first]) {
            XLOG(DBG0) << "PFC watchdog mismatch: expected[" << iter->first
                       << "]=" << iter->second << ", programmed[" << iter->first
                       << "]=" << config2[iter->first];
          }
          EXPECT_EQ(iter->second, config2[iter->first]);
        }
      };

  std::map<bcm_cosq_pfc_deadlock_control_t, int> expectedHwConfig;
  auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto bcmPort = bcmSwitch->getPortTable()->getBcmPort(portId);
  getExpectedPfcWatchdogHwConfigParams(
      hw, expectedHwConfig, watchdogEnabled, watchdog);
  for (auto pri : kLosslessPgs()) {
    std::map<bcm_cosq_pfc_deadlock_control_t, int> readHwConfig;
    bcmPort->getProgrammedPfcWatchdogParams(pri, readHwConfig);
    // Compare the expected params and params read from hardware
    compareExpectedAndProgrammedPfcWatchdogParams(
        expectedHwConfig, readHwConfig);
  }
}

int getPfcDeadlockDetectionTimerGranularity(int deadlockDetectionTimeMsec) {
  /*
   * BCM can configure a value 0-15 with a granularity of 1msec,
   * 10msec, 100msec as the PfcDeadlockDetectionTime, selecting
   * the granularity based on the attempted configuration.
   */
  int granularity{};

  if (deadlockDetectionTimeMsec <= kPfcDeadlockDetectionTimerLimit) {
    granularity = bcmCosqPFCDeadlockTimerInterval1MiliSecond;
  } else if (
      deadlockDetectionTimeMsec / 10 <= kPfcDeadlockDetectionTimerLimit) {
    granularity = bcmCosqPFCDeadlockTimerInterval10MiliSecond;
  } else {
    granularity = bcmCosqPFCDeadlockTimerInterval100MiliSecond;
  }
  return granularity;
}

int getCosqPFCDeadlockTimerGranularity() {
  return bcmCosqPFCDeadlockTimerGranularity;
}

// Gets all the per priority PFC watchdog params programmed in HW
int getProgrammedPfcWatchdogControlParam(
    const HwSwitch* hw,
    const PortID& portId,
    int param) {
  auto bcmParam = static_cast<bcm_cosq_pfc_deadlock_control_t>(param);
  std::map<bcm_cosq_pfc_deadlock_control_t, int> readHwConfig;
  auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto bcmPort = bcmSwitch->getPortTable()->getBcmPort(portId);
  // Get the specified param for the first enabled PFC priorities
  bcmPort->getProgrammedPfcWatchdogParams(kLosslessPgs().at(0), readHwConfig);
  return readHwConfig[bcmParam];
}

// Reads the BCM bcmSwitchPFCDeadlockRecoveryAction from HW
int getPfcWatchdogRecoveryAction() {
  int value = -1;
  const int unit = 0;
  auto rv =
      bcm_switch_control_get(unit, bcmSwitchPFCDeadlockRecoveryAction, &value);
  bcmCheckError(rv, "Failed to get PFC watchdog recovery action");
  return value;
}

// Maps cfg::PfcWatchdogRecoveryAction to BCM specific value
int pfcWatchdogRecoveryAction(
    cfg::PfcWatchdogRecoveryAction configuredRecoveryAction) {
  int bcmPfcWatchdogRecoveryAction;
  if (configuredRecoveryAction == cfg::PfcWatchdogRecoveryAction::NO_DROP) {
    bcmPfcWatchdogRecoveryAction = bcmSwitchPFCDeadlockActionTransmit;
  } else {
    bcmPfcWatchdogRecoveryAction = bcmSwitchPFCDeadlockActionDrop;
  }

  return bcmPfcWatchdogRecoveryAction;
}

} // namespace facebook::fboss::utility
