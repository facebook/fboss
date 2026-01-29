// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/running_config/CmdShowRunningConfig.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdShowRunningConfigTestFixture : public CmdHandlerTestBase {
 public:
  std::string mockConfig;
  std::string expectedPrettyConfig;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    // A simple JSON config for testing
    mockConfig = R"({"sw":{"ports":[]}})";
    // folly::toPrettyJson formats with 2-space indent
    expectedPrettyConfig = R"({
  "sw": {
    "ports": []
  }
})";
  }
};

TEST_F(CmdShowRunningConfigTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRunningConfig(_))
      .WillOnce(Invoke([&](std::string& config) { config = mockConfig; }));

  auto cmd = CmdShowRunningConfig();
  auto result = cmd.queryClient(localhost());

  EXPECT_EQ(result, expectedPrettyConfig);
}

} // namespace facebook::fboss
