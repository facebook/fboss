// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"
#include "fboss/agent/AgentConfig.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/AgentCommandExecutor.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/facebook/AgentPreStartConfig.h"

namespace facebook::fboss {

namespace {
static constexpr auto kWrapperRefactorFeatureOn =
    "/etc/fboss/features/cpp_wedge_agent_wrapper/current/on";
} // namespace

void AgentPreStartExec::run() {
  AgentDirectoryUtil dirUtil;
  AgentCommandExecutor executor;
  auto cppWedgeAgentWrapper = checkFileExists(kWrapperRefactorFeatureOn);
  auto config = AgentConfig::fromDefaultFile();
  run(&executor, dirUtil, std::move(config), cppWedgeAgentWrapper);
}

void AgentPreStartExec::run(
    AgentCommandExecutor* executor,
    const AgentDirectoryUtil& dirUtil,
    std::unique_ptr<AgentConfig> config,
    bool cppWedgeAgentWrapper) {
  if (cppWedgeAgentWrapper) {
    runAndRemoveScript(dirUtil.getPreStartShellScript());
    AgentPreStartConfig preStartConfig;
    preStartConfig.run();
  }

  if (config->getRunMode() != cfg::AgentRunMode::MULTI_SWITCH) {
    XLOG(INFO)
        << "Agent run mode is not MULTI_SWITCH, skip MULTI_SWITCH pre-start execution";
    if (checkFileExists(dirUtil.getSwAgentServiceSymLink())) {
      XLOG(INFO) << "Stop and disable fboss_sw_agent service";
      executor->runCommand(
          {"/usr/bin/systemctl", "stop", "fboss_sw_agent"}, false);
      executor->runCommand(
          {"/usr/bin/systemctl", "disable", "fboss_sw_agent"}, false);
      executor->runCommand({"/usr/bin/pkill", "fboss_sw_agent"}, false);
    }

    if (checkFileExists(dirUtil.getHwAgentServiceTemplateSymLink())) {
      XLOG(INFO) << "Stop and disable all instances of fboss_hw_agent@ service";
      for (auto iter :
           *config->thrift.sw()->switchSettings()->switchIdToSwitchInfo()) {
        auto& switchInfo = iter.second;
        auto unitName =
            fmt::format("fboss_hw_agent@{}.service", *switchInfo.switchIndex());
        executor->runCommand({"/usr/bin/systemctl", "stop", unitName}, false);
      }
      executor->runCommand(
          {"/usr/bin/systemctl", "disable", "fboss_hw_agent@.service"}, false);
      executor->runCommand({"/usr/bin/pkill", "wedge_hwagent"}, false);
    }
    return;
  }
  XLOG(INFO)
      << "Agent run mode is MULTI_SWITCH, perform MULTI_SWITCH pre-start execution";
  // TODO: do pre-initialization for MULTI_SWITCH mode agent
  try {
    executor->runShellCommand(dirUtil.getMultiSwitchPreStartScript());
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to execute pre-start script: " << ex.what();
    exit(2);
  }
}

} // namespace facebook::fboss
