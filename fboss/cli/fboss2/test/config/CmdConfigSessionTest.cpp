// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <folly/FileUtil.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/test/config/MockSystemdInterface.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

class ConfigSessionTestFixture : public CmdConfigTestBase {
 public:
  ConfigSessionTestFixture()
      : CmdConfigTestBase(
            "fboss_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000
      },
      {
        "logicalID": 2,
        "name": "eth1/1/2",
        "state": 2,
        "speed": 100000
      }
    ]
  }
})") {}
};

TEST_F(ConfigSessionTestFixture, sessionInitialization) {
  // Initially, session directory should not exist
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";
  EXPECT_FALSE(fs::exists(sessionDir));

  // Creating a ConfigSession should create the directory and copy the config
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Verify the directory was created
  EXPECT_TRUE(fs::exists(sessionDir));
  EXPECT_TRUE(session.sessionExists());
  EXPECT_TRUE(fs::exists(sessionConfig));

  // Verify content was copied correctly (reads via symlink)
  std::string systemContent = readFile(cliConfigPath);
  std::string sessionContent = readFile(sessionConfig);
  EXPECT_EQ(systemContent, sessionContent);
}

TEST_F(ConfigSessionTestFixture, sessionConfigModified) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Modify the session config through the ConfigSession API
  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());
  ports[0].description() = "Modified port";
  session.setCommandLine("config interface eth1/1/1 description Modified port");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Verify session config is modified
  std::string sessionContent = readFile(sessionConfig);
  std::string systemContent = readFile(cliConfigPath);
  EXPECT_NE(sessionContent, systemContent);
  EXPECT_THAT(sessionContent, ::testing::HasSubstr("Modified port"));
}

TEST_F(ConfigSessionTestFixture, sessionCommit) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Setup mock agent server
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(2);

  std::string firstCommitSha;
  std::string secondCommitSha;

  // First commit: Create a ConfigSession and commit a change
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Simulate a CLI command being tracked
    session.setCommandLine(
        "config interface eth1/1/1 description First commit");

    // Modify the session config
    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = "First commit";
    session.setCommandLine(
        "config interface eth1/1/1 description First commit");
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    // Commit the session
    auto result = session.commit(localhost());

    // Verify session config no longer exists (removed after commit)
    EXPECT_FALSE(fs::exists(sessionConfig));

    // Verify commit SHA was returned
    EXPECT_FALSE(result.commitSha.empty());
    EXPECT_EQ(result.commitSha.length(), 40); // Full SHA1 is 40 chars
    firstCommitSha = result.commitSha;

    // Verify metadata file was created alongside the config revision
    fs::path targetMetadata =
        getTestEtcDir() / "coop" / "cli" / "cli_metadata.json";
    EXPECT_TRUE(fs::exists(targetMetadata));

    // Verify system config was updated
    EXPECT_THAT(readFile(cliConfigPath), ::testing::HasSubstr("First commit"));
  }

  // Second commit: Create a new session and verify it's based on first commit
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Simulate a CLI command being tracked
    session.setCommandLine(
        "config interface eth1/1/1 description Second commit");

    // Verify the new session is based on the latest committed revision
    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    EXPECT_EQ(*ports[0].description(), "First commit");

    // Make another change to the same port
    ports[0].description() = "Second commit";
    session.setCommandLine(
        "config interface eth1/1/1 description Second commit");
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    // Commit the second change
    auto result = session.commit(localhost());

    // Verify new commit SHA was returned
    EXPECT_FALSE(result.commitSha.empty());
    EXPECT_NE(result.commitSha, firstCommitSha);
    secondCommitSha = result.commitSha;

    // Verify metadata file was created alongside the config revision
    fs::path targetMetadata =
        getTestEtcDir() / "coop" / "cli" / "cli_metadata.json";
    EXPECT_TRUE(fs::exists(targetMetadata));

    // Verify system config was updated
    EXPECT_THAT(readFile(cliConfigPath), ::testing::HasSubstr("Second commit"));

    // Verify Git history has all commits
    auto& git = session.getGit();
    auto commits = git.log(cliConfigPath.string());
    EXPECT_EQ(commits.size(), 3); // Initial + 2 commits

    // Verify metadata file was also committed to git
    auto metadataCommits = git.log(targetMetadata.string());
    EXPECT_EQ(metadataCommits.size(), 3); // Initial baseline + 2 commits
  }
}

// Ensure commit() works on a newly initialized session
// This verifies that initializeSession() creates the metadata file
TEST_F(ConfigSessionTestFixture, commitOnNewlyInitializedSession) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path cliConfigDir = getTestEtcDir() / "coop" / "cli";

  // Setup mock agent server
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(1);

  // Create a new session
  // This tests that metadata file is created during session initialization
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Verify metadata file was created during session initialization
  fs::path metadataPath = sessionDir / "cli_metadata.json";
  EXPECT_TRUE(fs::exists(metadataPath));

  // Make a change so commit has something to commit
  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());
  ports[0].description() = "Test change for commit";
  session.setCommandLine(
      "config interface eth1/1/1 description Test change for commit");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Commit should succeed
  auto result = session.commit(localhost());
  EXPECT_FALSE(result.commitSha.empty());

  // Verify metadata file was copied to CLI config directory
  fs::path targetMetadata = cliConfigDir / "cli_metadata.json";
  EXPECT_TRUE(fs::exists(targetMetadata));
}

TEST_F(ConfigSessionTestFixture, multipleChangesInOneSession) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Make first change
  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());
  ports[0].description() = "Change 1";
  session.setCommandLine("config interface eth1/1/1 description Change 1");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  EXPECT_THAT(readFile(sessionConfig), ::testing::HasSubstr("Change 1"));

  // Make second change
  ports[0].description() = "Change 2";
  session.setCommandLine("config interface eth1/1/1 description Change 2");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  EXPECT_THAT(readFile(sessionConfig), ::testing::HasSubstr("Change 2"));

  // Make third change
  ports[0].description() = "Change 3";
  session.setCommandLine("config interface eth1/1/1 description Change 3");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  EXPECT_THAT(readFile(sessionConfig), ::testing::HasSubstr("Change 3"));
}

TEST_F(ConfigSessionTestFixture, sessionPersistsAcrossCommands) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create first ConfigSession and modify config
  {
    TestableConfigSession session1(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config = session1.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = "Persistent change";
    session1.setCommandLine(
        "config interface eth1/1/1 description Persistent change");
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  }

  // Verify session persists (file still exists with same content)
  EXPECT_TRUE(fs::exists(sessionConfig));
  EXPECT_THAT(
      readFile(sessionConfig), ::testing::HasSubstr("Persistent change"));

  // Create another ConfigSession to simulate another command reading the
  // session
  {
    TestableConfigSession session2(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config = session2.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    // Verify the change persisted
    EXPECT_EQ(*ports[0].description(), "Persistent change");
  }
}

TEST_F(ConfigSessionTestFixture, configRollbackOnFailure) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Save the original config content
  std::string originalContent = readFile(cliConfigPath);

  // Setup mock agent server to fail reloadConfig on first call (the commit),
  // but succeed on second call (the rollback reload)
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig())
      .WillOnce(::testing::Throw(std::runtime_error("Reload failed")))
      .WillOnce(::testing::Return());

  // Create a ConfigSession and try to commit
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());
  ports[0].description() = "Failed change";
  session.setCommandLine("config interface eth1/1/1 description Failed change");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Commit should fail and rollback the config
  EXPECT_THROW(session.commit(localhost()), std::runtime_error);

  // Verify config was rolled back to original content
  std::string currentContent = readFile(cliConfigPath);
  EXPECT_EQ(currentContent, originalContent);

  // Verify session config still exists (not removed on failed commit)
  EXPECT_TRUE(fs::exists(sessionConfig));
}

TEST_F(ConfigSessionTestFixture, concurrentCommits) {
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Setup mock agent server
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(2);

  // Run two sequential commits to test Git commit functionality
  // Note: Git doesn't handle truly concurrent commits well due to index.lock,
  // so we run them sequentially to avoid race conditions.
  std::string commitSha1;
  std::string commitSha2;

  // First commit
  {
    fs::path sessionDir = getTestHomeDir() / ".fboss2_user1";

    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = "First commit";
    session.setCommandLine(
        "config interface eth1/1/1 description First commit");
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    auto result = session.commit(localhost());
    commitSha1 = result.commitSha;
  }

  // Second commit
  {
    fs::path sessionDir = getTestHomeDir() / ".fboss2_user2";

    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = "Second commit";
    session.setCommandLine(
        "config interface eth1/1/1 description Second commit");
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    auto result = session.commit(localhost());
    commitSha2 = result.commitSha;
  }

  // Both commits should succeed with different commit SHAs
  EXPECT_FALSE(commitSha1.empty());
  EXPECT_FALSE(commitSha2.empty());
  EXPECT_NE(commitSha1, commitSha2);

  // Verify Git history contains both commits
  Git git((getTestEtcDir() / "coop").string());
  auto commits = git.log(cliConfigPath.string());
  EXPECT_GE(commits.size(), 3); // Initial + 2 commits
}

TEST_F(ConfigSessionTestFixture, rollbackToSpecificCommit) {
  // This test calls the rollback() method with a specific commit SHA
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";
  fs::path metadataPath =
      getTestEtcDir() / "coop" / "cli" / "cli_metadata.json";

  // Setup mock agent server
  setupMockedAgentServer();
  // 2 commits + 1 rollback = 3 reloadConfig calls
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(3);

  // Create a session and make several commits to build history
  std::string firstCommitSha;
  std::string secondCommitSha;
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Simulate CLI command for first commit
    session.setCommandLine(
        "config interface eth1/1/1 description First version");

    // First commit
    auto& config1 = session.getAgentConfig();
    (*config1.sw()->ports())[0].description() = "First version";
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    auto result1 = session.commit(localhost());
    firstCommitSha = result1.commitSha;

    // Second commit (need new session after commit)
  }
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Simulate CLI command for second commit
    session.setCommandLine(
        "config interface eth1/1/1 description Second version");

    auto& config2 = session.getAgentConfig();
    (*config2.sw()->ports())[0].description() = "Second version";
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    auto result2 = session.commit(localhost());
    secondCommitSha = result2.commitSha;
  }

  // Verify current content is "Second version"
  EXPECT_THAT(readFile(cliConfigPath), ::testing::HasSubstr("Second version"));
  // Verify current metadata contains second command
  EXPECT_THAT(
      readFile(metadataPath), ::testing::HasSubstr("description Second"));

  // Now rollback to first commit
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    std::string rollbackSha = session.rollback(localhost(), firstCommitSha);

    // Verify rollback created a new commit
    EXPECT_FALSE(rollbackSha.empty());
    EXPECT_NE(rollbackSha, firstCommitSha);
    EXPECT_NE(rollbackSha, secondCommitSha);

    // Verify config content is now "First version"
    EXPECT_THAT(readFile(cliConfigPath), ::testing::HasSubstr("First version"));

    // Verify system metadata (in /etc/coop) was also rolled back to first
    // version
    std::string metadataContent = readFile(metadataPath);
    EXPECT_THAT(metadataContent, ::testing::HasSubstr("description First"));
    EXPECT_THAT(
        metadataContent,
        ::testing::Not(::testing::HasSubstr("description Second")));

    // Verify session config was updated to match the rolled-back config
    fs::path sessionConfigPath = sessionDir / "agent.conf";
    EXPECT_THAT(
        readFile(sessionConfigPath), ::testing::HasSubstr("First version"));

    // Verify session metadata was updated with the new base and empty commands
    fs::path sessionMetadataPath = sessionDir / "cli_metadata.json";
    std::string sessionMetadataContent = readFile(sessionMetadataPath);
    folly::dynamic sessionMetadata = folly::parseJson(sessionMetadataContent);
    // The base should be set to the rollback commit SHA
    EXPECT_EQ(sessionMetadata["base"].asString(), rollbackSha)
        << "Session metadata base should be set to the rollback commit SHA";
    // Commands should be empty (clean session)
    EXPECT_TRUE(sessionMetadata["commands"].isArray())
        << "Session metadata commands should be an array";
    EXPECT_EQ(sessionMetadata["commands"].size(), 0)
        << "Session metadata commands array should be empty after rollback to "
           "clean session";

    // Verify Git history has the rollback commit
    auto& git = session.getGit();
    auto commits = git.log(cliConfigPath.string());
    EXPECT_EQ(commits.size(), 4); // Initial + 2 commits + rollback

    // Verify metadata file history
    auto metadataCommits = git.log(metadataPath.string());
    EXPECT_EQ(
        metadataCommits.size(), 4); // Initial baseline + 2 commits + rollback
  }
}

TEST_F(ConfigSessionTestFixture, rollbackToPreviousCommit) {
  // This test calls the rollback() method without a commit SHA argument
  // to rollback to the previous commit
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Setup mock agent server
  setupMockedAgentServer();
  // 2 commits + 1 rollback = 3 reloadConfig calls
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(3);

  // Create commits to build history
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config1 = session.getAgentConfig();
    (*config1.sw()->ports())[0].description() = "First version";
    session.setCommandLine(
        "config interface eth1/1/1 description First version");
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    session.commit(localhost());
  }
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config2 = session.getAgentConfig();
    (*config2.sw()->ports())[0].description() = "Second version";
    session.setCommandLine(
        "config interface eth1/1/1 description Second version");
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    session.commit(localhost());
  }

  // Verify current content is "Second version"
  EXPECT_THAT(readFile(cliConfigPath), ::testing::HasSubstr("Second version"));

  // Rollback to previous commit (no argument)
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    std::string rollbackSha = session.rollback(localhost());

    // Verify rollback succeeded
    EXPECT_FALSE(rollbackSha.empty());

    // Verify content is now "First version" (from previous commit)
    EXPECT_THAT(readFile(cliConfigPath), ::testing::HasSubstr("First version"));

    // Verify session config was updated to match the rolled-back config
    fs::path sessionConfigPath = sessionDir / "agent.conf";
    EXPECT_THAT(
        readFile(sessionConfigPath), ::testing::HasSubstr("First version"));

    // Verify session metadata was updated with the new base and empty commands
    fs::path sessionMetadataPath = sessionDir / "cli_metadata.json";
    std::string sessionMetadataContent = readFile(sessionMetadataPath);
    folly::dynamic sessionMetadata = folly::parseJson(sessionMetadataContent);
    // The base should be set to the rollback commit SHA
    EXPECT_EQ(sessionMetadata["base"].asString(), rollbackSha)
        << "Session metadata base should be set to the rollback commit SHA";
    // Commands should be empty (clean session)
    EXPECT_TRUE(sessionMetadata["commands"].isArray())
        << "Session metadata commands should be an array";
    EXPECT_EQ(sessionMetadata["commands"].size(), 0)
        << "Session metadata commands array should be empty after rollback to "
           "clean session";
  }
}

TEST_F(ConfigSessionTestFixture, actionLevelDefaultIsHitless) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Default action level should be HITLESS
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::HITLESS);
}

TEST_F(ConfigSessionTestFixture, actionLevelUpdateAndGet) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Update to AGENT_WARMBOOT
  session.updateRequiredAction(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  // Verify the action level was updated
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::AGENT_WARMBOOT);
}

TEST_F(ConfigSessionTestFixture, actionLevelHigherTakesPrecedence) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Update to AGENT_WARMBOOT first
  session.updateRequiredAction(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  // Try to "downgrade" to HITLESS - should be ignored
  session.updateRequiredAction(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Verify action level remains at AGENT_WARMBOOT
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::AGENT_WARMBOOT);
}

TEST_F(ConfigSessionTestFixture, actionLevelReset) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Set to AGENT_WARMBOOT
  session.updateRequiredAction(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  // Reset the action level
  session.resetRequiredAction(cli::ServiceType::AGENT);

  // Verify action level was reset to HITLESS
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::HITLESS);
}

TEST_F(ConfigSessionTestFixture, actionLevelPersistsToMetadataFile) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path metadataFile = sessionDir / "cli_metadata.json";

  // Create a ConfigSession and set action level via saveConfig
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Load the config (required before saveConfig)
    session.getAgentConfig();
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);
  }

  // Verify metadata file exists and has correct JSON format
  EXPECT_TRUE(fs::exists(metadataFile));
  std::string content = readFile(metadataFile);

  // Parse the JSON and verify structure - uses symbolic enum names
  folly::dynamic json = folly::parseJson(content);
  EXPECT_TRUE(json.isObject());
  EXPECT_TRUE(json.count("action"));
  EXPECT_TRUE(json["action"].isObject());
  EXPECT_TRUE(json["action"].count("AGENT"));
  EXPECT_EQ(json["action"]["AGENT"].asString(), "AGENT_WARMBOOT");
}

TEST_F(ConfigSessionTestFixture, actionLevelLoadsFromMetadataFile) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "cli_metadata.json";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Create session directory and metadata file manually
  fs::create_directories(sessionDir);
  std::ofstream metaFile(metadataFile);
  // Use symbolic enum names for human readability
  metaFile << R"({"action":{"AGENT":"AGENT_WARMBOOT"}})";
  metaFile.close();

  // Also create the session config file (otherwise session will overwrite from
  // system)
  fs::copy_file(cliConfigPath, sessionConfig);

  // Create a ConfigSession - should load action level from metadata file
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Verify action level was loaded
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::AGENT_WARMBOOT);
}

TEST_F(ConfigSessionTestFixture, actionLevelPersistsAcrossSessions) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // First session: set action level via saveConfig
  {
    TestableConfigSession session1(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Load the config (required before saveConfig)
    session1.getAgentConfig();
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);
  }

  // Second session: verify action level was persisted
  {
    TestableConfigSession session2(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    EXPECT_EQ(
        session2.getRequiredAction(cli::ServiceType::AGENT),
        cli::ConfigActionLevel::AGENT_WARMBOOT);
  }
}

TEST_F(ConfigSessionTestFixture, commandTrackingBasic) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path metadataFile = sessionDir / "cli_metadata.json";

  // Create a ConfigSession, execute command, and verify persistence
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Initially, no commands should be recorded
    EXPECT_TRUE(session.getCommands().empty());

    // Simulate a command and save config
    session.setCommandLine("config interface eth1/1/1 description Test change");
    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = "Test change";
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    // Verify command was recorded in memory
    EXPECT_EQ(1, session.getCommands().size());
    EXPECT_EQ(
        "config interface eth1/1/1 description Test change",
        session.getCommands()[0]);
  }

  // Verify metadata file exists and has commands persisted
  EXPECT_TRUE(fs::exists(metadataFile));
  std::string content = readFile(metadataFile);

  // Parse the JSON and verify structure
  folly::dynamic json = folly::parseJson(content);
  EXPECT_TRUE(json.isObject());
  EXPECT_TRUE(json.count("commands"));
  EXPECT_TRUE(json["commands"].isArray());
  EXPECT_EQ(1, json["commands"].size());
  EXPECT_EQ(
      "config interface eth1/1/1 description Test change",
      json["commands"][0].asString());
}

TEST_F(ConfigSessionTestFixture, commandTrackingMultipleCommands) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Execute multiple commands
  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());

  session.setCommandLine("config interface eth1/1/1 mtu 9000");
  ports[0].description() = "First change";
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  session.setCommandLine("config interface eth1/1/1 description Test");
  ports[0].description() = "Second change";
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  session.setCommandLine("config interface eth1/1/1 speed 100G");
  ports[0].description() = "Third change";
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Verify all commands were recorded in order
  EXPECT_EQ(3, session.getCommands().size());
  EXPECT_EQ("config interface eth1/1/1 mtu 9000", session.getCommands()[0]);
  EXPECT_EQ(
      "config interface eth1/1/1 description Test", session.getCommands()[1]);
  EXPECT_EQ("config interface eth1/1/1 speed 100G", session.getCommands()[2]);
}

TEST_F(ConfigSessionTestFixture, commandTrackingPersistsAcrossSessions) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // First session: execute some commands
  {
    TestableConfigSession session1(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config = session1.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());

    session1.setCommandLine("config interface eth1/1/1 mtu 9000");
    ports[0].description() = "First change";
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    session1.setCommandLine("config interface eth1/1/1 description Test");
    ports[0].description() = "Second change";
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  }

  // Second session: verify commands were persisted
  {
    TestableConfigSession session2(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    EXPECT_EQ(2, session2.getCommands().size());
    EXPECT_EQ("config interface eth1/1/1 mtu 9000", session2.getCommands()[0]);
    EXPECT_EQ(
        "config interface eth1/1/1 description Test",
        session2.getCommands()[1]);
  }
}

TEST_F(ConfigSessionTestFixture, commandTrackingClearedOnReset) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  // Create a ConfigSession and add some commands
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());

  session.setCommandLine("config interface eth1/1/1 mtu 9000");
  ports[0].description() = "Test change";
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  EXPECT_EQ(1, session.getCommands().size());

  // Reset the action level (which also clears commands)
  session.resetRequiredAction(cli::ServiceType::AGENT);

  // Verify commands were cleared
  EXPECT_TRUE(session.getCommands().empty());
}

TEST_F(ConfigSessionTestFixture, commandTrackingLoadsFromMetadataFile) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "cli_metadata.json";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Create session directory and metadata file manually
  fs::create_directories(sessionDir);
  std::ofstream metaFile(metadataFile);
  metaFile << R"({
    "action": {"AGENT": "HITLESS"},
    "commands": ["cmd1", "cmd2", "cmd3"]
  })";
  metaFile.close();

  // Also create the session config file
  fs::copy_file(cliConfigPath, sessionConfig);

  // Create a ConfigSession - should load commands from metadata file
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Verify commands were loaded
  EXPECT_EQ(3, session.getCommands().size());
  EXPECT_EQ("cmd1", session.getCommands()[0]);
  EXPECT_EQ("cmd2", session.getCommands()[1]);
  EXPECT_EQ("cmd3", session.getCommands()[2]);
}

// Test that concurrent sessions are detected and rejected
// Scenario: user1 and user2 both start sessions based on the same commit,
// user1 commits first, then user2 tries to commit and should fail.
TEST_F(ConfigSessionTestFixture, concurrentSessionConflict) {
  fs::path sessionDir1 = getTestHomeDir() / ".fboss2_user1";
  fs::path sessionDir2 = getTestHomeDir() / ".fboss2_user2";

  // Setup mock agent server
  setupMockedAgentServer();
  // Only user1's commit should succeed, so only 1 reloadConfig call
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(1);

  // User1 creates a session (captures current HEAD as base)
  TestableConfigSession session1(
      sessionDir1.string(), (getTestEtcDir() / "coop").string());

  // User2 also creates a session at the same time (same base)
  TestableConfigSession session2(
      sessionDir2.string(), (getTestEtcDir() / "coop").string());

  // User1 makes a change and commits
  auto& config1 = session1.getAgentConfig();
  (*config1.sw()->ports())[0].description() = "User1 change";
  session1.setCommandLine("config interface eth1/1/1 description User1 change");
  session1.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  auto result1 = session1.commit(localhost());
  EXPECT_FALSE(result1.commitSha.empty());

  // User2 makes a different change
  auto& config2 = session2.getAgentConfig();
  (*config2.sw()->ports())[0].description() = "User2 change";
  session2.setCommandLine("config interface eth1/1/1 description User2 change");
  session2.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // User2 tries to commit but should fail because user1 already committed
  EXPECT_THROW(
      {
        try {
          session2.commit(localhost());
        } catch (const std::runtime_error& e) {
          // Verify the error message mentions the conflict
          EXPECT_THAT(
              e.what(),
              ::testing::HasSubstr("system configuration has changed"));
          throw;
        }
      },
      std::runtime_error);

  // Verify that only user1's change is in the system config
  Git git((getTestEtcDir() / "coop").string());
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";
  std::string content;
  EXPECT_TRUE(folly::readFile(cliConfigPath.c_str(), content));
  EXPECT_THAT(content, ::testing::HasSubstr("User1 change"));
  EXPECT_THAT(content, ::testing::Not(::testing::HasSubstr("User2 change")));
}

TEST_F(ConfigSessionTestFixture, rebaseSuccessNoConflict) {
  // Test successful rebase when user2's changes don't conflict with user1's
  fs::path sessionDir1 = getTestHomeDir() / ".fboss2_user1";
  fs::path sessionDir2 = getTestHomeDir() / ".fboss2_user2";

  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(2);

  // User1 creates a session
  TestableConfigSession session1(
      sessionDir1.string(), (getTestEtcDir() / "coop").string());

  // User2 also creates a session at the same time (same base)
  TestableConfigSession session2(
      sessionDir2.string(), (getTestEtcDir() / "coop").string());

  // User1 changes port[0] description and commits
  auto& config1 = session1.getAgentConfig();
  (*config1.sw()->ports())[0].description() = "User1 change";
  session1.setCommandLine("config interface eth1/1/1 description User1 change");
  session1.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  auto result1 = session1.commit(localhost());
  EXPECT_FALSE(result1.commitSha.empty());

  // User2 changes port[1] description (non-conflicting - different port)
  auto& config2 = session2.getAgentConfig();
  ASSERT_GE(config2.sw()->ports()->size(), 2) << "Need at least 2 ports";
  (*config2.sw()->ports())[1].description() = "User2 change";
  session2.setCommandLine("config interface eth1/1/2 description User2 change");
  session2.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // User2 tries to commit but fails due to stale base
  EXPECT_THROW(session2.commit(localhost()), std::runtime_error);

  // User2 rebases - should succeed since changes don't conflict
  EXPECT_NO_THROW(session2.rebase());

  // Now user2 can commit
  auto result2 = session2.commit(localhost());
  EXPECT_FALSE(result2.commitSha.empty());

  // Verify both changes are in the final config
  Git git((getTestEtcDir() / "coop").string());
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";
  std::string content;
  EXPECT_TRUE(folly::readFile(cliConfigPath.c_str(), content));
  EXPECT_THAT(content, ::testing::HasSubstr("User1 change"));
  EXPECT_THAT(content, ::testing::HasSubstr("User2 change"));
}

TEST_F(ConfigSessionTestFixture, rebaseFailsOnConflict) {
  // Test that rebase fails when user2's changes conflict with user1's
  fs::path sessionDir1 = getTestHomeDir() / ".fboss2_user1";
  fs::path sessionDir2 = getTestHomeDir() / ".fboss2_user2";

  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(1);

  // User1 creates a session
  TestableConfigSession session1(
      sessionDir1.string(), (getTestEtcDir() / "coop").string());

  // User2 also creates a session at the same time (same base)
  TestableConfigSession session2(
      sessionDir2.string(), (getTestEtcDir() / "coop").string());

  // User1 changes port[0] description to "User1 change"
  auto& config1 = session1.getAgentConfig();
  (*config1.sw()->ports())[0].description() = "User1 change";
  session1.setCommandLine("config interface eth1/1/1 description User1 change");
  session1.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  auto result1 = session1.commit(localhost());
  EXPECT_FALSE(result1.commitSha.empty());

  // User2 changes the SAME port[0] description to "User2 change" (conflict!)
  auto& config2 = session2.getAgentConfig();
  (*config2.sw()->ports())[0].description() = "User2 change";
  session2.setCommandLine("config interface eth1/1/1 description User2 change");
  session2.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // User2 tries to rebase but should fail due to conflict
  EXPECT_THROW(
      {
        try {
          session2.rebase();
        } catch (const std::runtime_error& e) {
          EXPECT_THAT(e.what(), ::testing::HasSubstr("conflict"));
          throw;
        }
      },
      std::runtime_error);
}

TEST_F(ConfigSessionTestFixture, rebaseNotNeeded) {
  // Test that rebase throws when session is already up-to-date
  fs::path sessionDir = getTestHomeDir() / ".fboss2";

  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(0);

  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Make a change but don't commit yet
  auto& config = session.getAgentConfig();
  (*config.sw()->ports())[0].description() = "My change";
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Try to rebase - should fail because we're already on HEAD
  EXPECT_THROW(
      {
        try {
          session.rebase();
        } catch (const std::runtime_error& e) {
          EXPECT_THAT(e.what(), ::testing::HasSubstr("No rebase needed"));
          throw;
        }
      },
      std::runtime_error);
}

// Tests 3-way merge algorithm through rebase() covering:
// - Only session changed (head == base)
// - Only head changed (session == base)
// - Both changed to same value (no conflict)
// - Both changed to different values (conflict)
TEST_F(ConfigSessionTestFixture, threeWayMergeScenarios) {
  fs::path sessionDir1 = getTestHomeDir() / ".fboss2_user1";
  fs::path sessionDir2 = getTestHomeDir() / ".fboss2_user2";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  setupMockedAgentServer();
  // 5 commits: 2 in scenario 1, 2 in scenario 2, 1 in scenario 3 (rebase fails)
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(5);

  // Scenario 1: Only session changed, head unchanged
  // User1 commits, User2 changes different field - should merge cleanly
  {
    TestableConfigSession session1(
        sessionDir1.string(), (getTestEtcDir() / "coop").string());
    TestableConfigSession session2(
        sessionDir2.string(), (getTestEtcDir() / "coop").string());

    (*session1.getAgentConfig().sw()->ports())[0].name() = "port0_renamed";
    session1.setCommandLine("config interface eth1/1/1 name port0_renamed");
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    session1.commit(localhost());

    (*session2.getAgentConfig().sw()->ports())[1].description() = "port1_desc";
    session2.setCommandLine("config interface eth1/1/2 description port1_desc");
    session2.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    EXPECT_NO_THROW(session2.rebase());
    session2.commit(localhost());

    std::string content;
    EXPECT_TRUE(folly::readFile(cliConfigPath.c_str(), content));
    EXPECT_THAT(content, ::testing::HasSubstr("port0_renamed"));
    EXPECT_THAT(content, ::testing::HasSubstr("port1_desc"));
  }

  // Scenario 2: Both changed same field to identical value - no conflict
  {
    TestableConfigSession session1(
        sessionDir1.string(), (getTestEtcDir() / "coop").string());
    TestableConfigSession session2(
        sessionDir2.string(), (getTestEtcDir() / "coop").string());

    (*session1.getAgentConfig().sw()->ports())[0].description() = "same_value";
    session1.setCommandLine("config interface eth1/1/1 description same_value");
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    session1.commit(localhost());

    (*session2.getAgentConfig().sw()->ports())[0].description() = "same_value";
    session2.setCommandLine("config interface eth1/1/1 description same_value");
    session2.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    EXPECT_NO_THROW(session2.rebase());
    session2.commit(localhost());

    std::string content;
    EXPECT_TRUE(folly::readFile(cliConfigPath.c_str(), content));
    EXPECT_THAT(content, ::testing::HasSubstr("same_value"));
  }

  // Scenario 3: Both changed same field to different values - conflict
  {
    TestableConfigSession session1(
        sessionDir1.string(), (getTestEtcDir() / "coop").string());
    TestableConfigSession session2(
        sessionDir2.string(), (getTestEtcDir() / "coop").string());

    (*session1.getAgentConfig().sw()->ports())[0].description() = "user1_value";
    session1.setCommandLine(
        "config interface eth1/1/1 description user1_value");
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    session1.commit(localhost());

    (*session2.getAgentConfig().sw()->ports())[0].description() = "user2_value";
    session2.setCommandLine(
        "config interface eth1/1/1 description user2_value");
    session2.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    EXPECT_THROW(
        {
          try {
            session2.rebase();
          } catch (const std::runtime_error& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conflict"));
            throw;
          }
        },
        std::runtime_error);
  }
}

// Test that committing an empty session (no changes) returns an empty result
// and doesn't create a git commit
TEST_F(ConfigSessionTestFixture, emptyCommit) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Setup mock agent server (should not be called for empty commit)
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(0);

  // Create a session but don't make any changes
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Get the current git HEAD before commit
  Git git((getTestEtcDir() / "coop").string());
  auto commitsBefore = git.log(cliConfigPath.string(), 10);

  // Commit without making any changes
  auto result = session.commit(localhost());

  // Verify the result indicates no commit was made
  EXPECT_TRUE(result.commitSha.empty());
  EXPECT_TRUE(result.actions.empty());

  // Verify no new git commit was created
  auto commitsAfter = git.log(cliConfigPath.string(), 10);
  EXPECT_EQ(commitsBefore.size(), commitsAfter.size());

  // Verify session still exists (not removed on empty commit)
  EXPECT_TRUE(session.sessionExists());
}

// Test that committing twice in a row - second commit should be empty
TEST_F(ConfigSessionTestFixture, commitTwiceSecondIsEmpty) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path cliConfigPath = getTestEtcDir() / "coop" / "cli" / "agent.conf";

  // Setup mock agent server (should only be called once for the first commit)
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(1);

  // First commit: make a change and commit
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = "First commit change";
    session.setCommandLine(
        "config interface eth1/1/1 description \"First commit change\"");
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    auto result = session.commit(localhost());
    EXPECT_FALSE(result.commitSha.empty());
    EXPECT_FALSE(result.actions.empty());
  }

  // Second commit: try to commit again without making changes
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());

    // Don't make any changes, just try to commit
    auto result = session.commit(localhost());

    // Verify the result indicates no commit was made
    EXPECT_TRUE(result.commitSha.empty());
    EXPECT_TRUE(result.actions.empty());
  }
}

// Editing one part of the typed BGP config (e.g. a top-level field) and saving
// must round-trip the WHOLE config, so peers/peer-groups already staged in the
// file survive. ConfigSession owns the entire typed bgp::thrift::BgpConfig and
// is agnostic about which section a command touches.
TEST_F(ConfigSessionTestFixture, bgpConfigEditPreservesOtherSections) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  TestableConfigSession session(
      sessionDir.string(), (getTestEtcDir() / "coop").string());

  // Stage a config that already has peers/peer-groups (as another command would
  // have done via the same typed config).
  fs::path bgpPath = sessionDir / "bgp_config.json";
  folly::dynamic staged = folly::dynamic::object(
      "peers",
      folly::dynamic::array(
          folly::dynamic::object("peer_addr", "2401:db00::1")(
              "is_passive", true)))(
      "peer_groups",
      folly::dynamic::array(folly::dynamic::object("name", "RACK")));
  ASSERT_TRUE(folly::writeFile(folly::toPrettyJson(staged), bgpPath.c_str()));

  // Edit a top-level field via the typed API and persist.
  auto& bgp = session.getBgpConfig();
  bgp.router_id() = "10.0.0.1";
  bgp.count_confeds_in_as_path_len() = true;
  session.setCommandLine(
      "fboss2-dev config protocol bgp global router-id 10.0.0.1");
  session.saveBgpConfig();

  // The saved file carries the edited fields AND the untouched peers/groups.
  std::string content;
  ASSERT_TRUE(folly::readFile(bgpPath.c_str(), content));
  auto saved = folly::parseJson(content);

  EXPECT_EQ(saved["router_id"].asString(), "10.0.0.1");
  ASSERT_TRUE(saved.count("count_confeds_in_as_path_len"));
  EXPECT_TRUE(saved["count_confeds_in_as_path_len"].asBool());

  ASSERT_TRUE(saved.count("peers"));
  ASSERT_EQ(saved["peers"].size(), 1);
  EXPECT_EQ(saved["peers"][0]["peer_addr"].asString(), "2401:db00::1");
  EXPECT_TRUE(saved["peers"][0]["is_passive"].asBool());

  ASSERT_TRUE(saved.count("peer_groups"));
  ASSERT_EQ(saved["peer_groups"].size(), 1);
  EXPECT_EQ(saved["peer_groups"][0]["name"].asString(), "RACK");

  // A BGP_PP restart must be recorded so `config session commit` applies it.
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::BGP_PP),
      cli::ConfigActionLevel::BGP_PP_RESTART);
}

// A staged BGP edit persists to disk and is seeded back by a fresh session via
// the typed view.
TEST_F(ConfigSessionTestFixture, bgpConfigEditPersistsAndSeeds) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path bgpPath = sessionDir / "bgp_config.json";
  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());
    auto& bgp = session.getBgpConfig();
    bgp.local_as_4_byte() = 65001;
    session.setCommandLine(
        "fboss2-dev config protocol bgp global local-asn 65001");
    session.saveBgpConfig();
  }
  ASSERT_TRUE(fs::exists(bgpPath));

  {
    TestableConfigSession session(
        sessionDir.string(), (getTestEtcDir() / "coop").string());
    auto& bgp = session.getBgpConfig();
    ASSERT_TRUE(bgp.local_as_4_byte().has_value());
    EXPECT_EQ(*bgp.local_as_4_byte(), 65001);
  }
}

// A BGP-only session must be rebaseable when another commit advances HEAD; the
// staged BGP global edit is preserved (merged against an unchanged bgpcpp.conf)
// and the agent change committed by the other user is folded in.
TEST_F(ConfigSessionTestFixture, rebaseBgpSession) {
  fs::path sessionDir1 = getTestHomeDir() / ".fboss2_user1";
  fs::path sessionDir2 = getTestHomeDir() / ".fboss2_user2";

  setupMockedAgentServer();
  // Only user1's agent commit reloads the agent; rebase does not.
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(1);

  TestableConfigSession session1(
      sessionDir1.string(), (getTestEtcDir() / "coop").string());
  TestableConfigSession session2(
      sessionDir2.string(), (getTestEtcDir() / "coop").string());

  // user1 commits an agent change -> HEAD advances, session2's base goes stale.
  (*session1.getAgentConfig().sw()->ports())[0].description() = "User1 change";
  session1.setCommandLine("config interface eth1/1/1 description User1");
  session1.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  EXPECT_FALSE(session1.commit(localhost()).commitSha.empty());

  // user2 stages a BGP change.
  session2.getBgpConfig().router_id() = "7.7.7.7";
  session2.setCommandLine("config protocol bgp global router-id 7.7.7.7");
  session2.saveBgpConfig();

  // Committing fails (stale base); rebase resolves it without conflicts.
  EXPECT_THROW(session2.commit(localhost()), std::runtime_error);
  EXPECT_NO_THROW(session2.rebase());

  // The staged BGP value survives the rebase...
  std::string bgp;
  ASSERT_TRUE(
      folly::readFile((sessionDir2 / "bgp_config.json").string().c_str(), bgp));
  EXPECT_THAT(bgp, ::testing::HasSubstr("7.7.7.7"));
  // ...and user1's agent change was folded into session2's agent config.
  std::string agent;
  ASSERT_TRUE(
      folly::readFile((sessionDir2 / "agent.conf").string().c_str(), agent));
  EXPECT_THAT(agent, ::testing::HasSubstr("User1 change"));
}

// Rolling back to an earlier commit must restore the BGP system config and
// restart bgp_pp (not just the agent config).
TEST_F(ConfigSessionTestFixture, rollbackBgpConfig) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path bgpSys = getTestEtcDir() / "coop" / "bgpcpp" / "bgpcpp.conf";

  auto makeSession = [&]() {
    auto s = std::make_unique<TestableConfigSession>(
        sessionDir.string(), (getTestEtcDir() / "coop").string());
    // BGP commits/rollback restart bgp_pp via systemd; mock it out.
    s->setMockSystemdFactory([] {
      return std::make_unique<::testing::NiceMock<MockSystemdInterface>>();
    });
    return s;
  };

  std::string sha1;
  {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "1.1.1.1";
    s->setCommandLine("config protocol bgp global router-id 1.1.1.1");
    s->saveBgpConfig();
    sha1 = s->commit(localhost()).commitSha;
    ASSERT_FALSE(sha1.empty());
  }
  {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "2.2.2.2";
    s->setCommandLine("config protocol bgp global router-id 2.2.2.2");
    s->saveBgpConfig();
    ASSERT_FALSE(s->commit(localhost()).commitSha.empty());
  }

  // System BGP config now reflects the latest commit.
  std::string content;
  ASSERT_TRUE(folly::readFile(bgpSys.string().c_str(), content));
  EXPECT_THAT(content, ::testing::HasSubstr("2.2.2.2"));

  // Roll back to the first commit -> BGP config restored to 1.1.1.1.
  {
    auto s = makeSession();
    std::string rb = s->rollback(localhost(), sha1);
    EXPECT_FALSE(rb.empty());
  }
  ASSERT_TRUE(folly::readFile(bgpSys.string().c_str(), content));
  EXPECT_THAT(content, ::testing::HasSubstr("1.1.1.1"));
  EXPECT_THAT(content, ::testing::Not(::testing::HasSubstr("2.2.2.2")));
}

// Re-committing a BGP config that is byte-identical to the running
// /etc/coop/bgpcpp/bgpcpp.conf must be a no-op: no git commit and (crucially)
// no disruptive bgp_pp restart. saveBgpConfig() records BGP_PP_RESTART
// unconditionally, so commit() compares staged vs running content.
TEST_F(ConfigSessionTestFixture, commitUnchangedBgpConfigIsNoOp) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  auto makeSession = [&]() {
    auto s = std::make_unique<TestableConfigSession>(
        sessionDir.string(), (getTestEtcDir() / "coop").string());
    s->setMockSystemdFactory([] {
      return std::make_unique<::testing::NiceMock<MockSystemdInterface>>();
    });
    return s;
  };

  {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "1.1.1.1";
    s->setCommandLine("config protocol bgp global router-id 1.1.1.1");
    s->saveBgpConfig();
    ASSERT_FALSE(s->commit(localhost()).commitSha.empty());
  }
  // Stage the SAME value again and commit -> no-op (empty commitSha, so no git
  // revision and no bgp_pp restart).
  {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "1.1.1.1";
    s->setCommandLine("config protocol bgp global router-id 1.1.1.1");
    s->saveBgpConfig();
    auto result = s->commit(localhost());
    EXPECT_TRUE(result.commitSha.empty())
        << "committing an unchanged BGP config should be a no-op (no restart)";
    EXPECT_EQ(result.actions.count(cli::ServiceType::BGP_PP), 0u)
        << "unchanged BGP config must not apply BGP_PP_RESTART";
  }
}

// commit() must abort if it cannot read the running bgpcpp.conf, instead of
// silently proceeding with an empty snapshot (which a later failed-commit
// rollback would write back, corrupting the running config). Mirrors the guard
// rollback() already has.
TEST_F(ConfigSessionTestFixture, commitThrowsWhenRunningBgpConfigUnreadable) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path bgpSys = getTestEtcDir() / "coop" / "bgpcpp" / "bgpcpp.conf";
  auto makeSession = [&]() {
    auto s = std::make_unique<TestableConfigSession>(
        sessionDir.string(), (getTestEtcDir() / "coop").string());
    s->setMockSystemdFactory([] {
      return std::make_unique<::testing::NiceMock<MockSystemdInterface>>();
    });
    return s;
  };

  {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "1.1.1.1";
    s->setCommandLine("config protocol bgp global router-id 1.1.1.1");
    s->saveBgpConfig();
    ASSERT_FALSE(s->commit(localhost()).commitSha.empty());
  }
  ASSERT_TRUE(fs::exists(bgpSys));

  // Make the running config unreadable; running as a non-root user this makes
  // folly::readFile fail. (Skip the assertion if we happen to run as root,
  // where permission bits are bypassed.)
  fs::permissions(bgpSys, fs::perms::none);
  if (::geteuid() != 0) {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "2.2.2.2";
    s->setCommandLine("config protocol bgp global router-id 2.2.2.2");
    s->saveBgpConfig();
    EXPECT_THROW(s->commit(localhost()), std::runtime_error);
  }
  // Restore perms so fixture teardown can remove the temp tree.
  fs::permissions(bgpSys, fs::perms::owner_all);
}

// A BGP-only session (bgp_config.json + metadata present, agent.conf session
// file absent) must RESUME on the next CLI invocation, not be misdetected as
// fresh -- otherwise requiredActions_ (BGP_PP_RESTART) is cleared and the
// staged change is silently dropped at commit time.
TEST_F(ConfigSessionTestFixture, bgpOnlySessionResumesAcrossInvocations) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  fs::path agentSess = sessionDir / "agent.conf";
  fs::path bgpSess = sessionDir / "bgp_config.json";
  auto makeSession = [&]() {
    auto s = std::make_unique<TestableConfigSession>(
        sessionDir.string(), (getTestEtcDir() / "coop").string());
    s->setMockSystemdFactory([] {
      return std::make_unique<::testing::NiceMock<MockSystemdInterface>>();
    });
    return s;
  };

  {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "9.9.9.9";
    s->setCommandLine("config protocol bgp global router-id 9.9.9.9");
    s->saveBgpConfig();
  }
  // Simulate a pure BGP-only session: drop the agent session file that the
  // first construction seeded, leaving only the staged BGP config + metadata.
  fs::remove(agentSess);
  ASSERT_TRUE(fs::exists(bgpSess));
  ASSERT_FALSE(fs::exists(agentSess));

  // The next invocation must resume and still commit the staged BGP change.
  {
    auto s = makeSession();
    auto result = s->commit(localhost());
    EXPECT_FALSE(result.commitSha.empty())
        << "BGP-only session must resume, not be wiped as a fresh session";
  }
}

// No-arg rollback() walks cli/cli_metadata.json history so BGP-only commits are
// reachable. The initial baseline commit must therefore also touch metadata, or
// rolling back to the baseline (after exactly one real commit) fails.
TEST_F(ConfigSessionTestFixture, noArgRollbackReachesBaseline) {
  fs::path sessionDir = getTestHomeDir() / ".fboss2";
  auto makeSession = [&]() {
    auto s = std::make_unique<TestableConfigSession>(
        sessionDir.string(), (getTestEtcDir() / "coop").string());
    s->setMockSystemdFactory([] {
      return std::make_unique<::testing::NiceMock<MockSystemdInterface>>();
    });
    return s;
  };

  {
    auto s = makeSession();
    s->getBgpConfig().router_id() = "1.1.1.1";
    s->setCommandLine("config protocol bgp global router-id 1.1.1.1");
    s->saveBgpConfig();
    ASSERT_FALSE(s->commit(localhost()).commitSha.empty());
  }
  // Exactly one real commit sits on top of the baseline; no-arg rollback must
  // still reach the baseline (it appears in metadata history now).
  {
    auto s = makeSession();
    std::string rb;
    EXPECT_NO_THROW(rb = s->rollback(localhost()));
    EXPECT_FALSE(rb.empty());
  }
}

} // namespace facebook::fboss
