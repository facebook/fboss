// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"
#include "fboss/agent/AgentConfig.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

void AgentPreStartExec::run() {
  auto config = AgentConfig::fromDefaultFile();
  if (config->getRunMode() != cfg::AgentRunMode::MULTI_SWITCH) {
    XLOG(INFO)
        << "Agent run mode is not MULTI_SWITCH, skip pre-start execution";
    return;
  }
  XLOG(INFO) << "Agent run mode is MULTI_SWITCH, perform pre-start execution";
  // TODO: do pre-initialization for MULTI_SWITCH mode agent
}

} // namespace facebook::fboss
