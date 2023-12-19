#pragma once

#include "fboss/agent/AgentDirectoryUtil.h"

#include <gtest/gtest.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss {

class AgentNetWhoAmI;
class AgentConfig;

template <typename T>
class AgentWrapperTest : public ::testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;

 protected:
  void startWedgeAgent();
  void stopWedgeAgent();

  void start();
  void stop();
  void wait(bool started);
  void waitForStart();
  void waitForStart(const std::string& unit);
  void waitForStop(bool crash = false);
  void waitForStop(const std::string& unit, bool crash = false);
  pid_t getAgentPid(const std::string& agentName) const;
  std::optional<std::string> getCoreDirectory(
      const std::string& agentName,
      pid_t pid) const;
  std::string getCoreFile(const std::string& directory) const;
  std::string getCoreMetaData(const std::string& directory) const;
  BootType getBootType();

  std::unique_ptr<AgentConfig> config_;
  AgentDirectoryUtil util_;
  std::unique_ptr<AgentNetWhoAmI> whoami_;
};
} // namespace facebook::fboss
