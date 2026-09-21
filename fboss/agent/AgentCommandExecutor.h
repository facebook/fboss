// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <vector>

namespace facebook::fboss {

class AgentCommandExecutor {
 public:
  virtual ~AgentCommandExecutor() = default;
  virtual void runCommand(
      const std::vector<std::string>& cmd,
      bool throwOnError = true) const;
  virtual void runShellCommand(const std::string& cmd, bool throwOnError = true)
      const;
  virtual void runAndRemoveScript(
      const std::string& cmd,
      const std::vector<std::string>& args = {}) const;
  void enableService(const std::string& serviceName, bool throwOnError = true);
  void disableService(const std::string& serviceName, bool throwOnError = true);
  void startService(const std::string& serviceName, bool throwOnError = true);
  void stopService(const std::string& serviceName, bool throwOnError = true);
};

} // namespace facebook::fboss
