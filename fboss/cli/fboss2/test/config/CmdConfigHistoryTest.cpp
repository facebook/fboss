// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/config/history/CmdConfigHistory.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {
const std::string initialConfigContents = R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000
      }
    ]
  }
})";
}

class CmdConfigHistoryTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigHistoryTestFixture()
      : CmdConfigTestBase(
            "fboss2_config_history_test_%%%%-%%%%-%%%%-%%%%", // unique_path
            initialConfigContents) {}
};

TEST_F(CmdConfigHistoryTestFixture, historyListsRevisions) {
  // Create revision files with valid config content
  createTestConfig(getCliConfigDir() / "agent-r1.conf", initialConfigContents);
  createTestConfig(getCliConfigDir() / "agent-r2.conf", initialConfigContents);
  createTestConfig(getCliConfigDir() / "agent-r3.conf", initialConfigContents);

  // Initialize ConfigSession singleton with test paths
  setupTestableConfigSession();

  // Create and execute the command
  auto cmd = CmdConfigHistory();
  auto result = cmd.queryClient(localhost());

  // Verify the output contains all three revisions
  EXPECT_NE(result.find("r1"), std::string::npos);
  EXPECT_NE(result.find("r2"), std::string::npos);
  EXPECT_NE(result.find("r3"), std::string::npos);

  // Verify table headers are present
  EXPECT_NE(result.find("Revision"), std::string::npos);
  EXPECT_NE(result.find("Owner"), std::string::npos);
  EXPECT_NE(result.find("Commit Time"), std::string::npos);
}

TEST_F(CmdConfigHistoryTestFixture, historyIgnoresNonMatchingFiles) {
  // Create valid revision files
  createTestConfig(getCliConfigDir() / "agent-r1.conf", initialConfigContents);
  createTestConfig(getCliConfigDir() / "agent-r2.conf", initialConfigContents);

  // Create files that should be ignored
  createTestConfig(getCliConfigDir() / "agent.conf.bak", R"({"backup": true})");
  createTestConfig(getCliConfigDir() / "other-r1.conf", R"({"other": true})");
  createTestConfig(
      getCliConfigDir() / "agent-r1.txt", R"({"wrong_ext": true})");
  createTestConfig(getCliConfigDir() / "agent-rX.conf", R"({"invalid": true})");

  // Initialize ConfigSession singleton with test paths
  setupTestableConfigSession();

  // Create and execute the command
  auto cmd = CmdConfigHistory();
  auto result = cmd.queryClient(localhost());

  // Verify only valid revisions are listed
  EXPECT_NE(result.find("r1"), std::string::npos);
  EXPECT_NE(result.find("r2"), std::string::npos);

  // Verify invalid files are not listed
  EXPECT_EQ(result.find("agent.conf.bak"), std::string::npos);
  EXPECT_EQ(result.find("other-r1.conf"), std::string::npos);
  EXPECT_EQ(result.find("agent-r1.txt"), std::string::npos);
  EXPECT_EQ(result.find("rX"), std::string::npos);
}

TEST_F(CmdConfigHistoryTestFixture, historyEmptyDirectory) {
  // Directory exists but has no revision files
  EXPECT_TRUE(cliConfigDirExists());

  // Remove the initial revision file created by SetUp() to simulate empty dir
  removeInitialRevisionFile();

  // Initialize ConfigSession singleton with test paths
  setupTestableConfigSession();

  // Create and execute the command
  auto cmd = CmdConfigHistory();
  auto result = cmd.queryClient(localhost());

  // Verify the output indicates no revisions found
  EXPECT_NE(result.find("No config revisions found"), std::string::npos);
  EXPECT_NE(result.find(getCliConfigDir().string()), std::string::npos);
}

TEST_F(CmdConfigHistoryTestFixture, historyNonSequentialRevisions) {
  // Create non-sequential revision files (e.g., after deletions)
  createTestConfig(getCliConfigDir() / "agent-r1.conf", initialConfigContents);
  createTestConfig(getCliConfigDir() / "agent-r5.conf", initialConfigContents);
  createTestConfig(getCliConfigDir() / "agent-r10.conf", initialConfigContents);

  // Initialize ConfigSession singleton with test paths
  setupTestableConfigSession();

  // Create and execute the command
  auto cmd = CmdConfigHistory();
  auto result = cmd.queryClient(localhost());

  // Verify all revisions are listed in order
  EXPECT_NE(result.find("r1"), std::string::npos);
  EXPECT_NE(result.find("r5"), std::string::npos);
  EXPECT_NE(result.find("r10"), std::string::npos);

  // Verify they appear in ascending order (r1 before r5 before r10)
  auto pos_r1 = result.find("r1");
  auto pos_r5 = result.find("r5");
  auto pos_r10 = result.find("r10");
  EXPECT_LT(pos_r1, pos_r5);
  EXPECT_LT(pos_r5, pos_r10);
}

TEST_F(CmdConfigHistoryTestFixture, printOutput) {
  auto cmd = CmdConfigHistory();
  std::string tableOutput =
      "Revision  Owner  Commit Time\nr1        user1  2024-01-01 12:00:00.000";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(tableOutput);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();

  EXPECT_NE(output.find("Revision"), std::string::npos);
  EXPECT_NE(output.find("r1"), std::string::npos);
  EXPECT_NE(output.find("user1"), std::string::npos);
}

} // namespace facebook::fboss
