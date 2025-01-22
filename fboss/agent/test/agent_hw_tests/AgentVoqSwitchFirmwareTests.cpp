// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"

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

 protected:
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
  void forceIsoalte(int delay = 1) {
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

TEST_F(AgentVoqSwitchIsolationFirmwareTest, forceIsolate) {
  auto setup = [this]() {
    assertPortAndDrainState(false /* not drained*/);
    setMinLinksConfig();
    forceIsoalte();
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
    // TODO - check crash count
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
