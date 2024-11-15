// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>

namespace facebook::fboss {

class AgentDirectoryUtil;
struct AgentConfig;
class AgentCommandExecutor;
class AgentNetWhoAmI;

class AgentPreStartExec {
 public:
  AgentPreStartExec() {}
  void run();
  void run(
      AgentCommandExecutor* commandExecutor,
      std::unique_ptr<AgentNetWhoAmI> whoami,
      const AgentDirectoryUtil& dirUtil,
      std::unique_ptr<AgentConfig> config,
      bool cppWedgeAgentWrapper,
      bool isNetOS,
      int switchIndex);
};

} // namespace facebook::fboss
