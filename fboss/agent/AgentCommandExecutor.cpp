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

void AgentCommandExecutor::enableService(
    const std::string& serviceName,
    bool throwOnError) {
  this->runCommand({"/usr/bin/systemctl", "enable", serviceName}, throwOnError);
}

void AgentCommandExecutor::disableService(
    const std::string& serviceName,
    bool throwOnError) {
  this->runCommand(
      {"/usr/bin/systemctl", "disable", serviceName}, throwOnError);
}

void AgentCommandExecutor::startService(
    const std::string& serviceName,
    bool throwOnError) {
  this->runCommand({"/usr/bin/systemctl", "start", serviceName}, throwOnError);
}

void AgentCommandExecutor::stopService(
    const std::string& serviceName,
    bool throwOnError) {
  this->runCommand({"/usr/bin/systemctl", "stop", serviceName}, throwOnError);
}

} // namespace facebook::fboss
