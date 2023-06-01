/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/dataplane_tests/HwConfigSetupTest.h"

#include "fboss/agent/AgentConfig.h"

namespace facebook::fboss {

DEFINE_string(from_agent_config, "", "Agent config used for cold boot");
DEFINE_string(to_agent_config, "", "Agent config used for warm boot");

cfg::SwitchConfig HwConfigSetupTest::getConfig(bool isWarmBoot) const {
  auto agentCfg = getAgentConfigFromFile(isWarmBoot);
  return agentCfg ? setPortsToLoopback(std::move(agentCfg))
                  : getFallbackConfig();
}

cfg::SwitchConfig HwConfigSetupTest::setPortsToLoopback(
    std::unique_ptr<AgentConfig> agentCfg) const {
  auto cfg = *agentCfg->thrift.sw();

  for (auto& port : *cfg.ports()) {
    if (*port.state() == cfg::PortState::ENABLED) {
      port.loopbackMode() =
          getAsic()->getDesiredLoopbackMode(port.get_portType());
    }
  }
  return cfg;
}

std::unique_ptr<AgentConfig> HwConfigSetupTest::getAgentConfigFromFile(
    bool isWarmBoot) const {
  std::unique_ptr<AgentConfig> agentCustomConfig{nullptr};
  std::string configPath =
      isWarmBoot ? FLAGS_to_agent_config : FLAGS_from_agent_config;
  try {
    agentCustomConfig = AgentConfig::fromFile(configPath);
    XLOG(DBG2) << "Loading agent config from " << configPath;
  } catch (const FbossError& ex) {
    XLOG(DBG2) << "No pre warmboot agent config provided, using static config";
  }
  return agentCustomConfig;
}

} // namespace facebook::fboss
