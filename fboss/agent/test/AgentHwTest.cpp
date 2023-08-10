// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

void AgentHwTest::setupConfigFlag() {
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<MultiSwitchPortMap>(), platform());
  auto portsByControllingPort =
      utility::getSubsidiaryPortIDs(platform()->getPlatformPorts());
  for (const auto& port : portsByControllingPort) {
    masterLogicalPortIds_.push_back(port.first);
  }

  auto newCfgFile = getTestConfigPath();
  FLAGS_config = newCfgFile;
  if (sw()->getBootType() != BootType::WARM_BOOT) {
    cfg::AgentConfig testConfig;
    auto swConfig = initialConfig();
    for (auto& port : *swConfig.ports()) {
      // Keep ports down by disabling them and setting loopback mode to NONE
      port.state() = cfg::PortState::DISABLED;
      port.loopbackMode() = cfg::PortLoopbackMode::NONE;
    }
    testConfig.sw() = swConfig;
    const auto& baseConfig = platform()->config();
    testConfig.platform() = *baseConfig->thrift.platform();
    auto newcfg = AgentConfig(
        testConfig,
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            testConfig));
    newcfg.dumpConfig(newCfgFile);
    // reload config so that test config is loaded
    platform()->reloadConfig();
  }
}

void AgentHwTest::SetUp() {
  AgentTest::SetUp();
  if (sw()->getBootType() != BootType::WARM_BOOT) {
    if (platform()->getAsic()->isSupported(
            HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET)) {
      // Set preempahsis to 0, so ports state can be manipulated by just setting
      // loopback mode (lopbackMode::NONE == down), loopbackMode::{MAC, PHY} ==
      // up)
      sw()->updateStateBlocking(
          "set port preemphasis 0", [&](const auto& state) {
            std::shared_ptr<SwitchState> newState{state};
            for (auto& portMap : std::as_const(*newState->getPorts())) {
              for (auto& port : std::as_const(*portMap.second)) {
                auto newPort = port.second->modify(&newState);
                auto pinConfigs = newPort->getPinConfigs();
                for (auto& pin : pinConfigs) {
                  pin.tx() = phy::TxSettings();
                }
                newPort->resetPinConfigs(pinConfigs);
              }
            }
            return newState;
          });
    }

    auto config = initialConfig();
    std::vector<PortID> enabledPorts =
        folly::gen::from(*config.ports()) |
        folly::gen::filter([](const auto& port) {
          return *port.state() == cfg::PortState::ENABLED &&
              *port.loopbackMode() != cfg::PortLoopbackMode::NONE;
        }) |
        folly::gen::map(
            [](const auto& port) { return PortID(*port.logicalID()); }) |
        folly::gen::as();

    // Make sure ports are down from initial config applied in setupConfigFlag
    waitForLinkStatus(enabledPorts, false);
    // Reapply initalConfig to bring loopback ports up
    sw()->applyConfig("Initial config with ports up", config);
    waitForLinkStatus(enabledPorts, true);
  }
}

void AgentHwTest::TearDown() {
  dumpRunningConfig(getTestConfigPath());
  AgentTest::TearDown();
}
} // namespace facebook::fboss
