/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/agent_hw_tests/AgentConfigSetupTest.h"

#include "fboss/agent/AgentConfig.h"

namespace facebook::fboss {

DEFINE_string(from_agent_config, "", "Agent config used for cold boot");
DEFINE_string(to_agent_config, "", "Agent config used for warm boot");

cfg::SwitchConfig AgentConfigSetupTest::getConfig(
    bool isWarmBoot,
    const AgentEnsemble& ensemble) const {
  auto agentCfg = getAgentConfigFromFile(isWarmBoot);
  auto switchId = ensemble.getSw()
                      ->getScopeResolver()
                      ->scope(ensemble.masterLogicalPortIds()[0])
                      .switchId();
  auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(switchId);
  return agentCfg ? setPortsToLoopback(std::move(agentCfg), asic)
                  : getFallbackConfig(ensemble);
}

cfg::SwitchConfig AgentConfigSetupTest::setPortsToLoopback(
    std::unique_ptr<AgentConfig> agentCfg,
    const HwAsic* asic) const {
  auto cfg = *agentCfg->thrift.sw();

  for (auto& port : *cfg.ports()) {
    if (*port.state() == cfg::PortState::ENABLED) {
      port.loopbackMode() = asic->getDesiredLoopbackMode(port.get_portType());
    }
  }
  return cfg;
}

std::unique_ptr<AgentConfig> AgentConfigSetupTest::getAgentConfigFromFile(
    bool isWarmBoot) const {
  std::unique_ptr<AgentConfig> agentCustomConfig{nullptr};
  std::string configPath =
      isWarmBoot ? FLAGS_to_agent_config : FLAGS_from_agent_config;
  try {
    agentCustomConfig = AgentConfig::fromFile(configPath);
    XLOG(DBG2) << "Loading agent config from " << configPath;
  } catch (const FbossError&) {
    XLOG(DBG2) << "No pre warmboot agent config provided, using static config";
  }
  return agentCustomConfig;
}

} // namespace facebook::fboss
