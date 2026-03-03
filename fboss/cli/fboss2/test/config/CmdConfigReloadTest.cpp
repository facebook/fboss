// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

#include "fboss/cli/fboss2/commands/config/CmdConfigReload.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigReloadTestFixture : public ::testing::Test {
 public:
  void SetUp() override {}
};

TEST_F(CmdConfigReloadTestFixture, printOutput) {
  auto cmd = CmdConfigReload();
  std::string successMessage = "Config reloaded successfully";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(successMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput = "Config reloaded successfully\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdConfigReloadTestFixture, printOutputCustomMessage) {
  auto cmd = CmdConfigReload();
  std::string customMessage = "Custom test message";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(customMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput = "Custom test message\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
