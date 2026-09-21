// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/json/json.h>

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigRunningAgent.h"

using namespace ::testing;

namespace facebook::fboss {
class CmdShowConfigRunningTestFixture : public CmdHandlerTestBase {
 public:
  std::string mockAgentRunningConfig;
  folly::dynamic expectedConfigRunningAgentOutput;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockAgentRunningConfig = getAgentRunningConfig();
    expectedConfigRunningAgentOutput = folly::parseJson(mockAgentRunningConfig);
  }

 private:
  std::string getAgentRunningConfig() {
    folly::dynamic agentRunningConfig = folly::dynamic::object("version", 0)(
        "switchSettings",
        folly::dynamic::object("l2LearningMode", 1)("qcmEnable", false));

    return folly::toJson(agentRunningConfig);
  }
};

TEST_F(CmdShowConfigRunningTestFixture, queryAgentClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRunningConfig(_))
      .WillOnce(Invoke(
          [&](std::string& configStr) { configStr = mockAgentRunningConfig; }));

  auto cmd = CmdShowConfigRunningAgent();
  auto configRunningAgentOutput = cmd.queryClient(localhost());
  EXPECT_EQ(configRunningAgentOutput, expectedConfigRunningAgentOutput);
}

TEST_F(CmdShowConfigRunningTestFixture, printAgentOutput) {
  std::stringstream ss;
  CmdShowConfigRunningAgent().printOutput(expectedConfigRunningAgentOutput, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      folly::toPrettyJson(expectedConfigRunningAgentOutput);
  expectedOutput.append("\n");

  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
