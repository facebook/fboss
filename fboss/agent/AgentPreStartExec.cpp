// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"
#include "fboss/agent/AgentConfig.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/AgentCommandExecutor.h"
#include "fboss/agent/AgentNetWhoAmI.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/facebook/AgentPreStartConfig.h"

namespace facebook::fboss {

void AgentPreStartExec::run() {
  AgentDirectoryUtil dirUtil;
  AgentCommandExecutor executor;
  auto cppWedgeAgentWrapper = checkFileExists(dirUtil.getWrapperRefactorFlag());
  auto config = AgentConfig::fromDefaultFile();
  initFlagDefaults(*config->thrift.defaultCommandLineArgs());
  run(&executor,
      std::make_unique<AgentNetWhoAmI>(),
      dirUtil,
      std::move(config),
      cppWedgeAgentWrapper);
}

void AgentPreStartExec::run(
    AgentCommandExecutor* executor,
    std::unique_ptr<AgentNetWhoAmI> whoami,
    const AgentDirectoryUtil& dirUtil,
    std::unique_ptr<AgentConfig> config,
    bool cppWedgeAgentWrapper) {
  auto mode = config->getRunMode();

  if (cppWedgeAgentWrapper) {
    runAndRemoveScript(dirUtil.getPreStartShellScript());
    AgentPreStartConfig preStartConfig(
        std::move(whoami), config.get(), dirUtil);
    preStartConfig.run(executor);
  }

  if (mode != cfg::AgentRunMode::MULTI_SWITCH) {
    XLOG(INFO) << "Agent run mode is not MULTI_SWITCH";
    if (checkFileExists(dirUtil.getSwAgentServiceSymLink())) {
      XLOG(INFO) << "Stop and disable fboss_sw_agent service";
      executor->stopService("fboss_sw_agent", false);
      executor->disableService("fboss_sw_agent", false);
      executor->runCommand({"/usr/bin/pkill", "fboss_sw_agent"}, false);
    }

    if (checkFileExists(dirUtil.getHwAgentServiceTemplateSymLink())) {
      XLOG(INFO) << "Stop and disable all instances of fboss_hw_agent@ service";
      for (auto iter :
           *config->thrift.sw()->switchSettings()->switchIdToSwitchInfo()) {
        auto& switchInfo = iter.second;
        auto unitName =
            fmt::format("fboss_hw_agent@{}.service", *switchInfo.switchIndex());
        executor->stopService(unitName, false);
      }
      executor->disableService("fboss_hw_agent@.service", false);
      executor->runCommand({"/usr/bin/pkill", "fboss_hw_agent"}, false);
    }
    return;
  }
}

} // namespace facebook::fboss
