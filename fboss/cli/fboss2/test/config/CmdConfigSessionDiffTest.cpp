// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>

#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigSessionDiffTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigSessionDiffTestFixture()
      : CmdConfigTestBase(
            "fboss2_config_session_diff_test_%%%%-%%%%-%%%%-%%%%",
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
})") {}
};

TEST_F(CmdConfigSessionDiffTestFixture, diffNoSession) {
  // Initialize the session (which creates the session config file)
  setupTestableConfigSession();

  // Then delete the session config file to simulate "no session" case
  std::filesystem::remove(getSessionConfigPath());

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList emptyRevisions(std::vector<std::string>{});

  auto result = cmd.queryClient(localhost(), emptyRevisions);

  EXPECT_EQ(result, "No config session exists. Make a config change first.");
}

TEST_F(CmdConfigSessionDiffTestFixture, diffIdenticalConfigs) {
  setupTestableConfigSession();

  // initializeTestSession() already creates the session config by copying
  // the system config, so we don't need to do it again

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList emptyRevisions(std::vector<std::string>{});

  auto result = cmd.queryClient(localhost(), emptyRevisions);

  EXPECT_NE(
      result.find(
          "No differences between current live config and session config"),
      std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffDifferentConfigs) {
  setupTestableConfigSession();

  // Create session directory and modify the config
  std::filesystem::create_directories(getSessionConfigPath().parent_path());
  createTestConfig(
      getSessionConfigPath(),
      R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "description": "Modified port",
        "state": 2,
        "speed": 100000
      }
    ]
  }
})");

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList emptyRevisions(std::vector<std::string>{});

  auto result = cmd.queryClient(localhost(), emptyRevisions);

  // Should show a diff with the added "description" field
  EXPECT_NE(result.find("description"), std::string::npos);
  EXPECT_NE(result.find("Modified port"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffSessionVsRevision) {
  setupTestableConfigSession();

  // Create a revision file
  createTestConfig(
      getCliConfigDir() / "agent-r1.conf",
      R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 1
      }
    ]
  }
})");

  // Create session with different content
  std::filesystem::create_directories(getSessionConfigPath().parent_path());
  createTestConfig(
      getSessionConfigPath(),
      R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2
      }
    ]
  }
})");

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList revisions(std::vector<std::string>{"r1"});

  auto result = cmd.queryClient(localhost(), revisions);

  // Should show a diff between r1 and session (state changed from 1 to 2)
  EXPECT_NE(result.find("-        \"state\": 1"), std::string::npos);
  EXPECT_NE(result.find("+        \"state\": 2"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffTwoRevisions) {
  setupTestableConfigSession();

  // Create two different revision files
  createTestConfig(
      getCliConfigDir() / "agent-r1.conf",
      R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2
      }
    ]
  }
})");

  createTestConfig(
      getCliConfigDir() / "agent-r2.conf",
      R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "description": "Added description",
        "state": 2
      }
    ]
  }
})");

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList revisions(std::vector<std::string>{"r1", "r2"});

  auto result = cmd.queryClient(localhost(), revisions);

  // Should show the added "description" field
  EXPECT_NE(result.find("description"), std::string::npos);
  EXPECT_NE(result.find("Added description"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffWithCurrentKeyword) {
  // Remove the initial revision file created by SetUp() to simulate empty dir
  removeInitialRevisionFile();
  setupTestableConfigSession();

  // Create a revision file
  createTestConfig(
      getCliConfigDir() / "agent-r1.conf",
      R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 1
      }
    ]
  }
})");

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList revisions(std::vector<std::string>{"r1", "current"});

  auto result = cmd.queryClient(localhost(), revisions);

  // Should show a diff between r1 and current system config (state changed from
  // 1 to 2, speed added)
  EXPECT_NE(result.find("-        \"state\": 1"), std::string::npos);
  EXPECT_NE(result.find("+        \"state\": 2"), std::string::npos);
  EXPECT_NE(result.find("+        \"speed\": 100000"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffNonexistentRevision) {
  setupTestableConfigSession();

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList revisions(std::vector<std::string>{"r999"});

  // Should throw an exception for nonexistent revision
  EXPECT_THROW(cmd.queryClient(localhost(), revisions), std::invalid_argument);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffTooManyArguments) {
  setupTestableConfigSession();

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList revisions(std::vector<std::string>{"r1", "r2", "r3"});

  // Should throw an exception for too many arguments
  EXPECT_THROW(cmd.queryClient(localhost(), revisions), std::invalid_argument);
}

TEST_F(CmdConfigSessionDiffTestFixture, printOutput) {
  auto cmd = CmdConfigSessionDiff();
  std::string diffOutput =
      "--- a/config\n+++ b/config\n@@ -1 +1 @@\n-old\n+new";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(diffOutput);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();

  EXPECT_NE(output.find("--- a/config"), std::string::npos);
  EXPECT_NE(output.find("+++ b/config"), std::string::npos);
  EXPECT_NE(output.find("-old"), std::string::npos);
  EXPECT_NE(output.find("+new"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, printOutputNoDifferences) {
  auto cmd = CmdConfigSessionDiff();
  std::string noDiffMessage =
      "No differences between current live config and session config.";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(noDiffMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();

  EXPECT_NE(output.find("No differences"), std::string::npos);
}

} // namespace facebook::fboss
