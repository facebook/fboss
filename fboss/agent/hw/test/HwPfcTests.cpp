/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <string>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPfcUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwPfcTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
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
    watchdog.recoveryAction() = recoveryAction;
    watchdog.recoveryTimeMsecs() = recoveryTime;
    watchdog.detectionTimeMsecs() = detectionTime;
  }

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

  void addPfcConfig(
      cfg::SwitchConfig& cfg,
      const HwSwitch* /*hwSwitch*/,
      const std::vector<PortID>& ports,
      bool rxEnable = true,
      bool txEnable = true) {
    cfg::PortPfc pfc;
    pfc.tx() = txEnable;
    pfc.rx() = rxEnable;
    pfc.portPgConfigName() = "foo";

    for (const auto& portID : ports) {
      auto portCfg = utility::findCfgPort(cfg, portID);
      portCfg->pfc() = pfc;
    }

    // Copied from enablePgConfigConfig
    std::vector<cfg::PortPgConfig> portPgConfigs;
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    for (const auto& pgId : kLosslessPgs()) {
      cfg::PortPgConfig pgConfig;
      pgConfig.id() = pgId;
      pgConfig.bufferPoolName() = "bufferNew";
      // provide atleast 1 cell worth of minLimit
      pgConfig.minLimitBytes() = 300;
      portPgConfigs.emplace_back(pgConfig);
    }

    // create buffer pool
    std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
    cfg::BufferPoolConfig poolConfig;
    poolConfig.sharedBytes() = 10000;
    bufferPoolCfgMap.insert(std::make_pair("bufferNew", poolConfig));
    currentConfig.bufferPoolConfigs() = bufferPoolCfgMap;

    portPgConfigMap.insert({"foo", portPgConfigs});
    cfg.portPgConfigs() = portPgConfigMap;
  }

  void setupPfcAndPfcWatchdog(
      const PortID& portId,
      const cfg::PfcWatchdog& watchdog) {
    addPfcConfig(
        currentConfig,
        getHwSwitch(),
        {portId},
        true /* RX enable */,
        true /* TX enable */);
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    if (portCfg->pfc().has_value()) {
      portCfg->pfc()->watchdog() = watchdog;
    } else {
      XLOG(ERR) << "PFC is not enabled on port " << portId
                << " during PFC and watchdog setup!";
    }
    applyNewConfig(currentConfig);
  }

  // Setup and apply the new config with passed in PFC watchdog config
  void setupPfcWatchdog(
      const PortID& portId,
      const cfg::PfcWatchdog& watchdog) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);

    if (portCfg->pfc().has_value()) {
      portCfg->pfc()->watchdog() = watchdog;
    } else {
      XLOG(ERR) << "PFC is not enabled on port " << portId
                << " during PFC watchdog setup!";
    }
    applyNewConfig(currentConfig);
  }

  // Removes the PFC watchdog configuration and applies the same
  void removePfcWatchdogConfig(const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    if (portCfg->pfc().has_value()) {
      portCfg->pfc()->watchdog().reset();
    } else {
      XLOG(ERR) << "PFC is not enabled on port " << portId
                << " during PFC watchdog removal!";
    }
    applyNewConfig(currentConfig);
  }

  // Cross check PFC watchdog HW programming with SW config
  void runPfcWatchdogTest(const cfg::PfcWatchdog& pfcWatchdogConfig) {
    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfcAndPfcWatchdog(
          masterLogicalInterfacePortIds()[0], pfcWatchdogConfig);
    };

    auto verify = [=]() {
      utility::pfcWatchdogProgrammingMatchesConfig(
          getHwSwitch(),
          masterLogicalInterfacePortIds()[0],
          true,
          pfcWatchdogConfig);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Test to verify PFC is not configured in HW
  void runPfcNotConfiguredTest(bool rxEnabled, bool txEnabled) {
    auto setup = [=]() { setupBaseConfig(); };

    auto verify = [=]() {
      bool pfcRx = false;
      bool pfcTx = false;

      utility::getPfcEnabledStatus(
          getHwSwitch(), masterLogicalInterfacePortIds()[0], pfcRx, pfcTx);
      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Test to verify PFC watchdog is not configured in HW
  void runPfcWatchdogNotConfiguredTest() {
    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfc(masterLogicalInterfacePortIds()[0], true, true);
    };

    auto verify = [=]() {
      cfg::PfcWatchdog defaultPfcWatchdogConfig{};

      XLOG(DBG0)
          << "Verify PFC watchdog is disabled by default on enabling PFC";
      utility::pfcWatchdogProgrammingMatchesConfig(
          getHwSwitch(),
          masterLogicalInterfacePortIds()[0],
          false,
          defaultPfcWatchdogConfig);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Setup and apply the new config with passed in PFC configurations
  void setupPfc(
      const PortID& portId,
      const bool pfcRxEnable,
      const bool pfcTxEnable) {
    // setup pfc
    addPfcConfig(
        currentConfig, getHwSwitch(), {portId}, pfcRxEnable, pfcTxEnable);
    applyNewConfig(currentConfig);
  }

  // Removes PFC configuration for port and applies the config
  void removePfcConfig(const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    portCfg->pfc().reset();
    applyNewConfig(currentConfig);
  }

  // Run the various enabled/disabled combinations of PFC RX/TX
  void runPfcTest(bool rxEnabled, bool txEnabled) {
    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfc(masterLogicalInterfacePortIds()[0], rxEnabled, txEnabled);
    };

    auto verify = [=]() {
      bool pfcRx = false;
      bool pfcTx = false;

      utility::getPfcEnabledStatus(
          getHwSwitch(), masterLogicalInterfacePortIds()[0], pfcRx, pfcTx);

      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Program PFC watchdog and make sure the detection
  // timer granularity is as expected
  void runPfcWatchdogGranularityTest(
      const cfg::PfcWatchdog& pfcWatchdogConfig,
      const int expectedBcmGranularity) {
    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfcAndPfcWatchdog(
          masterLogicalInterfacePortIds()[0], pfcWatchdogConfig);
    };

    auto verify = [=]() {
      auto portId = masterLogicalInterfacePortIds()[0];
      utility::pfcWatchdogProgrammingMatchesConfig(
          getHwSwitch(), portId, true, pfcWatchdogConfig);
      // Explicitly validate granularity!
      EXPECT_EQ(
          utility::getProgrammedPfcWatchdogControlParam(
              getHwSwitch(),
              portId,
              utility::getCosqPFCDeadlockTimerGranularity()),
          expectedBcmGranularity);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Removes PFC configuration for port, but dont apply
  void removePfcConfigSkipApply(const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    portCfg->pfc().reset();
  }

 private:
  cfg::SwitchConfig currentConfig;
};

TEST_F(HwPfcTest, PfcDefaultProgramming) {
  runPfcNotConfiguredTest(false, false);
}

TEST_F(HwPfcTest, PfcRxDisabledTxDisabled) {
  runPfcTest(false, false);
}

TEST_F(HwPfcTest, PfcRxEnabledTxDisabled) {
  runPfcTest(true, false);
}

TEST_F(HwPfcTest, PfcRxDisabledTxEnabled) {
  runPfcTest(false, true);
}

TEST_F(HwPfcTest, PfcRxEnabledTxEnabled) {
  runPfcTest(true, true);
}

TEST_F(HwPfcTest, PfcWatchdogDefaultProgramming) {
  runPfcWatchdogNotConfiguredTest();
}

TEST_F(HwPfcTest, PfcWatchdogProgramming) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 1, 400, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogTest(pfcWatchdogConfig);
}

// Try a sequence of configuring, modifying and removing PFC watchdog
TEST_F(HwPfcTest, PfcWatchdogProgrammingSequence) {
  auto setup = [&]() { setupBaseConfig(); };

  auto verify = [&]() {
    bool pfcRx = false;
    bool pfcTx = false;
    cfg::PfcWatchdog pfcWatchdogConfig{};
    auto portId = masterLogicalInterfacePortIds()[0];
    cfg::PfcWatchdog defaultPfcWatchdogConfig{};

    // Enable PFC and PFC wachdog
    XLOG(DBG0) << "Verify PFC watchdog is enabled with specified configs";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 1, 400, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId, pfcWatchdogConfig);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, true, pfcWatchdogConfig);

    XLOG(DBG0) << "Change just the detection timer and ensure programming";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 150, 400, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcWatchdog(portId, pfcWatchdogConfig);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, true, pfcWatchdogConfig);

    XLOG(DBG0) << "Change just the recovery timer and ensure programming";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 150, 200, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcWatchdog(portId, pfcWatchdogConfig);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, true, pfcWatchdogConfig);

    XLOG(DBG0) << "Verify removing PFC watchdog removes the programming";
    removePfcWatchdogConfig(portId);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, false, defaultPfcWatchdogConfig);

    XLOG(DBG0)
        << "Verify removing PFC watchdog does not impact PFC programming";
    utility::getPfcEnabledStatus(getHwSwitch(), portId, pfcRx, pfcTx);
    EXPECT_TRUE(pfcRx);
    EXPECT_TRUE(pfcTx);

    XLOG(DBG0) << "Verify programming PFC watchdog after unconfiguring works";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 2, 11, cfg::PfcWatchdogRecoveryAction::NO_DROP);
    setupPfcWatchdog(portId, pfcWatchdogConfig);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, true, pfcWatchdogConfig);

    XLOG(DBG0)
        << "Verify removing PFC will remove PFC watchdog programming as well";
    removePfcConfig(portId);
    utility::getPfcEnabledStatus(getHwSwitch(), portId, pfcRx, pfcTx);
    EXPECT_FALSE(pfcRx);
    EXPECT_FALSE(pfcTx);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, false, defaultPfcWatchdogConfig);
  };

  // The test fails warmboot as there are reconfigurations done in verify
  setup();
  verify();
}

// Validate PFC watchdog deadlock detection timer in the 0-16msec range
TEST_F(HwPfcTest, PfcWatchdogGranularity1) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection timer range 0-15msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 15, 20, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, utility::getPfcDeadlockDetectionTimerGranularity(15));
}

// Validate PFC watchdog deadlock detection timer in the 16-160msec range
TEST_F(HwPfcTest, PfcWatchdogGranularity2) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection timer range 16-159msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 16, 20, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, utility::getPfcDeadlockDetectionTimerGranularity(16));
}

// Validate PFC watchdog deadlock detection timer in the 16-160msec range
TEST_F(HwPfcTest, PfcWatchdogGranularity3) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer 10msec range boundary value 159msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 159, 100, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, utility::getPfcDeadlockDetectionTimerGranularity(159));
}

// Validate PFC watchdog deadlock detection timer in the 160-1599msec range
TEST_F(HwPfcTest, PfcWatchdogGranularity4) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer 100msec range boundary value 160msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 160, 600, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig, utility::getPfcDeadlockDetectionTimerGranularity(160));
}

// Validate PFC watchdog deadlock detection timer in the 160-1599msec range
TEST_F(HwPfcTest, PfcWatchdogGranularity5) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer 100msec range boundary value 1599msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 1599, 1000, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig,
      utility::getPfcDeadlockDetectionTimerGranularity(1599));
}

// Validate PFC watchdog deadlock detection timer outside the 0-1599msec
// range
TEST_F(HwPfcTest, PfcWatchdogGranularity6) {
  cfg::PfcWatchdog pfcWatchdogConfig{};
  XLOG(DBG0) << "Verify PFC watchdog deadlock detection "
             << "timer outside range with 1600msec";
  initalizePfcConfigWatchdogValues(
      pfcWatchdogConfig, 1600, 2000, cfg::PfcWatchdogRecoveryAction::DROP);
  runPfcWatchdogGranularityTest(
      pfcWatchdogConfig,
      utility::getPfcDeadlockDetectionTimerGranularity(1600));
}

// PFC watchdog deadlock recovery action tests
TEST_F(HwPfcTest, PfcWatchdogDeadlockRecoveryAction) {
  auto setup = [&]() { setupBaseConfig(); };

  auto verify = [&]() {
    auto portId1 = masterLogicalInterfacePortIds()[0];
    auto portId2 = masterLogicalInterfacePortIds()[1];
    cfg::PfcWatchdog pfcWatchdogConfig{};

    XLOG(DBG0)
        << "Verify PFC watchdog recovery action is configured as expected";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 15, 20, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId1, pfcWatchdogConfig);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId1, true, pfcWatchdogConfig);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            *pfcWatchdogConfig.recoveryAction()));

    XLOG(DBG0) << "Enable PFC watchdog on more ports and validate programming";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 140, 500, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId2, pfcWatchdogConfig);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            *pfcWatchdogConfig.recoveryAction()));

    XLOG(DBG0) << "Remove PFC watchdog programming on one port and make"
               << " sure watchdog recovery action is not impacted";
    removePfcWatchdogConfig(portId1);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            cfg::PfcWatchdogRecoveryAction::DROP));

    XLOG(DBG0) << "Remove PFC watchdog programming on the remaining port, "
               << "make sure the watchdog recovery action goes to default";
    removePfcWatchdogConfig(portId2);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            cfg::PfcWatchdogRecoveryAction::NO_DROP));

    XLOG(DBG0) << "Enable PFC watchdog config again on the port";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 20, 100, cfg::PfcWatchdogRecoveryAction::DROP);
    setupPfcAndPfcWatchdog(portId2, pfcWatchdogConfig);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            *pfcWatchdogConfig.recoveryAction()));

    XLOG(DBG0) << "Unconfigure PFC on port and make sure PFC "
               << "watchdog recovery action is reverted to default";
    removePfcConfig(portId2);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            cfg::PfcWatchdogRecoveryAction::NO_DROP));
  };
  // The test fails warmboot as there are reconfigurations done in verify
  setup();
  verify();
}

// Verify all ports should be configured with the same
// PFC watchdog deadlock recovery action.
TEST_F(HwPfcTest, PfcWatchdogDeadlockRecoveryActionMismatch) {
  auto setup = [&]() { setupBaseConfig(); };

  auto verify = [&]() {
    auto portId1 = masterLogicalInterfacePortIds()[0];
    auto portId2 = masterLogicalInterfacePortIds()[1];
    cfg::PfcWatchdog pfcWatchdogConfig{};
    cfg::PfcWatchdog defaultPfcWatchdogConfig{};

    XLOG(DBG0) << "Enable PFC watchdog and make sure programming is fine";
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 1000, 3000, cfg::PfcWatchdogRecoveryAction::NO_DROP);
    setupPfcAndPfcWatchdog(portId1, pfcWatchdogConfig);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId1, true, pfcWatchdogConfig);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            *pfcWatchdogConfig.recoveryAction()));

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
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId2, false, defaultPfcWatchdogConfig);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
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
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId2, true, pfcWatchdogConfig);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(),
        utility::pfcWatchdogRecoveryAction(
            *pfcWatchdogConfig.recoveryAction()));
  };
  // The test fails warmboot as there are reconfigurations done in verify
  setup();
  verify();
}

} // namespace facebook::fboss
