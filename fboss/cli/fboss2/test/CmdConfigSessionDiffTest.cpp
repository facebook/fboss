// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <boost/filesystem/operations.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/PortMap.h" // NOLINT(misc-include-cleaner)

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigSessionDiffTestFixture : public CmdHandlerTestBase {
 public:
  fs::path testHomeDir_;
  fs::path testEtcDir_;
  fs::path systemConfigDir_; // /etc/coop (git repo root)
  fs::path sessionConfigDir_; // ~/.fboss2

  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories for each test to avoid conflicts when
    // running tests in parallel
    auto tempBase = fs::temp_directory_path();
    auto uniquePath = boost::filesystem::unique_path(
        "fboss2_config_session_diff_test_%%%%-%%%%-%%%%-%%%%");
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
    // Structure: systemConfigDir_ = /etc/coop (git repo root)
    //   - agent.conf (symlink -> cli/agent.conf)
    //   - cli/agent.conf (actual config file)
    systemConfigDir_ = testEtcDir_ / "coop";
    sessionConfigDir_ = testHomeDir_ / ".fboss2";
    fs::path cliConfigPath = systemConfigDir_ / "cli" / "agent.conf";

    // Create CLI config directory
    fs::create_directories(systemConfigDir_ / "cli");

    // Create the actual config file at cli/agent.conf
    createTestConfig(
        cliConfigPath,
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

    // Create symlink at /etc/coop/agent.conf -> cli/agent.conf
    fs::create_symlink("cli/agent.conf", systemConfigDir_ / "agent.conf");

    // Initialize Git repository and create initial commit
    Git git(systemConfigDir_.string());
    git.init();
    git.commit({cliConfigPath.string()}, "Initial commit");
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
    fs::path cliConfigPath = systemConfigDir_ / "cli" / "agent.conf";

    // Ensure system config exists before initializing session
    // (ConfigSession constructor calls initializeSession which will copy it)
    if (!fs::exists(cliConfigPath)) {
      createTestConfig(
          cliConfigPath,
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

    // Ensure the session config directory exists
    fs::create_directories(sessionConfigDir_);

    // Initialize ConfigSession singleton with test paths
    // The constructor will automatically call initializeSession()
    TestableConfigSession::setInstance(
        std::make_unique<TestableConfigSession>(
            sessionConfigDir_.string(), systemConfigDir_.string()));
  }
};

TEST_F(CmdConfigSessionDiffTestFixture, diffNoSession) {
  fs::path sessionConfigPath = sessionConfigDir_ / "agent.conf";

  // Initialize the session (which creates the session config file)
  initializeTestSession();

  // Then delete the session config file to simulate "no session" case
  fs::remove(sessionConfigPath);

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList emptyRevisions(std::vector<std::string>{});

  auto result = cmd.queryClient(localhost(), emptyRevisions);

  EXPECT_EQ(result, "No config session exists. Make a config change first.");
}

TEST_F(CmdConfigSessionDiffTestFixture, diffIdenticalConfigs) {
  initializeTestSession();

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
  fs::path sessionConfigPath = sessionConfigDir_ / "agent.conf";

  initializeTestSession();

  // Create session directory and modify the config
  fs::create_directories(sessionConfigDir_);
  createTestConfig(
      sessionConfigPath,
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
  fs::path cliConfigPath = systemConfigDir_ / "cli" / "agent.conf";
  fs::path sessionConfigPath = sessionConfigDir_ / "agent.conf";

  // Create a commit with state: 1
  createTestConfig(
      cliConfigPath,
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

  Git git(systemConfigDir_.string());
  std::string commitSha = git.commit({cliConfigPath.string()}, "State 1");

  initializeTestSession();

  // Create session with different content (state: 2)
  fs::create_directories(sessionConfigDir_);
  createTestConfig(
      sessionConfigPath,
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
  utils::RevisionList revisions(std::vector<std::string>{commitSha});

  auto result = cmd.queryClient(localhost(), revisions);

  // Should show a diff between the commit and session (state changed from 1 to
  // 2)
  EXPECT_NE(result.find("-        \"state\": 1"), std::string::npos);
  EXPECT_NE(result.find("+        \"state\": 2"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffTwoRevisions) {
  fs::path cliConfigPath = systemConfigDir_ / "cli" / "agent.conf";
  Git git(systemConfigDir_.string());

  // Create first commit with state: 2
  createTestConfig(
      cliConfigPath,
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
  std::string commit1 = git.commit({cliConfigPath.string()}, "Commit 1");

  // Create second commit with description added
  createTestConfig(
      cliConfigPath,
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
  std::string commit2 = git.commit({cliConfigPath.string()}, "Commit 2");

  initializeTestSession();

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList revisions(std::vector<std::string>{commit1, commit2});

  auto result = cmd.queryClient(localhost(), revisions);

  // Should show the added "description" field
  EXPECT_NE(result.find("description"), std::string::npos);
  EXPECT_NE(result.find("Added description"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffWithCurrentKeyword) {
  fs::path cliConfigPath = systemConfigDir_ / "cli" / "agent.conf";
  Git git(systemConfigDir_.string());

  // Create a commit with state: 1
  createTestConfig(
      cliConfigPath,
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
  std::string commit1 = git.commit({cliConfigPath.string()}, "State 1");

  // Update system config to state: 2 with speed (this is "current")
  createTestConfig(
      cliConfigPath,
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
  git.commit({cliConfigPath.string()}, "Current state");

  initializeTestSession();

  auto cmd = CmdConfigSessionDiff();
  utils::RevisionList revisions(std::vector<std::string>{commit1, "current"});

  auto result = cmd.queryClient(localhost(), revisions);

  // Should show a diff between commit1 and current system config (state changed
  // from 1 to 2, speed added)
  EXPECT_NE(result.find("-        \"state\": 1"), std::string::npos);
  EXPECT_NE(result.find("+        \"state\": 2"), std::string::npos);
  EXPECT_NE(result.find("+        \"speed\": 100000"), std::string::npos);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffNonexistentRevision) {
  initializeTestSession();

  auto cmd = CmdConfigSessionDiff();
  // Use a fake SHA that doesn't exist
  utils::RevisionList revisions(
      std::vector<std::string>{"0000000000000000000000000000000000000000"});

  // Should throw an exception for nonexistent revision
  // Git throws runtime_error when the commit doesn't exist
  EXPECT_THROW(cmd.queryClient(localhost(), revisions), std::runtime_error);
}

TEST_F(CmdConfigSessionDiffTestFixture, diffTooManyArguments) {
  initializeTestSession();

  auto cmd = CmdConfigSessionDiff();
  // Use fake SHAs for the test
  utils::RevisionList revisions(
      std::vector<std::string>{
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
          "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
          "cccccccccccccccccccccccccccccccccccccccc"});

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
