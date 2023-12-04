#pragma once

#include "fboss/agent/AgentDirectoryUtil.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

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
  void waitForStop();

  AgentDirectoryUtil util_;
};
} // namespace facebook::fboss
