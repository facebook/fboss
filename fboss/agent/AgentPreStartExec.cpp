// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"
#include "fboss/agent/AgentConfig.h"

#include <folly/logging/xlog.h>
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

namespace {
static constexpr auto kMultiSwitchAgentPreStartScript =
    "/etc/packages/neteng-fboss-wedge_agent/current/multi_switch_agent_scripts/pre_multi_switch_agent_start.par";
static constexpr auto kWrapperRefactorFeatureOn =
    "/etc/fboss/features/cpp_wedge_agent_wrapper/current/on";
static auto constexpr kPreStartSh = "/dev/shm/fboss/pre_start.sh";
} // namespace

void AgentPreStartExec::run() {
  if (checkFileExists(kWrapperRefactorFeatureOn)) {
    runAndRemoveScript(kPreStartSh);
  }

  auto config = AgentConfig::fromDefaultFile();
  if (config->getRunMode() != cfg::AgentRunMode::MULTI_SWITCH) {
    XLOG(INFO)
        << "Agent run mode is not MULTI_SWITCH, skip MULTI_SWITCH pre-start execution";
    return;
  }
  XLOG(INFO)
      << "Agent run mode is MULTI_SWITCH, perform MULTI_SWITCH pre-start execution";
  // TODO: do pre-initialization for MULTI_SWITCH mode agent
  try {
    runShellCommand(kMultiSwitchAgentPreStartScript);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to execute pre-start script: " << ex.what();
    exit(2);
  }
}

} // namespace facebook::fboss
