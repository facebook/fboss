// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/AgentConfig.h"
#include "fboss/qsfp_service/QsfpConfig.h"

namespace facebook::fboss {

inline void setupFakeAgentConfig(const std::string& agentCfgPath) {
  auto agentCfg = createEmptyAgentConfig();
  agentCfg->dumpConfig(agentCfgPath);
  gflags::SetCommandLineOptionWithMode(
      "config", agentCfgPath.c_str(), gflags::SET_FLAGS_DEFAULT);
}

inline void setupFakeQsfpConfig(const std::string& qsfpCfgPath) {
  auto qsfpCfg = createEmptyQsfpConfig();
  qsfpCfg->dumpConfig(qsfpCfgPath);
  gflags::SetCommandLineOptionWithMode(
      "qsfp_config", qsfpCfgPath.c_str(), gflags::SET_FLAGS_DEFAULT);
}

inline void setupFakeQsfpConfig(
    const std::string& qsfpCfgPath,
    cfg::QsfpServiceConfig cfg) {
  auto qsfpCfg = createFakeQsfpConfig(cfg);
  qsfpCfg->dumpConfig(qsfpCfgPath);
  gflags::SetCommandLineOptionWithMode(
      "qsfp_config", qsfpCfgPath.c_str(), gflags::SET_FLAGS_DEFAULT);
}

} // namespace facebook::fboss
