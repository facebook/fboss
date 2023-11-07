// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentCommandExecutor.h"

#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

void AgentCommandExecutor::runCommand(
    const std::vector<std::string>& cmd,
    bool throwOnError) const {
  return ::facebook::fboss::runCommand(cmd, throwOnError);
}

void AgentCommandExecutor::runShellCommand(
    const std::string& cmd,
    bool throwOnError) const {
  return ::facebook::fboss::runShellCommand(cmd, throwOnError);
}

void AgentCommandExecutor::runAndRemoveScript(
    const std::string& cmd,
    const std::vector<std::string>& args) const {
  return ::facebook::fboss::runAndRemoveScript(cmd, args);
}

} // namespace facebook::fboss
