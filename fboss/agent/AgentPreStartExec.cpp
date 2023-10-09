// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"
#include "fboss/agent/AgentConfig.h"

#include <folly/Subprocess.h>
#include <folly/logging/xlog.h>
#include <folly/system/Shell.h>

using namespace folly::literals::shell_literals;

namespace facebook::fboss {

namespace {
static constexpr auto kMultiSwitchAgentPreStartScript =
    "/etc/packages/neteng-fboss-wedge_agent/current/multi_switch_agent_scripts/pre_multi_switch_agent_start.par";
}

void AgentPreStartExec::run() {
  auto config = AgentConfig::fromDefaultFile();
  if (config->getRunMode() != cfg::AgentRunMode::MULTI_SWITCH) {
    XLOG(INFO)
        << "Agent run mode is not MULTI_SWITCH, skip pre-start execution";
    return;
  }
  XLOG(INFO) << "Agent run mode is MULTI_SWITCH, perform pre-start execution";
  // TODO: do pre-initialization for MULTI_SWITCH mode agent
  try {
    auto command = "{}"_shellify(kMultiSwitchAgentPreStartScript);
    folly::Subprocess proc(std::move(command));
    proc.waitChecked();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to execute pre-start script: " << ex.what();
    exit(2);
  }
}

} // namespace facebook::fboss
