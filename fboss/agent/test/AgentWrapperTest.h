#pragma once

#include "fboss/agent/AgentDirectoryUtil.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

class AgentNetWhoAmI;
class AgentConfig;

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

  std::unique_ptr<AgentConfig> config_;
  AgentDirectoryUtil util_;
  std::unique_ptr<AgentNetWhoAmI> whoami_;
};
} // namespace facebook::fboss
