// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>

#include "fboss/agent/AgentCommandExecutor.h"

namespace facebook::fboss {

class MockAgentCommandExecutor : public AgentCommandExecutor {
 public:
  MOCK_CONST_METHOD2(
      runCommand,
      void(const std::vector<std::string>& cmd, bool throwOnError));
  MOCK_CONST_METHOD2(
      runShellCommand,
      void(const std::string& cmd, bool throwOnError));
  MOCK_CONST_METHOD2(
      runAndRemoveScript,
      void(
          const std::string& scriptName,
          const std::vector<std::string>& args));
};

} // namespace facebook::fboss
