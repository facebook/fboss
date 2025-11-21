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
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct PfcWdTestConfigs {
  uint32_t detectionTimeMsecs;
  uint32_t recoveryTimeMsecs;
  cfg::PfcWatchdogRecoveryAction recoveryAction;
  std::string description;
};

class HwVerifyPfcConfigInHwTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }

  // Basic config with 2 L3 interface config
  void setupBaseConfig() {
    applyNewConfig(initialConfig());
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

  std::vector<PortID> portIdsForTest() {
    if (FLAGS_hyper_port) {
      return masterLogicalHyperPortIds();
    }
    return masterLogicalInterfacePortIds();
  }

  void addPfcConfig(
      cfg::SwitchConfig& cfg,
      const HwSwitch* /*hwSwitch*/,
      const std::vector<PortID>& ports,
      bool rxEnable = true,
      bool txEnable = true) {
    auto buffer = utility::PfcBufferParams::getPfcBufferParams(getAsicType());
    utility::setupPfcBuffers(
        getHwSwitchEnsemble(), cfg, ports, kLosslessPgs(), {}, {}, buffer);

    for (const auto& portID : ports) {
      auto portCfg = utility::findCfgPort(cfg, portID);
      portCfg->pfc()->tx() = txEnable;
      portCfg->pfc()->rx() = rxEnable;
    }
  }

  std::shared_ptr<SwitchState> setupPfcAndPfcWatchdog(
      cfg::SwitchConfig& currentConfig,
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
    return applyNewConfig(currentConfig);
  }

  // Setup and apply the new config with passed in PFC watchdog config
  void setupPfcWatchdog(
      cfg::SwitchConfig& currentConfig,
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
  void removePfcWatchdogConfig(
      cfg::SwitchConfig& currentConfig,
      const PortID& portId) {
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
    auto setup = [=, this]() {
      auto cfg = initialConfig();
      setupPfcAndPfcWatchdog(cfg, this->portIdsForTest()[0], pfcWatchdogConfig);
    };

    auto verify = [=, this]() {
      utility::pfcWatchdogProgrammingMatchesConfig(
          getHwSwitch(), this->portIdsForTest()[0], true, pfcWatchdogConfig);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Test to verify PFC is not configured in HW
  void runPfcNotConfiguredTest(bool rxEnabled, bool txEnabled) {
    auto setup = [=, this]() { setupBaseConfig(); };

    auto verify = [=, this]() {
      bool pfcRx = false;
      bool pfcTx = false;

      utility::getPfcEnabledStatus(
          getHwSwitch(), this->portIdsForTest()[0], pfcRx, pfcTx);
      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Verify PFC watchdog is not configured in HW
  void verifyPfcWatchdogNotConfigured() {
    auto currentConfig = initialConfig();
    setupPfc(currentConfig, this->portIdsForTest()[0], true, true);
    cfg::PfcWatchdog defaultPfcWatchdogConfig{};

    XLOG(DBG0) << "Verify PFC watchdog is disabled by default on enabling PFC";
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(),
        this->portIdsForTest()[0],
        false,
        defaultPfcWatchdogConfig);
  }

  // Setup and apply the new config with passed in PFC configurations
  void setupPfc(
      cfg::SwitchConfig& currentConfig,
      const PortID& portId,
      const bool pfcRxEnable,
      const bool pfcTxEnable) {
    // setup pfc
    addPfcConfig(
        currentConfig, getHwSwitch(), {portId}, pfcRxEnable, pfcTxEnable);
    applyNewConfig(currentConfig);
  }

  // Removes PFC configuration for port and applies the config
  void removePfcConfig(cfg::SwitchConfig& currentConfig, const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    portCfg->pfc().reset();
    applyNewConfig(currentConfig);
  }

  // Run the various enabled/disabled combinations of PFC RX/TX
  void runPfcTest(bool rxEnabled, bool txEnabled) {
    auto setup = [=, this]() {
      auto currentConfig = initialConfig();
      setupPfc(currentConfig, this->portIdsForTest()[0], rxEnabled, txEnabled);
    };

    auto verify = [=, this]() {
      bool pfcRx = false;
      bool pfcTx = false;

      utility::getPfcEnabledStatus(
          getHwSwitch(), this->portIdsForTest()[0], pfcRx, pfcTx);

      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // Removes PFC configuration for port, but dont apply
  void removePfcConfigSkipApply(
      cfg::SwitchConfig& currentConfig,
      const PortID& portId) {
    auto portCfg = utility::findCfgPort(currentConfig, portId);
    portCfg->pfc().reset();
  }

  void setupPfcWdAndValidateProgramming(
      const PfcWdTestConfigs& wdTestCfg,
      const PortID& portId,
      cfg::SwitchConfig& switchConfig) {
    cfg::PfcWatchdog pfcWatchdogConfig{};
    XLOG(DBG0) << wdTestCfg.description;
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig,
        wdTestCfg.detectionTimeMsecs,
        wdTestCfg.recoveryTimeMsecs,
        wdTestCfg.recoveryAction);
    setupPfcAndPfcWatchdog(switchConfig, portId, pfcWatchdogConfig);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, true, pfcWatchdogConfig);
  }

  std::vector<PfcWdTestConfigs> getPfcWdGranularityTestParam() {
    std::vector<PfcWdTestConfigs> wdParams;
    if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK3 ||
        getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK4) {
      wdParams.push_back(
          {15,
           20,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer range 0-15msec"});
      wdParams.push_back(
          {16,
           20,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer range 16-159msec"});
      wdParams.push_back(
          {159,
           100,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer 10msec range boundary value 159msec"});
      wdParams.push_back(
          {160,
           600,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer 100msec range boundary value 160msec"});
      wdParams.push_back(
          {1599,
           1000,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer 100msec range boundary value 1599msec"});
      wdParams.push_back(
          {1600,
           2000,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer outside range with 1600msec"});
    } else if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      // Chenab ASIC requires at minimum 200ms DLD/ 400ms DLR intervals
      wdParams.push_back(
          {200,
           400,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection boundary value 200ms, recovery boundary value 400ms"});
      wdParams.push_back(
          {1600,
           2000,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer longer value"});
    } else if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) {
      // YUBA supports a min timer value of 25msec and max of 10sec. In the
      // supported range, timer value as a multiple of 25msec is expected.
      wdParams.push_back(
          {25,
           100,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection value 25ms, recovery value 100ms"});
      wdParams.push_back(
          {25,
           1000,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection value 25ms, recovery value 1000ms"});
      wdParams.push_back(
          {100,
           10000,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection value 100ms, recovery value 10s"});
      // Production config
      wdParams.push_back(
          {200,
           1000,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection value 200ms, recovery value 1000ms"});
    } else {
      // TODO: Param combinations for a granularity of 1msec
      wdParams.push_back(
          {10,
           100,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer 10/100 msecs"});
      wdParams.push_back(
          {100,
           500,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer 100/500 msecs"});
      wdParams.push_back(
          {800,
           1000,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog deadlock detection timer 800/1000 msecs"});
    }
    return wdParams;
  }
};

TEST_F(HwVerifyPfcConfigInHwTest, PfcDefaultProgramming) {
  runPfcNotConfiguredTest(false, false);
}

TEST_F(HwVerifyPfcConfigInHwTest, PfcRxDisabledTxDisabled) {
  runPfcTest(false, false);
}

TEST_F(HwVerifyPfcConfigInHwTest, PfcRxEnabledTxDisabled) {
  runPfcTest(true, false);
}

TEST_F(HwVerifyPfcConfigInHwTest, PfcRxDisabledTxEnabled) {
  runPfcTest(false, true);
}

TEST_F(HwVerifyPfcConfigInHwTest, PfcRxEnabledTxEnabled) {
  runPfcTest(true, true);
}

// Try a sequence of configuring, modifying and removing PFC watchdog.
// This test will be retained as a HwTest given there is a lot of programming
// followed by reading back from HW.
TEST_F(HwVerifyPfcConfigInHwTest, PfcWatchdogProgrammingSequence) {
  auto portId = this->portIdsForTest()[0];
  cfg::PfcWatchdog prodPfcWdConfig;
  // The granularity of PFC deadlock timer config in J3 is such that a
  // config of 200 msec results in 198 msec being programmed in HW. To
  // avoid a mismatch between SW and HW, ensure that we program 198 msec
  // so that there is no reprogramming attempt during WB.
  auto pfcDeadlockDetectionInterval =
      getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3 ? 198 : 200;
  initalizePfcConfigWatchdogValues(
      prodPfcWdConfig,
      pfcDeadlockDetectionInterval,
      1000,
      cfg::PfcWatchdogRecoveryAction::NO_DROP);

  auto setup = [&]() {
    setupBaseConfig();
    // Make sure that we start with no PFC configured
    verifyPfcWatchdogNotConfigured();
    auto initialCfg = initialConfig();
    setupPfcAndPfcWatchdog(initialCfg, portId, prodPfcWdConfig);
  };

  auto verify = [&]() {
    // Initially, we should have the prod config applied!
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, true, prodPfcWdConfig);

    bool pfcRx = false;
    bool pfcTx = false;
    cfg::PfcWatchdog defaultPfcWatchdogConfig{};
    auto currentConfig = initialConfig();

    // All PFC WD configuration combinations to test
    std::vector<PfcWdTestConfigs> configTest;
    if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      // Chenab ASIC requires at minimum 200ms DLD/ 400ms DLR intervals
      configTest.push_back(
          {200,
           400,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog is enabled with specified configs"});
      configTest.push_back(
          {250,
           400,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Change just the detection timer and ensure programming"});
      configTest.push_back(
          {250,
           600,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Change just the recovery timer and ensure programming"});
    } else {
      configTest.push_back(
          {5,
           400,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Verify PFC watchdog is enabled with specified configs"});
      configTest.push_back(
          {150,
           400,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Change just the detection timer and ensure programming"});
      configTest.push_back(
          {150,
           200,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Change just the recovery timer and ensure programming"});
    }

    // Enable PFC and PFC wachdog
    for (const auto& wdTestCfg : configTest) {
      setupPfcWdAndValidateProgramming(wdTestCfg, portId, currentConfig);
    }

    XLOG(DBG0) << "Verify removing PFC watchdog removes the programming";
    removePfcWatchdogConfig(currentConfig, portId);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId, false, defaultPfcWatchdogConfig);

    XLOG(DBG0)
        << "Verify removing PFC watchdog does not impact PFC programming";
    utility::getPfcEnabledStatus(getHwSwitch(), portId, pfcRx, pfcTx);
    EXPECT_TRUE(pfcRx);
    EXPECT_TRUE(pfcTx);

    // Granularity tests which is ASIC specific
    for (const auto& wdTestCfg : getPfcWdGranularityTestParam()) {
      setupPfcWdAndValidateProgramming(wdTestCfg, portId, currentConfig);
    }

    // PFC watchdog deadlock config on multiple ports
    auto portId2 = this->portIdsForTest()[1];
    if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      // Chenab ASIC requires at minimum 200ms DLD/ 400ms DLR intervals
      setupPfcWdAndValidateProgramming(
          {200,
           500,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Enable PFC watchdog on more ports and validate programming"},
          portId2,
          currentConfig);
    } else {
      setupPfcWdAndValidateProgramming(
          {140,
           500,
           cfg::PfcWatchdogRecoveryAction::DROP,
           "Enable PFC watchdog on more ports and validate programming"},
          portId2,
          currentConfig);
    }

    XLOG(DBG0) << "Remove PFC watchdog programming on one port and make"
               << " sure watchdog recovery action is not impacted";
    removePfcWatchdogConfig(currentConfig, portId);
    EXPECT_EQ(
        utility::getPfcWatchdogRecoveryAction(getHwSwitch(), portId2),
        cfg::PfcWatchdogRecoveryAction::DROP);

    // Validate PFC WD recovery action being reset to default
    XLOG(DBG0) << "Remove PFC watchdog programming on the remaining port, "
               << "make sure the watchdog recovery action goes to default";
    removePfcWatchdogConfig(currentConfig, portId2);
    utility::pfcWatchdogProgrammingMatchesConfig(
        getHwSwitch(), portId2, false, defaultPfcWatchdogConfig);

    if (!this->getHwSwitchEnsemble()->isSai()) {
      // In SAI implementation, PFC and PFC WD are separate attributes
      // and are independently configured, so unconfiguring PFC will not
      // unconfigure PFC WD.
      if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
        // Chenab ASIC requires at minimum 200ms DLD/ 400ms DLR intervals
        setupPfcWdAndValidateProgramming(
            {200,
             400,
             cfg::PfcWatchdogRecoveryAction::DROP,
             "Enable PFC watchdog config again on the port"},
            portId,
            currentConfig);
      } else {
        setupPfcWdAndValidateProgramming(
            {20,
             100,
             cfg::PfcWatchdogRecoveryAction::DROP,
             "Enable PFC watchdog config again on the port"},
            portId,
            currentConfig);
      }

      // Unconfigure PFC
      XLOG(DBG0)
          << "Verify removing PFC will remove PFC watchdog programming as well";
      removePfcConfig(currentConfig, portId);
      utility::getPfcEnabledStatus(getHwSwitch(), portId, pfcRx, pfcTx);
      EXPECT_FALSE(pfcRx);
      EXPECT_FALSE(pfcTx);
      utility::pfcWatchdogProgrammingMatchesConfig(
          getHwSwitch(), portId, false, defaultPfcWatchdogConfig);
    }

    // Verify PFC deadlock recovery action mismatch across ports
    cfg::PfcWatchdog pfcWatchdogConfig;
    initalizePfcConfigWatchdogValues(
        pfcWatchdogConfig, 300, 900, cfg::PfcWatchdogRecoveryAction::DROP);
    auto state =
        setupPfcAndPfcWatchdog(currentConfig, portId2, pfcWatchdogConfig);
    XLOG(DBG0) << "Enable PFC watchdog with conflicting recovery action "
               << "on anther port and make sure programming did not happen";
    // Applying this config will cause a PFC deadlock recovery action
    // mismatch between ports and will result in FbossError in SwSwitch,
    // In HwTests we call the isValidStateUpdate api directly to verify
    // the check.
    auto newState =
        setupPfcAndPfcWatchdog(currentConfig, portId, prodPfcWdConfig);
    EXPECT_FALSE(
        getHwSwitch()->isValidStateUpdate(StateDelta(state, newState)));
    XLOG(DBG0)
        << "Disable PFC on one port and enable with new recovery action on"
        << " the other port";
    removePfcConfigSkipApply(currentConfig, portId2);

    // At the end, make sure that we configure prod config, so that
    // in WB case, we can ensure the config is as expected post WB!
    setupPfcAndPfcWatchdog(currentConfig, portId, prodPfcWdConfig);
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
