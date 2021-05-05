/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/types.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

using namespace facebook::fboss;

namespace {
constexpr int kDefaultPfcDeadlockRecoveryTimer = 0;
constexpr int kDefaultPfcDeadlockDetectionTimer = 0;
constexpr int kDefaultPfcDeadlockDetectionAndRecoveryEnable = 0;
constexpr int kDefaultPfcDeadlockTimerGranularity =
    bcmCosqPFCDeadlockTimerInterval1MiliSecond;
} // unnamed namespace

namespace facebook::fboss {
class BcmPfcTests : public BcmTest {
 protected:
  std::vector<int> enabledPfcPriorities_{0, 7};

  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }

  // Basic config with 2 L3 interface config
  void setupBaseConfig() {
    currentConfig = initialConfig();
    applyNewConfig(currentConfig);
  }

  // Initialize a PFC watchdog configuration with passed in params
  void initalizePfcConfigWatchdogValues(
      cfg::PfcWatchdog& watchdog,
      const int detectionTime,
      const int recoveryTime,
      const cfg::PfcWatchdogRecoveryAction recoveryAction) {
    watchdog.recoveryAction_ref() = recoveryAction;
    watchdog.recoveryTimeMsecs_ref() = recoveryTime;
    watchdog.detectionTimeMsecs_ref() = detectionTime;
  }

  // Setup and apply the new config with passed in PFC
  // watchdog config and have PFC RX/TX enabled
  void setupPfcAndPfcWatchdog(
      const PortID& portId,
      const cfg::PfcWatchdog& watchdog) {
    cfg::PortPfc pfc;
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    pfc.tx_ref() = true;
    pfc.rx_ref() = true;
    pfc.watchdog_ref() = watchdog;
    portCfg->pfc_ref() = pfc;
    applyNewConfig(currentConfig);
  }

  // Setup and apply the new config with passed in PFC watchdog config
  void setupPfcWatchdog(
      const PortID& portId,
      const cfg::PfcWatchdog& watchdog) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);

    if (portCfg->pfc_ref().has_value()) {
      cfg::PortPfc pfc = *portCfg->pfc_ref();
      pfc.watchdog_ref() = watchdog;
      portCfg->pfc_ref() = pfc;
    } else {
      XLOG(ERR) << "PFC is not enabled on port " << portId
                << " during PFC watchdog setup!";
    }
    applyNewConfig(currentConfig);
  }

  // Removes the PFC watchdog configuration and applies the same
  void removePfcWatchdogConfig(const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    if (portCfg->pfc_ref().has_value()) {
      portCfg->pfc_ref()->watchdog_ref().reset();
    } else {
      XLOG(ERR) << "PFC is not enabled on port " << portId
                << " during PFC watchdog removal!";
    }
    applyNewConfig(currentConfig);
  }

  // Get the values expected to be programmed in HW for a specific PFC watchdog
  // configuration
  void getExpectedPfcWatchdogHwConfigParams(
      std::map<bcm_cosq_pfc_deadlock_control_t, int>& watchdogPrams,
      bool watchdogEnabled,
      const cfg::PfcWatchdog& watchdogConfig) {
    if (watchdogEnabled) {
      int configuredDetectionTime = *watchdogConfig.detectionTimeMsecs_ref();
      watchdogPrams[bcmCosqPFCDeadlockTimerGranularity] =
          utility::getPfcDeadlockDetectionTimerGranularity(
              configuredDetectionTime);
      watchdogPrams[bcmCosqPFCDeadlockDetectionTimer] =
          utility::getAdjustedPfcDeadlockDetectionTimerValue(
              configuredDetectionTime);
      watchdogPrams[bcmCosqPFCDeadlockRecoveryTimer] =
          *watchdogConfig.recoveryTimeMsecs_ref();
      watchdogPrams[bcmCosqPFCDeadlockDetectionAndRecoveryEnable] = 1;
    } else {
      watchdogPrams[bcmCosqPFCDeadlockTimerGranularity] =
          kDefaultPfcDeadlockTimerGranularity;
      watchdogPrams[bcmCosqPFCDeadlockDetectionTimer] =
          kDefaultPfcDeadlockDetectionTimer;
      watchdogPrams[bcmCosqPFCDeadlockRecoveryTimer] =
          kDefaultPfcDeadlockRecoveryTimer;
      watchdogPrams[bcmCosqPFCDeadlockDetectionAndRecoveryEnable] =
          kDefaultPfcDeadlockDetectionAndRecoveryEnable;
    }
  }

  // Verifies if the PFC watchdog config provided matches the one
  // programmed in BCM HW
  void pfcWatchdogProgrammingMatchesConfig(
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
                         << "]=" << iter->second << ", programmed["
                         << iter->first << "]=" << config2[iter->first];
            }
            EXPECT_EQ(iter->second, config2[iter->first]);
          }
        };

    std::map<bcm_cosq_pfc_deadlock_control_t, int> expectedHwConfig;
    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(portId);
    getExpectedPfcWatchdogHwConfigParams(
        expectedHwConfig, watchdogEnabled, watchdog);
    for (auto pri : enabledPfcPriorities_) {
      std::map<bcm_cosq_pfc_deadlock_control_t, int> readHwConfig;
      bcmPort->getProgrammedPfcWatchdogParams(pri, readHwConfig);
      // Compare the expected params and params read from hardware
      compareExpectedAndProgrammedPfcWatchdogParams(
          expectedHwConfig, readHwConfig);
    }
  }

  // Reads the BCM bcmSwitchPFCDeadlockRecoveryAction from HW
  int getPfcWatchdogRecoveryAction() {
    int value = -1;
    const int unit = 0;
    auto rv = bcm_switch_control_get(
        unit, bcmSwitchPFCDeadlockRecoveryAction, &value);
    bcmCheckError(rv, "Failed to get PFC watchdog recovery action");
    return value;
  }

  // Maps cfg::PfcWatchdogRecoveryAction to BCM specific value
  int pfcWatchdogRecoveryActionToBcm(
      cfg::PfcWatchdogRecoveryAction configuredRecoveryAction) {
    int bcmPfcWatchdogRecoveryAction;
    if (configuredRecoveryAction == cfg::PfcWatchdogRecoveryAction::NO_DROP) {
      bcmPfcWatchdogRecoveryAction = bcmSwitchPFCDeadlockActionTransmit;
    } else {
      bcmPfcWatchdogRecoveryAction = bcmSwitchPFCDeadlockActionDrop;
    }

    return bcmPfcWatchdogRecoveryAction;
  }

  // Gets all the per priority PFC watchdog params programmed in HW
  int getProgrammedPfcWatchdogControlParam(
      const PortID& portId,
      bcm_cosq_pfc_deadlock_control_t param) {
    std::map<bcm_cosq_pfc_deadlock_control_t, int> readHwConfig;
    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(portId);
    // Get the specified param for the first enabled PFC priorities
    bcmPort->getProgrammedPfcWatchdogParams(
        enabledPfcPriorities_.at(0), readHwConfig);
    return readHwConfig[param];
  }

  // Gets the PFC enabled/disabled status for RX/TX from HW
  void getPfcEnabledStatus(const PortID& portId, bool& pfcRx, bool& pfcTx) {
    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(portId);
    int rx = 0;
    int tx = 0;
    bcmPort->getProgrammedPfcState(&rx, &tx);
    pfcRx = rx ? true : false;
    pfcTx = tx ? true : false;
    XLOG(DBG0) << "Read PFC rx " << rx << " tx " << tx;
  }

  // Setup and apply the new config with passed in PFC configurations
  void setupPfc(
      const PortID& portId,
      const bool pfcRxEnable,
      const bool pfcTxEnable) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);

    // setup pfc
    cfg::PortPfc pfc;
    pfc.tx_ref() = pfcTxEnable;
    pfc.rx_ref() = pfcRxEnable;
    portCfg->pfc_ref() = pfc;
    applyNewConfig(currentConfig);
  }

  // Removes PFC configuration for port and applies the config
  void removePfcConfig(const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    portCfg->pfc_ref().reset();
    applyNewConfig(currentConfig);
  }

  // Removes PFC configuration for port, but dont apply
  void removePfcConfigSkipApply(const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    portCfg->pfc_ref().reset();
  }

  // Log enabled/disable status of feature
  std::string enabledStatusToString(bool enabled) {
    return (enabled ? "enabled" : "disabled");
  }

  // Run the various enabled/disabled combinations of PFC RX/TX
  void runPfcTest(bool rxEnabled, bool txEnabled) {
    if (!isSupported(HwAsic::Feature::PFC)) {
      XLOG(WARNING) << "Platform doesn't support PFC";
      return;
    }

    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfc(masterLogicalPortIds()[0], rxEnabled, txEnabled);
    };

    auto verify = [=]() {
      bool pfcRx = false;
      bool pfcTx = false;

      getPfcEnabledStatus(masterLogicalPortIds()[0], pfcRx, pfcTx);
      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Test to verify PFC is not configured in HW
  void runPfcNotConfiguredTest(bool rxEnabled, bool txEnabled) {
    if (!isSupported(HwAsic::Feature::PFC)) {
      XLOG(WARNING) << "Platform doesn't support PFC";
      return;
    }

    auto setup = [=]() { setupBaseConfig(); };

    auto verify = [=]() {
      bool pfcRx = false;
      bool pfcTx = false;

      getPfcEnabledStatus(masterLogicalPortIds()[0], pfcRx, pfcTx);
      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Test to verify PFC watchdog is not configured in HW
  void runPfcWatchdogNotConfiguredTest() {
    if (!isSupported(HwAsic::Feature::PFC)) {
      XLOG(WARNING) << "Platform doesn't support PFC";
      return;
    }

    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfc(masterLogicalPortIds()[0], true, true);
    };

    auto verify = [=]() {
      cfg::PfcWatchdog defaultPfcWatchdogConfig{};

      XLOG(DBG0)
          << "Verify PFC watchdog is disabled by default on enabling PFC";
      pfcWatchdogProgrammingMatchesConfig(
          masterLogicalPortIds()[0], false, defaultPfcWatchdogConfig);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Cross check PFC watchdog HW programming with SW config
  void runPfcWatchdogTest(const cfg::PfcWatchdog& pfcWatchdogConfig) {
    if (!isSupported(HwAsic::Feature::PFC)) {
      XLOG(WARNING) << "Platform doesn't support PFC";
      return;
    }

    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfcAndPfcWatchdog(masterLogicalPortIds()[0], pfcWatchdogConfig);
    };

    auto verify = [=]() {
      pfcWatchdogProgrammingMatchesConfig(
          masterLogicalPortIds()[0], true, pfcWatchdogConfig);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Program PFC watchdog and make sure the detection
  // timer granularity is as expected
  void runPfcWatchdogGranularityTest(
      const cfg::PfcWatchdog& pfcWatchdogConfig,
      const int expectedBcmGranularity) {
    if (!isSupported(HwAsic::Feature::PFC)) {
      XLOG(WARNING) << "Platform doesn't support PFC";
      return;
    }

    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfcAndPfcWatchdog(masterLogicalPortIds()[0], pfcWatchdogConfig);
    };

    auto verify = [=]() {
      auto portId = masterLogicalPortIds()[0];
      pfcWatchdogProgrammingMatchesConfig(portId, true, pfcWatchdogConfig);
      // Explicitly validate granularity!
      EXPECT_EQ(
          getProgrammedPfcWatchdogControlParam(
              portId, bcmCosqPFCDeadlockTimerGranularity),
          expectedBcmGranularity);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  cfg::SwitchConfig currentConfig;
};

TEST_F(BcmPfcTests, PfcDefaultProgramming) {
  runPfcNotConfiguredTest(false, false);
}

TEST_F(BcmPfcTests, PfcRxDisabledTxDisabled) {
  runPfcTest(false, false);
}

TEST_F(BcmPfcTests, PfcRxEnabledTxDisabled) {
  runPfcTest(true, false);
}

TEST_F(BcmPfcTests, PfcRxDisabledTxEnabled) {
  runPfcTest(false, true);
}

TEST_F(BcmPfcTests, PfcRxEnabledTxEnabled) {
  runPfcTest(true, true);
}

TEST_F(BcmPfcTests, PfcWatchdogDefaultProgramming) {
  runPfcWatchdogNotConfiguredTest();
}

TEST_F(BcmPfcTests, PfcWatchdogProgramming) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 1, 400, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogTest(pfcWatchdogConfig);
}

// Try a sequence of configuring, modifying and removing PFC watchdog
TEST_F(BcmPfcTests, PfcWatchdogProgrammingSequence) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [&]() { setupBaseConfig(); };

  auto verify = [&]() {
    bool pfcRx = false;
    bool pfcTx = false;
    cfg::PfcWatchdog pfcWatchdogConfig{};
    auto portId = masterLogicalPortIds()[0];
    cfg::PfcWatchdog defaultPfcWatchdogConfig{};

    // Enable PFC and PFC wachdog
    XLOG(DBG0) << "Verify PFC watchdog is enabled with specified configs";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 1, 400, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId, pfcWatchdogConfig);
    pfcWatchdogProgrammingMatchesConfig(portId, true, pfcWatchdogConfig);

    XLOG(DBG0) << "Change just the detection timer and ensure programming";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 150, 400, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcWatchdog(portId, pfcWatchdogConfig);
    pfcWatchdogProgrammingMatchesConfig(portId, true, pfcWatchdogConfig);

    XLOG(DBG0) << "Change just the recovery timer and ensure programming";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 150, 200, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcWatchdog(portId, pfcWatchdogConfig);
    pfcWatchdogProgrammingMatchesConfig(portId, true, pfcWatchdogConfig);

    XLOG(DBG0) << "Verify removing PFC watchdog removes the programming";
    removePfcWatchdogConfig(portId);
    pfcWatchdogProgrammingMatchesConfig(
        portId, false, defaultPfcWatchdogConfig);

    XLOG(DBG0)
        << "Verify removing PFC watchdog does not impact PFC programming";
    getPfcEnabledStatus(portId, pfcRx, pfcTx);
    EXPECT_TRUE(pfcRx);
    EXPECT_TRUE(pfcTx);

    XLOG(DBG0) << "Verify programming PFC watchdog after unconfiguring works";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 2, 11, cfg::PfcWatchdogRecoveryAction::NO_DROP);
    setupPfcWatchdog(portId, pfcWatchdogConfig);
    pfcWatchdogProgrammingMatchesConfig(portId, true, pfcWatchdogConfig);

    XLOG(DBG0)
        << "Verify removing PFC will remove PFC watchdog programming as well";
    removePfcConfig(portId);
    getPfcEnabledStatus(portId, pfcRx, pfcTx);
    EXPECT_FALSE(pfcRx);
    EXPECT_FALSE(pfcTx);
    pfcWatchdogProgrammingMatchesConfig(
        portId, false, defaultPfcWatchdogConfig);
  };

  // The test fails warmboot as there are reconfigurations done in verify
  setup();
  verify();
}

// Validate PFC watchdog deadlock detection timer in the 0-16msec range
TEST_F(BcmPfcTests, PfcWatchdogGranularity1) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection timer range 0-15msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 15, 20, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, bcmCosqPFCDeadlockTimerInterval1MiliSecond);
}

// Validate PFC watchdog deadlock detection timer in the 16-160msec range
TEST_F(BcmPfcTests, PfcWatchdogGranularity2) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection timer range 16-159msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 16, 20, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, bcmCosqPFCDeadlockTimerInterval10MiliSecond);
}

// Validate PFC watchdog deadlock detection timer in the 16-160msec range
TEST_F(BcmPfcTests, PfcWatchdogGranularity3) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer 10msec range boundary value 159msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 159, 100, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, bcmCosqPFCDeadlockTimerInterval10MiliSecond);
}

// Validate PFC watchdog deadlock detection timer in the 160-1599msec range
TEST_F(BcmPfcTests, PfcWatchdogGranularity4) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer 100msec range boundary value 160msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 160, 600, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, bcmCosqPFCDeadlockTimerInterval100MiliSecond);
}

// Validate PFC watchdog deadlock detection timer in the 160-1599msec range
TEST_F(BcmPfcTests, PfcWatchdogGranularity5) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer 100msec range boundary value 1599msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 1599, 1000, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, bcmCosqPFCDeadlockTimerInterval100MiliSecond);
}

// Validate PFC watchdog deadlock detection timer outside the 0-1599msec range
TEST_F(BcmPfcTests, PfcWatchdogGranularity6) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer outside range with 1600msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 1600, 2000, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, bcmCosqPFCDeadlockTimerInterval100MiliSecond);
}

// PFC watchdog deadlock recovery action tests
TEST_F(BcmPfcTests, PfcWatchdogDeadlockRecoveryAction) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [&]() { setupBaseConfig(); };

  auto verify = [&]() {
    auto portId1 = masterLogicalPortIds()[0];
    auto portId2 = masterLogicalPortIds()[1];
    cfg::PfcWatchdog pfcWatchdogConfig{};

    XLOG(DBG0)
        << "Verify PFC watchdog recovery action is configured as expected";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 15, 20, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId1, pfcWatchdogConfig);
    pfcWatchdogProgrammingMatchesConfig(portId1, true, pfcWatchdogConfig);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            *pfcWatchdogConfig.recoveryAction_ref()));

    XLOG(DBG0) << "Enable PFC watchdog on more ports and validate programming";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 140, 500, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId2, pfcWatchdogConfig);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            *pfcWatchdogConfig.recoveryAction_ref()));

    XLOG(DBG0) << "Remove PFC watchdog programming on one port and make"
               << " sure watchdog recovery action is not impacted";
    removePfcWatchdogConfig(portId1);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(cfg::PfcWatchdogRecoveryAction::DROP));

    XLOG(DBG0) << "Remove PFC watchdog programming on the remaining port, "
               << "make sure the watchdog recovery action goes to default";
    removePfcWatchdogConfig(portId2);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            cfg::PfcWatchdogRecoveryAction::NO_DROP));

    XLOG(DBG0) << "Enable PFC watchdog config again on the port";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 20, 100, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId2, pfcWatchdogConfig);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            *pfcWatchdogConfig.recoveryAction_ref()));

    XLOG(DBG0) << "Unconfigure PFC on port and make sure PFC "
               << "watchdog recovery action is reverted to default";
    removePfcConfig(portId2);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            cfg::PfcWatchdogRecoveryAction::NO_DROP));
  };
  // The test fails warmboot as there are reconfigurations done in verify
  setup();
  verify();
}

// Verify all ports should be configured with the same
// PFC watchdog deadlock recovery action.
TEST_F(BcmPfcTests, PfcWatchdogDeadlockRecoveryActionMismatch) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [&]() { setupBaseConfig(); };

  auto verify = [&]() {
    auto portId1 = masterLogicalPortIds()[0];
    auto portId2 = masterLogicalPortIds()[1];
    cfg::PfcWatchdog pfcWatchdogConfig{};
    cfg::PfcWatchdog defaultPfcWatchdogConfig{};

    XLOG(DBG0) << "Enable PFC watchdog and make sure programming is fine";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 1000, 3000, cfg::PfcWatchdogRecoveryAction::NO_DROP);
    setupPfcAndPfcWatchdog(portId1, pfcWatchdogConfig);
    pfcWatchdogProgrammingMatchesConfig(portId1, true, pfcWatchdogConfig);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            *pfcWatchdogConfig.recoveryAction_ref()));

    XLOG(DBG0) << "Enable PFC watchdog with conflicting recovery action "
               << "on anther port and make sure programming did not happen";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 1200, 6000, cfg::PfcWatchdogRecoveryAction::DROP);
    /*
     * Applying this config will cause a PFC deadlock recovery action
     * mismatch between ports and will result in FbossError!
     */
    EXPECT_THROW(
        setupPfcAndPfcWatchdog(portId2, pfcWatchdogConfig), FbossError);

    // Make sure the watchdog programming is still as previously configured
    pfcWatchdogProgrammingMatchesConfig(
        portId2, false, defaultPfcWatchdogConfig);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            cfg::PfcWatchdogRecoveryAction::NO_DROP));

    /*
     * Remove PFC watchdog from the first port and enable PFC watchdog with
     * new recovery action on the second port and make sure programming is fine
     */
    XLOG(DBG0)
        << "Disable PFC on one and enable with new action on the other port";
    removePfcConfigSkipApply(portId1);
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 1200, 6000, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId2, pfcWatchdogConfig);
    pfcWatchdogProgrammingMatchesConfig(portId2, true, pfcWatchdogConfig);
    EXPECT_EQ(
        getPfcWatchdogRecoveryAction(),
        pfcWatchdogRecoveryActionToBcm(
            *pfcWatchdogConfig.recoveryAction_ref()));
  };
  // The test fails warmboot as there are reconfigurations done in verify
  setup();
  verify();
}

} // namespace facebook::fboss
