/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmConfigSetupTests.h"

#include "fboss/agent/AgentConfig.h"

namespace facebook::fboss {

DEFINE_string(agent_config, "", "Agent config to test");

cfg::SwitchConfig BcmConfigSetupTest::getConfig() const {
  auto agentCfg = getAgentConfigFromFile();
  return agentCfg ? setPortsToLoopback(std::move(agentCfg))
                  : getFallbackConfig();
}

cfg::SwitchConfig BcmConfigSetupTest::setPortsToLoopback(
    std::unique_ptr<AgentConfig> agentCfg) const {
  auto cfg = *agentCfg->thrift.sw_ref();

  for (auto& port : *cfg.ports_ref()) {
    if (*port.state_ref() == cfg::PortState::ENABLED) {
      *port.loopbackMode_ref() = cfg::PortLoopbackMode::MAC;
    }
  }
  return cfg;
}

std::unique_ptr<AgentConfig> BcmConfigSetupTest::getAgentConfigFromFile()
    const {
  std::unique_ptr<AgentConfig> agentCustomConfig{nullptr};

  try {
    agentCustomConfig = AgentConfig::fromFile(FLAGS_agent_config);
  } catch (const FbossError& ex) {
    XLOG(DBG0) << "No pre warmboot agent config provided, using static config";
  }
  return agentCustomConfig;
}

} // namespace facebook::fboss
