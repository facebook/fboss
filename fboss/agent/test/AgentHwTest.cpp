// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

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
  testConfig.sw() = initialConfig();
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
} // namespace facebook::fboss
