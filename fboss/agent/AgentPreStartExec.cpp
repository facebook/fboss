// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"
#include "fboss/agent/AgentConfig.h"

#include <folly/logging/xlog.h>
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/facebook/AgentPreStartConfig.h"

namespace facebook::fboss {

namespace {
static constexpr auto kMultiSwitchAgentPreStartScript =
    "/etc/packages/neteng-fboss-wedge_agent/current/multi_switch_agent_scripts/pre_multi_switch_agent_start.par";
static constexpr auto kWrapperRefactorFeatureOn =
    "/etc/fboss/features/cpp_wedge_agent_wrapper/current/on";
static auto constexpr kPreStartSh = "/dev/shm/fboss/pre_start.sh";
static auto constexpr kSwAgentServiceUnit =
    "/etc/systemd/system/fboss_sw_agent.service";
static auto constexpr kHwAgentServiceUnit =
    "/etc/systemd/system/fboss_hw_agent@.service";
} // namespace

void AgentPreStartExec::run() {
  if (checkFileExists(kWrapperRefactorFeatureOn)) {
    runAndRemoveScript(kPreStartSh);
    AgentPreStartConfig preStartConfig;
    preStartConfig.run();
  }

  auto config = AgentConfig::fromDefaultFile();
  if (config->getRunMode() != cfg::AgentRunMode::MULTI_SWITCH) {
    XLOG(INFO)
        << "Agent run mode is not MULTI_SWITCH, skip MULTI_SWITCH pre-start execution";
    if (checkFileExists(kSwAgentServiceUnit)) {
      XLOG(INFO) << "Stop and disable fboss_sw_agent service";
      runCommand({"/usr/bin/systemctl", "stop", "fboss_sw_agent"}, false);
      runCommand({"/usr/bin/systemctl", "disable", "fboss_sw_agent"}, false);
      runCommand({"/usr/bin/pkill", "fboss_sw_agent"}, false);
    }

    if (checkFileExists(kHwAgentServiceUnit)) {
      XLOG(INFO) << "Stop and disable all instances of fboss_hw_agent@ service";
      for (auto iter :
           *config->thrift.sw()->switchSettings()->switchIdToSwitchInfo()) {
        auto& switchInfo = iter.second;
        auto unitName =
            fmt::format("fboss_hw_agent@{}.service", *switchInfo.switchIndex());
        runCommand({"/usr/bin/systemctl", "stop", unitName}, false);
      }
      runCommand(
          {"/usr/bin/systemctl", "disable", "fboss_hw_agent@.service"}, false);
      runCommand({"/usr/bin/pkill", "wedge_hwagent"}, false);
    }
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
