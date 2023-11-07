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
};

} // namespace facebook::fboss
