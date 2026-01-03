// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <boost/filesystem.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

#include "fboss/cli/fboss2/commands/config/history/CmdConfigHistory.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigHistoryTestFixture : public CmdHandlerTestBase {
 public:
  fs::path testHomeDir_;
  fs::path testEtcDir_;
  fs::path systemConfigPath_;
  fs::path sessionConfigPath_;
  fs::path cliConfigDir_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories for each test to avoid conflicts when
    // running tests in parallel
    auto tempBase = fs::temp_directory_path();
    auto uniquePath = boost::filesystem::unique_path(
        "fboss2_config_history_test_%%%%-%%%%-%%%%-%%%%");
    testHomeDir_ = tempBase / (uniquePath.string() + "_home");
    testEtcDir_ = tempBase / (uniquePath.string() + "_etc");

    // Clean up any previous test artifacts (shouldn't exist with unique names)
    std::error_code ec;
    fs::remove_all(testHomeDir_, ec);
    fs::remove_all(testEtcDir_, ec);

    // Create fresh test directories
    fs::create_directories(testHomeDir_);
    fs::create_directories(testEtcDir_);

    // Set up paths
    systemConfigPath_ = testEtcDir_ / "agent.conf";
    sessionConfigPath_ = testHomeDir_ / ".fboss2" / "agent.conf";
    cliConfigDir_ = testEtcDir_ / "coop" / "cli";

    // Create CLI config directory
    fs::create_directories(cliConfigDir_);

    // Create a default system config
    createTestConfig(
        systemConfigPath_,
        R"({
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
})");
  }

  void TearDown() override {
    // Reset the singleton to ensure tests don't interfere with each other
    TestableConfigSession::setInstance(nullptr);
    // Clean up test directories (use error_code to avoid exceptions)
    std::error_code ec;
    fs::remove_all(testHomeDir_, ec);
    fs::remove_all(testEtcDir_, ec);
    CmdHandlerTestBase::TearDown();
  }

  void createTestConfig(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path);
    file << content;
    file.close();
  }

  std::string readFile(const fs::path& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  void initializeTestSession() {
    // Ensure system config exists before initializing session
    if (!fs::exists(systemConfigPath_)) {
      createTestConfig(
          systemConfigPath_,
          R"({
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
})");
    }

    // Ensure the parent directory of session config exists
    fs::create_directories(sessionConfigPath_.parent_path());

    // Initialize ConfigSession singleton with test paths
    TestableConfigSession::setInstance(
        std::make_unique<TestableConfigSession>(
            sessionConfigPath_.string(),
            systemConfigPath_.string(),
            cliConfigDir_.string()));
  }
};

TEST_F(CmdConfigHistoryTestFixture, historyListsRevisions) {
  // Create revision files with valid config content
  createTestConfig(
      cliConfigDir_ / "agent-r1.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");
  createTestConfig(
      cliConfigDir_ / "agent-r2.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");
  createTestConfig(
      cliConfigDir_ / "agent-r3.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");

  // Initialize ConfigSession singleton with test paths
  initializeTestSession();

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
  createTestConfig(
      cliConfigDir_ / "agent-r1.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");
  createTestConfig(
      cliConfigDir_ / "agent-r2.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");

  // Create files that should be ignored
  createTestConfig(cliConfigDir_ / "agent.conf.bak", R"({"backup": true})");
  createTestConfig(cliConfigDir_ / "other-r1.conf", R"({"other": true})");
  createTestConfig(cliConfigDir_ / "agent-r1.txt", R"({"wrong_ext": true})");
  createTestConfig(cliConfigDir_ / "agent-rX.conf", R"({"invalid": true})");

  // Initialize ConfigSession singleton with test paths
  initializeTestSession();

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
  EXPECT_TRUE(fs::exists(cliConfigDir_));

  // Initialize ConfigSession singleton with test paths
  initializeTestSession();

  // Create and execute the command
  auto cmd = CmdConfigHistory();
  auto result = cmd.queryClient(localhost());

  // Verify the output indicates no revisions found
  EXPECT_NE(result.find("No config revisions found"), std::string::npos);
  EXPECT_NE(result.find(cliConfigDir_.string()), std::string::npos);
}

TEST_F(CmdConfigHistoryTestFixture, historyNonSequentialRevisions) {
  // Create non-sequential revision files (e.g., after deletions)
  createTestConfig(
      cliConfigDir_ / "agent-r1.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");
  createTestConfig(
      cliConfigDir_ / "agent-r5.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");
  createTestConfig(
      cliConfigDir_ / "agent-r10.conf",
      R"({"sw": {"ports": [{"logicalID": 1, "name": "eth1/1/1", "state": 2, "speed": 100000}]}})");

  // Initialize ConfigSession singleton with test paths
  initializeTestSession();

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
