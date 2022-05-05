// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

void AgentHwTest::setupConfigFlag() {
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<PortMap>(), platform());
  auto portsByControllingPort =
      utility::getSubsidiaryPortIDs(platform()->getPlatformPorts());
  for (const auto& port : portsByControllingPort) {
    masterLogicalPortIds_.push_back(port.first);
  }

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
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  auto testConfigDir = platform()->getPersistentStateDir() + "/agent_test/";
  auto newCfgFile = testConfigDir + "/agent_test.conf";
  utilCreateDir(testConfigDir);
  newcfg.dumpConfig(newCfgFile);
  FLAGS_config = newCfgFile;
  // reload config so that test config is loaded
  platform()->reloadConfig();
}

void AgentHwTest::SetUp() {
  AgentTest::SetUp();
  // TODO: handle warmboot
  // TODO: set port preemphasis 0 so cabled ports do not come up
  auto config = initialConfig();
  sw()->applyConfig("Initial config with ports up", config);
  std::vector<PortID> enabledPorts =
      folly::gen::from(*config.ports()) |
      folly::gen::filter([](const auto& port) {
        return *port.state() == cfg::PortState::ENABLED &&
            *port.loopbackMode() != cfg::PortLoopbackMode::NONE;
      }) |
      folly::gen::map(
          [](const auto& port) { return PortID(*port.logicalID()); }) |
      folly::gen::as();
  waitForLinkStatus(enabledPorts, true);
}
} // namespace facebook::fboss
