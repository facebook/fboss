// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/l2/CmdShowL2.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

std::string createExpectedMessage() {
  return "Please run \"show mac details\" for L2 entries.";
}

class CmdShowL2TestFixture : public CmdHandlerTestBase {
 public:
  std::string expectedMessage;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    expectedMessage = createExpectedMessage();
  }
};

TEST_F(CmdShowL2TestFixture, queryClient) {
  auto cmd = CmdShowL2();
  auto message = cmd.queryClient(localhost());
  EXPECT_EQ(message, expectedMessage);
}

TEST_F(CmdShowL2TestFixture, printOutput) {
  auto cmd = CmdShowL2();

  std::stringstream ss;
  cmd.printOutput(expectedMessage, ss);

  std::string output = ss.str();
  std::string expectOutput =
      "Please run \"show mac details\" for L2 entries.\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
