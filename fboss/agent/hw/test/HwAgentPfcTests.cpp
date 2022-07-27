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

class HwAgentPfcTests : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
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

  // Test to verify PFC is not configured in HW
  void runPfcNotConfiguredTest(bool rxEnabled, bool txEnabled) {
    auto setup = [=]() { setupBaseConfig(); };

    auto verify = [=]() {
      bool pfcRx = false;
      bool pfcTx = false;

      utility::getPfcEnabledStatus(
          getHwSwitch(), masterLogicalPortIds()[0], pfcRx, pfcTx);
      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
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

  // Run the various enabled/disabled combinations of PFC RX/TX
  void runPfcTest(bool rxEnabled, bool txEnabled) {
    auto setup = [=]() {
      currentConfig = initialConfig();
      setupPfc(masterLogicalPortIds()[0], rxEnabled, txEnabled);
    };

    auto verify = [=]() {
      bool pfcRx = false;
      bool pfcTx = false;

      utility::getPfcEnabledStatus(
          getHwSwitch(), masterLogicalPortIds()[0], pfcRx, pfcTx);

      EXPECT_EQ(pfcRx, rxEnabled);
      EXPECT_EQ(pfcTx, txEnabled);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  cfg::SwitchConfig currentConfig;
};

TEST_F(HwAgentPfcTests, PfcDefaultProgramming) {
  runPfcNotConfiguredTest(false, false);
}

TEST_F(HwAgentPfcTests, PfcRxDisabledTxDisabled) {
  runPfcTest(false, false);
}

TEST_F(HwAgentPfcTests, PfcRxEnabledTxDisabled) {
  runPfcTest(true, false);
}

TEST_F(HwAgentPfcTests, PfcRxDisabledTxEnabled) {
  runPfcTest(false, true);
}

TEST_F(HwAgentPfcTests, PfcRxEnabledTxEnabled) {
  runPfcTest(true, true);
}

} // namespace facebook::fboss
