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
  session.saveConfig();

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
    session.saveConfig();

    // Commit the session
    int revision = session.commit(localhost());

    // Verify session config no longer exists (removed after commit)
    EXPECT_FALSE(fs::exists(sessionConfig));

    // Verify new revision was created in cli directory
    EXPECT_EQ(revision, 2);
    fs::path targetConfig = cliConfigDir / "agent-r2.conf";
    EXPECT_TRUE(fs::exists(targetConfig));
    EXPECT_THAT(readFile(targetConfig), ::testing::HasSubstr("First commit"));

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
    session.saveConfig();

    // Commit the second change
    int revision = session.commit(localhost());

    // Verify new revision was created
    EXPECT_EQ(revision, 3);
    fs::path targetConfig = cliConfigDir / "agent-r3.conf";
    EXPECT_TRUE(fs::exists(targetConfig));
    EXPECT_THAT(readFile(targetConfig), ::testing::HasSubstr("Second commit"));

    // Verify symlink was updated to point to r3
    EXPECT_TRUE(fs::is_symlink(systemConfigPath_));
    EXPECT_EQ(fs::read_symlink(systemConfigPath_), targetConfig);

    // Verify all revisions exist
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
    EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));
  }
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
  session.saveConfig();
  EXPECT_THAT(readFile(sessionConfig), ::testing::HasSubstr("Change 1"));

  // Make second change
  ports[0].description() = "Change 2";
  session.saveConfig();
  EXPECT_THAT(readFile(sessionConfig), ::testing::HasSubstr("Change 2"));

  // Make third change
  ports[0].description() = "Change 3";
  session.saveConfig();
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
    session1.saveConfig();
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

  // Setup mock agent server to fail reloadConfig
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig())
      .WillOnce(::testing::Throw(std::runtime_error("Reload failed")));

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
  session.saveConfig();

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
    session.saveConfig();

    rev = session.commit(localhost());
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
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), reloadConfig()).Times(2);

  // Test concurrent session creation and commits for the SAME user
  // This tests the race conditions in:
  // 1. ensureDirectoryExists() - concurrent directory creation
  // 2. copySystemConfigToSession() - concurrent session file creation
  // 3. saveConfig() - concurrent writes to the same session file
  // 4. atomicSymlinkUpdate() - concurrent symlink updates
  //
  // Note: When two threads share the same session file, they race to modify it.
  // The atomic operations ensure no crashes or corruption, but both commits
  // might have the same content if one thread's saveConfig() overwrites the
  // other's changes. This is expected behavior - the important thing is that
  // both commits succeed without crashes.
  std::atomic<int> revision1{0};
  std::atomic<int> revision2{0};

  auto commitTask = [&](const std::string& description, std::atomic<int>& rev) {
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
    session.saveConfig();

    rev = session.commit(localhost());
  };

  std::thread thread1(commitTask, "First commit", std::ref(revision1));
  std::thread thread2(commitTask, "Second commit", std::ref(revision2));

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

} // namespace facebook::fboss
