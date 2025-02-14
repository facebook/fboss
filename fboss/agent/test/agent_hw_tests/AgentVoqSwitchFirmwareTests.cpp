// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/lib/CommonUtils.h"

// For Netcastle runs, netcastle sets up db directory under
// /tmp and the isolation firmware is placed under that dir
// When running by hand, we can either mimic this or override the
// location via a command line argument
DEFINE_string(
    isolation_firmware_path,
    "/tmp/db/jericho3ai_a0/fi-2.4.0.1-GA.elf",
    "Path where isolation FW is placed");

namespace facebook::fboss {

class AgentVoqSwitchIsolationFirmwareTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true, /*interfaceHasSubnet*/
        true, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    utility::populatePortExpectedNeighborsToSelf(
        ensemble.masterLogicalPortIds(), config);
    /*
     * Clear FW config since only one FW config is supported today. We
     * started adding the default FW config in the config template so
     * all tests load the FW. However for these FW tests, we add more
     * custom config. So clear out the default config so we can prepare
     * afresh.
     */
    config = clearFWConfig(config);
    config = addFwConfig(config);
    return config;
  }

 protected:
  cfg::SwitchConfig clearFWConfig(cfg::SwitchConfig config) const {
    auto fwCapableSwitchIndices = getFWCapableSwitchIndices(config);
    auto& switchSettings = *config.switchSettings();
    for (auto& [_, switchInfo] : *switchSettings.switchIdToSwitchInfo()) {
      if (fwCapableSwitchIndices.find(*switchInfo.switchIndex()) !=
          fwCapableSwitchIndices.end()) {
        switchInfo.firmwareNameToFirmwareInfo()->clear();
      }
    }
    return config;
  }
  cfg::SwitchConfig addFwConfig(cfg::SwitchConfig config) const {
    cfg::FirmwareInfo j3FwInfo;
    j3FwInfo.coreToUse() = 5;
    j3FwInfo.path() = FLAGS_isolation_firmware_path;
    j3FwInfo.logPath() = "/tmp/edk.log";
    j3FwInfo.firmwareLoadType() =
        cfg::FirmwareLoadType::FIRMWARE_LOAD_TYPE_START;
    auto fwCapableSwitchIndices = getFWCapableSwitchIndices(config);
    auto& switchSettings = *config.switchSettings();
    for (auto& [_, switchInfo] : *switchSettings.switchIdToSwitchInfo()) {
      if (fwCapableSwitchIndices.find(*switchInfo.switchIndex()) !=
          fwCapableSwitchIndices.end()) {
        switchInfo.firmwareNameToFirmwareInfo()->insert(
            std::make_pair("isolationFirmware", j3FwInfo));
      }
    }
    return config;
  }
  std::set<uint16_t> getFWCapableSwitchIndices(
      const cfg::SwitchConfig& config) const {
    std::set<uint16_t> switchIndices;
    auto& switchSettings = *config.switchSettings();
    for (auto& [id, switchInfo] : *switchSettings.switchIdToSwitchInfo()) {
      if (switchInfo.asicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
        switchIndices.insert(*switchInfo.switchIndex());
      }
    }
    return switchIndices;
  }
  void setMinLinksConfig() {
    auto config = getSw()->getConfig();
    auto& switchSettings = *config.switchSettings();
    switchSettings.minLinksToJoinVOQDomain() =
        ceil(masterLogicalFabricPortIds().size() * 0.66);
    switchSettings.minLinksToRemainInVOQDomain() =
        *switchSettings.minLinksToJoinVOQDomain() -
        masterLogicalFabricPortIds().size() / 8;
    applyNewConfig(config);
  }
  void assertSwitchDrainState(bool expectDrained) const {
    auto expectDrainState = expectDrained
        ? cfg::SwitchDrainState::DRAINED_DUE_TO_ASIC_ERROR
        : cfg::SwitchDrainState::UNDRAINED;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      // reachability should always be there regardless of drain state
      utility::checkFabricConnectivity(getAgentEnsemble(), switchId);
      HwSwitchMatcher matcher(std::unordered_set<SwitchID>({switchId}));
      WITH_RETRIES({
        const auto& switchSettings =
            getProgrammedState()->getSwitchSettings()->getSwitchSettings(
                matcher);
        EXPECT_EVENTUALLY_EQ(switchSettings->isSwitchDrained(), expectDrained);
        EXPECT_EVENTUALLY_EQ(
            switchSettings->getActualSwitchDrainState(), expectDrainState);
      });
    }
  }
  void assertPortAndDrainState(bool expectDrained) const {
    assertSwitchDrainState(expectDrained);
    // Drained - expect inactive
    // Undrained - expect active
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(), masterLogicalFabricPortIds(), !expectDrained);
  }
  void forceIsolate(int delay = 1) {
    std::stringstream ss;
    ss << "edk -c fi force_isolate 0 5 1 " << delay << std::endl;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      std::string out;
      getAgentEnsemble()->runDiagCommand(ss.str(), out, switchId);
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);
    }
  }
  void forceCrash(int delay = 1) {
    std::stringstream ss;
    ss << "edk -c fi crash 0 5 " << delay << std::endl;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      std::string out;
      getAgentEnsemble()->runDiagCommand(ss.str(), out, switchId);
    }
  }
  /*
   * Invoked post FW crash to
   * - Induce a isolate event through FW
   * - Verify that event takes effect
   *   This is a concrete way of verifying that
   *   FW is functional post a crash
   */
  void forceIsolatePostCrashAndVerify() {
    WITH_RETRIES({
      // We issue force isolation with retries, since the FW
      // just crashed. So our command to force isolate may
      // race with FW restarting
      forceIsolate();
      assertSwitchDrainState(true /* drained */);
    });
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
    // Fabric connectivity manager to expect single NPU
    if (!FLAGS_multi_switch) {
      FLAGS_janga_single_npu_for_testing = true;
    }
    FLAGS_fw_drained_unrecoverable_error = true;
  }
};

class AgentVoqSwitchIsolationFirmwareWBEventsTest
    : public AgentVoqSwitchIsolationFirmwareTest {
  static bool isColdBoot;
  static uint16_t fwCapableSwitchIndex;

 public:
  static constexpr auto kEventDelay = 20;
  void tearDownAgentEnsemble(bool warmboot = false) override {
    // We check for agentEnsemble not being NULL since the tests
    // are also invoked for just listing their prod features.
    // Then in such case, agentEnsemble is never setup
    isColdBoot =
        getAgentEnsemble() && getSw()->getBootType() == BootType::COLD_BOOT;
    if (isColdBoot) {
      // We use just the first FW capable switch index for asserting for
      // counter.
      fwCapableSwitchIndex =
          *getFWCapableSwitchIndices(getSw()->getConfig()).begin();
    }
    std::atexit([]() {
      if (isColdBoot) {
        WITH_RETRIES({
          auto fwIsolated = fb303::fbData->getCounterIfExists(
              "fw_drained_with_high_num_active_fabric_links.sum");
          EXPECT_EVENTUALLY_TRUE(
              fwIsolated.has_value() && fwIsolated.value() == 0);
          auto crashCounterRegex = std::string("(switch.") +
              folly::to<std::string>(fwCapableSwitchIndex) + ".)?" +
              "bcm.isolationFirmwareCrash.sum";
          auto fwCrashedCounters =
              fb303::fbData->getRegexCounters(crashCounterRegex);
          EXPECT_EVENTUALLY_TRUE(
              fwCrashedCounters.size() == 1 &&
              fwCrashedCounters.begin()->second == 0);
        });

        XLOG(ERR)
            << "Allow for Firmware Isolate callback to fire post cb unregister, sleep for "
            << kEventDelay * 2 << " seconds";
        sleep(kEventDelay * 2);
      }
    });
    AgentHwTest::tearDownAgentEnsemble(warmboot);
  }
};

bool AgentVoqSwitchIsolationFirmwareWBEventsTest::isColdBoot = false;
uint16_t AgentVoqSwitchIsolationFirmwareWBEventsTest::fwCapableSwitchIndex = 0;

class AgentVoqSwitchIsolationFirmwareUpdateTest
    : public AgentVoqSwitchIsolationFirmwareTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true, /*interfaceHasSubnet*/
        true, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    utility::populatePortExpectedNeighborsToSelf(
        ensemble.masterLogicalPortIds(), config);
    config = clearFWConfig(config);
    return config;
  }
  void tearDownAgentEnsemble(bool warmboot = false) override {
    // We check for agentEnsemble not being NULL since the tests
    // are also invoked for just listing their prod features.
    // Then in such case, agentEnsemble is never setup
    if (getAgentEnsemble() && getSw()->getBootType() == BootType::COLD_BOOT) {
      auto agentConfig = getSw()->getAgentConfig();
      const auto& configFileName = getAgentEnsemble()->configFileName();
      agentConfig.sw() = addFwConfig(*agentConfig.sw());
      AgentEnsemble::writeConfig(agentConfig, configFileName);
    }
    AgentHwTest::tearDownAgentEnsemble(warmboot);
  }
};

TEST_F(AgentVoqSwitchIsolationFirmwareTest, forceIsolate) {
  auto setup = [this]() {
    assertPortAndDrainState(false /* not drained*/);
    setMinLinksConfig();
    forceIsolate();
  };

  auto verify = [this]() {
    assertSwitchDrainState(true /* drained */);
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(),
        masterLogicalFabricPortIds(),
        true /* expect active*/);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchIsolationFirmwareTest, forceCrash) {
  auto setup = [this]() {
    assertPortAndDrainState(false /* not drained*/);
    setMinLinksConfig();
  };

  auto verify = [this]() {
    forceCrash();
    WITH_RETRIES({
      for (auto switchIdx : getFWCapableSwitchIndices(getSw()->getConfig())) {
        auto switchStats = getHwSwitchStats(switchIdx);
        auto asicErrors = switchStats.hwAsicErrors();
        EXPECT_EVENTUALLY_GT(asicErrors->isolationFirmwareCrashes(), 0);
      }
    });
    forceIsolatePostCrashAndVerify();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(
    AgentVoqSwitchIsolationFirmwareWBEventsTest,
    forceIsolateDuringWarmBoot) {
  auto setup = [this]() {
    assertPortAndDrainState(false /* not drained*/);
    setMinLinksConfig();
    forceIsolate(kEventDelay);
  };

  auto verifyPostWarmboot = [this]() {
    assertSwitchDrainState(true /* drained */);
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(),
        masterLogicalFabricPortIds(),
        true /* expect active*/);
  };
  verifyAcrossWarmBoots(setup, []() {}, []() {}, verifyPostWarmboot);
}

TEST_F(AgentVoqSwitchIsolationFirmwareWBEventsTest, forceCrashDuringWarmBoot) {
  auto setup = [this]() {
    assertPortAndDrainState(false /* not drained*/);
    setMinLinksConfig();
    // Force delayed crash - which most likely
    // will now happen while switch is warm booting
    forceCrash(kEventDelay);
  };

  auto verifyPostWarmboot = [this]() { forceIsolatePostCrashAndVerify(); };
  verifyAcrossWarmBoots(setup, []() {}, []() {}, verifyPostWarmboot);
}

TEST_F(AgentVoqSwitchIsolationFirmwareUpdateTest, loadOnWarmboot) {
  auto setup = [this]() {
    assertPortAndDrainState(false /* not drained*/);
    setMinLinksConfig();
  };

  auto verifyPostWarmboot = [this]() {
    forceIsolate();
    assertSwitchDrainState(true /* drained */);
  };
  verifyAcrossWarmBoots(setup, []() {}, []() {}, verifyPostWarmboot);
}

} // namespace facebook::fboss
