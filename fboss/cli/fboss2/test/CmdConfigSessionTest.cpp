/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/filesystem.hpp>
#include <folly/json.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <thread>

#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

class ConfigSessionTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories for each test to avoid conflicts when
    // running tests in parallel
    auto tempBase = fs::temp_directory_path();
    auto uniquePath =
        boost::filesystem::unique_path("fboss_test_%%%%-%%%%-%%%%-%%%%");
    testHomeDir_ = tempBase / (uniquePath.string() + "_home");
    testEtcDir_ = tempBase / (uniquePath.string() + "_etc");

    // Clean up any previous test artifacts (shouldn't exist with unique names)
    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }
    if (fs::exists(testEtcDir_)) {
      fs::remove_all(testEtcDir_, ec);
    }

    // Create test directories
    fs::create_directories(testHomeDir_);
    fs::create_directories(testEtcDir_ / "coop");
    fs::create_directories(testEtcDir_ / "coop" / "cli");

    // Set environment variables
    setenv("HOME", testHomeDir_.c_str(), 1);
    setenv("USER", "testuser", 1);

    // Create a test system config file as agent-r1.conf in the cli directory
    // and create a symlink at agent.conf pointing to it
    fs::path initialRevision = testEtcDir_ / "coop" / "cli" / "agent-r1.conf";
    createTestConfig(initialRevision, R"({
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

    // Create symlink at agent.conf pointing to agent-r1.conf
    // Use an absolute path for the symlink target so it works in tests
    systemConfigPath_ = testEtcDir_ / "coop" / "agent.conf";
    fs::create_symlink(initialRevision, systemConfigPath_);
  }

  void TearDown() override {
    // Clean up test directories
    // Use error_code to avoid throwing exceptions in TearDown
    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
      if (ec) {
        std::cerr << "Warning: Failed to remove " << testHomeDir_ << ": "
                  << ec.message() << std::endl;
      }
    }
    if (fs::exists(testEtcDir_)) {
      fs::remove_all(testEtcDir_, ec);
      if (ec) {
        std::cerr << "Warning: Failed to remove " << testEtcDir_ << ": "
                  << ec.message() << std::endl;
      }
    }

    CmdHandlerTestBase::TearDown();
  }

 protected:
  void createTestConfig(const fs::path& path, const std::string& content) {
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

  fs::path testHomeDir_;
  fs::path testEtcDir_;
  fs::path systemConfigPath_;
};

TEST_F(ConfigSessionTestFixture, sessionInitialization) {
  // Initially, session directory should not exist
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  EXPECT_FALSE(fs::exists(sessionDir));

  // Creating a ConfigSession should create the directory and copy the config
  // systemConfigPath_ is already a symlink created in SetUp()
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

  // Verify the directory was created
  EXPECT_TRUE(fs::exists(sessionDir));
  EXPECT_TRUE(session.sessionExists());
  EXPECT_TRUE(fs::exists(sessionConfig));

  // Verify content was copied correctly
  // Read the actual file that systemConfigPath_ points to
  // fs::read_symlink returns a relative path, so we need to resolve it
  fs::path symlinkTarget = fs::read_symlink(systemConfigPath_);
  fs::path actualConfigPath = systemConfigPath_.parent_path() / symlinkTarget;
  std::string systemContent = readFile(actualConfigPath);
  std::string sessionContent = readFile(sessionConfig);
  EXPECT_EQ(systemContent, sessionContent);
}

TEST_F(ConfigSessionTestFixture, sessionConfigModified) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  // systemConfigPath_ is already a symlink created in SetUp()
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

  // Modify the session config through the ConfigSession API
  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());
  ports[0].description() = "Modified port";
  session.setCommandLine("config interface eth1/1/1 description Modified port");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Verify session config is modified
  std::string sessionContent = readFile(sessionConfig);
  // fs::read_symlink returns a relative path, so we need to resolve it
  fs::path symlinkTarget = fs::read_symlink(systemConfigPath_);
  fs::path actualConfigPath = systemConfigPath_.parent_path() / symlinkTarget;
  std::string systemContent = readFile(actualConfigPath);
  EXPECT_NE(sessionContent, systemContent);
  EXPECT_THAT(sessionContent, ::testing::HasSubstr("Modified port"));
}

TEST_F(ConfigSessionTestFixture, sessionCommit) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Verify old symlink exists (created in SetUp)
  EXPECT_TRUE(fs::is_symlink(systemConfigPath_));
  EXPECT_EQ(
      fs::read_symlink(systemConfigPath_), cliConfigDir / "agent-r1.conf");

  // Setup mock agent server
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(2);

  // First commit: Create a ConfigSession and commit a change
  {
    // systemConfigPath_ is already a symlink to agent-r1.conf created in
    // SetUp()
    TestableConfigSession session(
        sessionConfig.string(),
        systemConfigPath_.string(),
        cliConfigDir.string());

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

    // Verify new revision was created in cli directory
    EXPECT_EQ(result.revision, 2);
    fs::path targetConfig = cliConfigDir / "agent-r2.conf";
    EXPECT_TRUE(fs::exists(targetConfig));
    EXPECT_THAT(readFile(targetConfig), ::testing::HasSubstr("First commit"));

    // Verify metadata file was created alongside the config revision
    fs::path targetMetadata = cliConfigDir / "agent-r2.metadata.json";
    EXPECT_TRUE(fs::exists(targetMetadata));

    // Verify symlink was replaced and points to new revision
    EXPECT_TRUE(fs::is_symlink(systemConfigPath_));
    EXPECT_EQ(fs::read_symlink(systemConfigPath_), targetConfig);
  }

  // Second commit: Create a new session and verify it's based on r2, not r1
  {
    TestableConfigSession session(
        sessionConfig.string(),
        systemConfigPath_.string(),
        cliConfigDir.string());

    // Verify the new session is based on r2 (the latest committed revision)
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

    // Verify new revision was created
    EXPECT_EQ(result.revision, 3);
    fs::path targetConfig = cliConfigDir / "agent-r3.conf";
    EXPECT_TRUE(fs::exists(targetConfig));
    EXPECT_THAT(readFile(targetConfig), ::testing::HasSubstr("Second commit"));

    // Verify metadata file was created alongside the config revision
    fs::path targetMetadata = cliConfigDir / "agent-r3.metadata.json";
    EXPECT_TRUE(fs::exists(targetMetadata));

    // Verify symlink was updated to point to r3
    EXPECT_TRUE(fs::is_symlink(systemConfigPath_));
    EXPECT_EQ(fs::read_symlink(systemConfigPath_), targetConfig);

    // Verify all revisions and their metadata files exist
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.metadata.json"));
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.metadata.json"));
  }
}

// Ensure commit() works on a newly initialized session
// This verifies that initializeSession() creates the metadata file
TEST_F(ConfigSessionTestFixture, commitOnNewlyInitializedSession) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Setup mock agent server
  setupMockedAgentServer();
  // No config changes were made, so reloadConfig() should not be called
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(0);

  // Create a new session and immediately commit it
  // This tests that metadata file is created during session initialization
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      cliConfigDir.string());

  // Verify metadata file was created during session initialization
  fs::path metadataPath = sessionDir / "conf_metadata.json";
  EXPECT_TRUE(fs::exists(metadataPath));

  // Make no changes to the session. It's initialized but that's it.

  // Commit should succeed, right now empty sessions still commmit a new
  // revision (TODO: fix this so we don't create empty commits).
  auto result = session.commit(localhost());
  EXPECT_EQ(result.revision, 2);

  // Verify metadata file was copied to revision directory
  fs::path targetMetadata = cliConfigDir / "agent-r2.metadata.json";
  EXPECT_TRUE(fs::exists(targetMetadata));
}

TEST_F(ConfigSessionTestFixture, multipleChangesInOneSession) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  // systemConfigPath_ is already a symlink created in SetUp()
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

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
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create first ConfigSession and modify config
  // systemConfigPath_ is already a symlink created in SetUp()
  {
    TestableConfigSession session1(
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

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
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

    auto& config = session2.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    // Verify the change persisted
    EXPECT_EQ(*ports[0].description(), "Persistent change");
  }
}

TEST_F(ConfigSessionTestFixture, symlinkRollbackOnFailure) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Verify old symlink exists (created in SetUp)
  EXPECT_TRUE(fs::is_symlink(systemConfigPath_));
  EXPECT_EQ(
      fs::read_symlink(systemConfigPath_), cliConfigDir / "agent-r1.conf");

  // Setup mock agent server to fail reloadConfig on first call (the commit),
  // but succeed on second call (the rollback reload)
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig())
      .WillOnce(::testing::Throw(std::runtime_error("Reload failed")))
      .WillOnce(::testing::Return());

  // Create a ConfigSession and try to commit
  // systemConfigPath_ is already a symlink to agent-r1.conf created in SetUp()
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      cliConfigDir.string());

  auto& config = session.getAgentConfig();
  auto& ports = *config.sw()->ports();
  ASSERT_FALSE(ports.empty());
  ports[0].description() = "Failed change";
  session.setCommandLine("config interface eth1/1/1 description Failed change");
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  // Commit should fail and rollback the symlink
  EXPECT_THROW(session.commit(localhost()), std::runtime_error);

  // Verify symlink was rolled back to old target
  EXPECT_TRUE(fs::is_symlink(systemConfigPath_));
  EXPECT_EQ(
      fs::read_symlink(systemConfigPath_), cliConfigDir / "agent-r1.conf");

  // Verify session config still exists (not removed on failed commit)
  EXPECT_TRUE(fs::exists(sessionConfig));
}

TEST_F(ConfigSessionTestFixture, atomicRevisionCreation) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Setup mock agent server
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(2);

  // Run two concurrent commits to test atomic revision creation
  // Each thread uses a separate session config path (simulating different
  // users) Both threads will try to commit at the same time, and the atomic
  // file creation (O_CREAT | O_EXCL) should ensure they get different revision
  // numbers without conflicts
  std::atomic<int> revision1{0};
  std::atomic<int> revision2{0};

  auto commitTask = [&](const std::string& sessionName,
                        const std::string& description,
                        std::atomic<int>& rev) {
    fs::path sessionDir = testHomeDir_ / sessionName;
    fs::path sessionConfig = sessionDir / "agent.conf";

    TestableConfigSession session(
        sessionConfig.string(),
        systemConfigPath_.string(),
        cliConfigDir.string());

    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = description;
    session.setCommandLine(
        "config interface eth1/1/1 description " + description);
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    rev = session.commit(localhost()).revision;
  };

  std::thread thread1(
      commitTask, ".fboss2_user1", "First commit", std::ref(revision1));
  std::thread thread2(
      commitTask, ".fboss2_user2", "Second commit", std::ref(revision2));

  thread1.join();
  thread2.join();

  // Both commits should succeed with different revision numbers
  EXPECT_NE(revision1.load(), 0);
  EXPECT_NE(revision2.load(), 0);
  EXPECT_NE(revision1.load(), revision2.load());

  // Both should be either r2 or r3 (one gets r2, the other gets r3)
  EXPECT_TRUE(
      (revision1.load() == 2 && revision2.load() == 3) ||
      (revision1.load() == 3 && revision2.load() == 2));

  // Both revision files should exist
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));

  // Verify the content of each revision matches what was committed
  std::string r2Content = readFile(cliConfigDir / "agent-r2.conf");
  std::string r3Content = readFile(cliConfigDir / "agent-r3.conf");
  EXPECT_TRUE(
      (r2Content.find("First commit") != std::string::npos &&
       r3Content.find("Second commit") != std::string::npos) ||
      (r2Content.find("Second commit") != std::string::npos &&
       r3Content.find("First commit") != std::string::npos));
}

TEST_F(ConfigSessionTestFixture, concurrentSessionCreationSameUser) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Setup mock agent server
  // Either 1 or 2 commits might succeed depending on the race
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(testing::Between(1, 2));

  // Test concurrent session creation and commits for the SAME user
  // This tests the race conditions in:
  // 1. ensureDirectoryExists() - concurrent directory creation
  // 2. copySystemConfigToSession() - concurrent session file creation
  // 3. saveConfig() - concurrent writes to the same session file
  // 4. atomicSymlinkUpdate() - concurrent symlink updates
  //
  // Note: When two threads share the same session file, they race to modify it.
  // The atomic operations ensure no crashes or corruption. However, if one
  // thread commits and deletes the session files before the other thread
  // calls commit(), the second thread will get "No config session exists".
  // This is a valid race outcome - the important thing is no crashes.
  std::atomic<int> revision1{0};
  std::atomic<int> revision2{0};
  std::atomic<bool> thread1NoSession{false};
  std::atomic<bool> thread2NoSession{false};

  auto commitTask = [&](const std::string& description,
                        std::atomic<int>& rev,
                        std::atomic<bool>& noSession) {
    // Both threads use the SAME session path
    fs::path sessionDir = testHomeDir_ / ".fboss2_shared";
    fs::path sessionConfig = sessionDir / "agent.conf";

    TestableConfigSession session(
        sessionConfig.string(),
        systemConfigPath_.string(),
        cliConfigDir.string());

    auto& config = session.getAgentConfig();
    auto& ports = *config.sw()->ports();
    ASSERT_FALSE(ports.empty());
    ports[0].description() = description;
    session.setCommandLine(
        "config interface eth1/1/1 description " + description);
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

    try {
      rev = session.commit(localhost()).revision;
    } catch (const std::runtime_error& e) {
      // If the other thread already committed and deleted the session files,
      // we'll get "No config session exists" - this is a valid race outcome
      if (folly::StringPiece(e.what()).contains("No config session exists")) {
        noSession = true;
      } else {
        throw; // Re-throw unexpected errors
      }
    }
  };

  std::thread thread1(
      commitTask,
      "First commit",
      std::ref(revision1),
      std::ref(thread1NoSession));
  std::thread thread2(
      commitTask,
      "Second commit",
      std::ref(revision2),
      std::ref(thread2NoSession));

  thread1.join();
  thread2.join();

  // At least one commit should succeed
  bool commit1Succeeded = revision1.load() != 0;
  bool commit2Succeeded = revision2.load() != 0;
  EXPECT_TRUE(commit1Succeeded || commit2Succeeded);

  // If both succeeded, they should have different revision numbers
  if (commit1Succeeded && commit2Succeeded) {
    EXPECT_NE(revision1.load(), revision2.load());
    // Both should be either r2 or r3 (one gets r2, the other gets r3)
    EXPECT_TRUE(
        (revision1.load() == 2 && revision2.load() == 3) ||
        (revision1.load() == 3 && revision2.load() == 2));
    // Both revision files should exist
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));
  } else {
    // One thread got "No config session exists" because the other committed
    // first
    EXPECT_TRUE(thread1NoSession.load() || thread2NoSession.load());
    // The successful commit should be r2
    int successfulRevision =
        commit1Succeeded ? revision1.load() : revision2.load();
    EXPECT_EQ(successfulRevision, 2);
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  }

  // The history command would list all three revisions with their metadata
}

TEST_F(ConfigSessionTestFixture, revisionNumberExtraction) {
  // Test the revision number extraction logic
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create files with various revision numbers
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({})");
  createTestConfig(cliConfigDir / "agent-r42.conf", R"({})");
  createTestConfig(cliConfigDir / "agent-r999.conf", R"({})");

  // Verify files exist
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r42.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r999.conf"));

  // Test extractRevisionNumber() method
  EXPECT_EQ(
      ConfigSession::extractRevisionNumber(
          (cliConfigDir / "agent-r1.conf").string()),
      1);
  EXPECT_EQ(
      ConfigSession::extractRevisionNumber(
          (cliConfigDir / "agent-r42.conf").string()),
      42);
  EXPECT_EQ(
      ConfigSession::extractRevisionNumber(
          (cliConfigDir / "agent-r999.conf").string()),
      999);
}

TEST_F(ConfigSessionTestFixture, rollbackCreatesNewRevision) {
  // This test actually calls the rollback() method with a specific revision
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path symlinkPath = testEtcDir_ / "coop" / "agent.conf";
  fs::path sessionConfigPath = testHomeDir_ / ".fboss2" / "agent.conf";

  // Remove the regular file created by SetUp
  if (fs::exists(symlinkPath)) {
    fs::remove(symlinkPath);
  }

  // Create revision files (simulating previous commits)
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r2.conf", R"({"revision": 2})");
  createTestConfig(cliConfigDir / "agent-r3.conf", R"({"revision": 3})");

  // Create symlink pointing to r3 (current revision)
  fs::create_symlink(cliConfigDir / "agent-r3.conf", symlinkPath);

  // Verify initial state
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), cliConfigDir / "agent-r3.conf");

  // Setup mock agent server
  setupMockedAgentServer();

  // Expect reloadConfig to be called once
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(1);

  // Create a testable ConfigSession with test paths
  TestableConfigSession session(
      sessionConfigPath.string(), symlinkPath.string(), cliConfigDir.string());

  // Call the actual rollback method to rollback to r1
  int newRevision = session.rollback(localhost(), "r1");

  // Verify rollback created a new revision (r4)
  EXPECT_EQ(newRevision, 4);
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), cliConfigDir / "agent-r4.conf");
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r4.conf"));

  // Verify r4 has same content as r1 (the target revision)
  EXPECT_EQ(
      readFile(cliConfigDir / "agent-r1.conf"),
      readFile(cliConfigDir / "agent-r4.conf"));

  // Verify old revisions still exist (rollback doesn't delete history)
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));
}

TEST_F(ConfigSessionTestFixture, rollbackToPreviousRevision) {
  // This test actually calls the rollback() method without a revision argument
  // to rollback to the previous revision
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path symlinkPath = testEtcDir_ / "coop" / "agent.conf";
  fs::path sessionConfigPath = testHomeDir_ / ".fboss2" / "agent.conf";

  // Remove the regular file created by SetUp
  if (fs::exists(symlinkPath)) {
    fs::remove(symlinkPath);
  }

  // Create revision files (simulating previous commits)
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r2.conf", R"({"revision": 2})");
  createTestConfig(cliConfigDir / "agent-r3.conf", R"({"revision": 3})");

  // Create symlink pointing to r3 (current revision)
  fs::create_symlink(cliConfigDir / "agent-r3.conf", symlinkPath);

  // Verify initial state
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), cliConfigDir / "agent-r3.conf");

  // Setup mock agent server
  setupMockedAgentServer();

  // Expect reloadConfig to be called once
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(1);

  // Create a testable ConfigSession with test paths
  TestableConfigSession session(
      sessionConfigPath.string(), symlinkPath.string(), cliConfigDir.string());

  // Call the actual rollback method without a revision (should go to previous)
  int newRevision = session.rollback(localhost());

  // Verify rollback to previous revision created r4 with content from r2
  EXPECT_EQ(newRevision, 4);
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), cliConfigDir / "agent-r4.conf");
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r4.conf"));

  // Verify r4 has same content as r2 (the previous revision)
  EXPECT_EQ(
      readFile(cliConfigDir / "agent-r2.conf"),
      readFile(cliConfigDir / "agent-r4.conf"));

  // Verify old revisions still exist (rollback doesn't delete history)
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));
}

TEST_F(ConfigSessionTestFixture, actionLevelDefaultIsHitless) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

  // Default action level should be HITLESS
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::HITLESS);
}

TEST_F(ConfigSessionTestFixture, actionLevelUpdateAndGet) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

  // Update to AGENT_WARMBOOT
  session.updateRequiredAction(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  // Verify the action level was updated
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::AGENT_WARMBOOT);
}

TEST_F(ConfigSessionTestFixture, actionLevelHigherTakesPrecedence) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

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
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

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
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "conf_metadata.json";

  // Create a ConfigSession and set action level via saveConfig
  {
    TestableConfigSession session(
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

    // Load the config (required before saveConfig)
    session.getAgentConfig();
    session.setCommandLine("config interface eth1/1/1 description Test");
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
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "conf_metadata.json";

  // Create session directory and metadata file manually
  fs::create_directories(sessionDir);
  std::ofstream metaFile(metadataFile);
  // Use symbolic enum names for human readability
  metaFile << R"({"action":{"AGENT":"AGENT_WARMBOOT"}})";
  metaFile.close();

  // Also create the session config file (otherwise session will overwrite from
  // system)
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Create a ConfigSession - should load action level from metadata file
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

  // Verify action level was loaded
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::AGENT_WARMBOOT);
}

TEST_F(ConfigSessionTestFixture, actionLevelPersistsAcrossSessions) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // First session: set action level via saveConfig
  {
    TestableConfigSession session1(
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

    // Load the config (required before saveConfig)
    session1.getAgentConfig();
    session1.setCommandLine("config interface eth1/1/1 description Test");
    session1.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);
  }

  // Second session: verify action level was persisted
  {
    TestableConfigSession session2(
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

    EXPECT_EQ(
        session2.getRequiredAction(cli::ServiceType::AGENT),
        cli::ConfigActionLevel::AGENT_WARMBOOT);
  }
}

TEST_F(ConfigSessionTestFixture, commandTrackingBasic) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "conf_metadata.json";

  // Create a ConfigSession, execute command, and verify persistence
  {
    TestableConfigSession session(
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

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
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

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
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // First session: execute some commands
  {
    TestableConfigSession session1(
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

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
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());

    EXPECT_EQ(2, session2.getCommands().size());
    EXPECT_EQ("config interface eth1/1/1 mtu 9000", session2.getCommands()[0]);
    EXPECT_EQ(
        "config interface eth1/1/1 description Test",
        session2.getCommands()[1]);
  }
}

TEST_F(ConfigSessionTestFixture, commandTrackingClearedOnReset) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create a ConfigSession and add some commands
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

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
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "conf_metadata.json";

  // Create session directory and metadata file manually
  fs::create_directories(sessionDir);
  std::ofstream metaFile(metadataFile);
  metaFile << R"({
    "action": {"AGENT": "HITLESS"},
    "commands": ["cmd1", "cmd2", "cmd3"]
  })";
  metaFile.close();

  // Also create the session config file
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Create a ConfigSession - should load commands from metadata file
  TestableConfigSession session(
      sessionConfig.string(),
      systemConfigPath_.string(),
      (testEtcDir_ / "coop" / "cli").string());

  // Verify commands were loaded
  EXPECT_EQ(3, session.getCommands().size());
  EXPECT_EQ("cmd1", session.getCommands()[0]);
  EXPECT_EQ("cmd2", session.getCommands()[1]);
  EXPECT_EQ("cmd3", session.getCommands()[2]);
}

} // namespace facebook::fboss
