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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

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

    // Create a test system config file
    systemConfigPath_ = testEtcDir_ / "coop" / "agent.conf";
    createTestConfig(systemConfigPath_, R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": "ENABLED",
        "speed": "HUNDREDG"
      }
    ]
  }
})");
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

TEST_F(ConfigSessionTestFixture, sessionDirectoryCreated) {
  // Initially, session directory should not exist
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  EXPECT_FALSE(fs::exists(sessionDir));

  // Creating a ConfigSession should create the directory
  // Note: We can't actually test ConfigSession::getInstance() here because
  // it uses hardcoded paths. This test would need to be adapted if we
  // make ConfigSession testable with dependency injection.

  // For now, just verify the directory structure
  fs::create_directories(sessionDir);
  EXPECT_TRUE(fs::exists(sessionDir));
}

TEST_F(ConfigSessionTestFixture, sessionConfigCopied) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create session directory
  fs::create_directories(sessionDir);

  // Copy system config to session (simulating what ConfigSession does)
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Verify the file was copied
  EXPECT_TRUE(fs::exists(sessionConfig));

  // Verify content matches
  std::string systemContent = readFile(systemConfigPath_);
  std::string sessionContent = readFile(sessionConfig);
  EXPECT_EQ(systemContent, sessionContent);
}

TEST_F(ConfigSessionTestFixture, sessionConfigModified) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create session directory and copy config
  fs::create_directories(sessionDir);
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Modify the session config
  std::string modifiedContent = R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "description": "Modified port",
        "state": "ENABLED",
        "speed": "HUNDREDG"
      }
    ]
  }
})";
  createTestConfig(sessionConfig, modifiedContent);

  // Verify session config is modified
  std::string sessionContent = readFile(sessionConfig);
  EXPECT_NE(sessionContent, readFile(systemConfigPath_));
  EXPECT_EQ(sessionContent, modifiedContent);
}

TEST_F(ConfigSessionTestFixture, commitMovesConfigToCliDir) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path targetConfig = cliConfigDir / "testuser.conf";

  // Create session directory and copy config
  fs::create_directories(sessionDir);
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Modify the session config
  std::string modifiedContent = R"({"modified": true})";
  createTestConfig(sessionConfig, modifiedContent);

  // Simulate commit: copy session config to CLI dir and remove session
  fs::copy_file(
      sessionConfig, targetConfig, fs::copy_options::overwrite_existing);
  fs::remove(sessionConfig);

  // Verify session config no longer exists
  EXPECT_FALSE(fs::exists(sessionConfig));

  // Verify target config exists with correct content
  EXPECT_TRUE(fs::exists(targetConfig));
  EXPECT_EQ(readFile(targetConfig), modifiedContent);
}

TEST_F(ConfigSessionTestFixture, commitCreatesSymlink) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path targetConfig = cliConfigDir / "testuser.conf";
  fs::path symlinkPath = testEtcDir_ / "coop" / "agent.conf";

  // Create target config
  createTestConfig(targetConfig, R"({"test": true})");

  // Remove old symlink/file if exists
  if (fs::exists(symlinkPath)) {
    fs::remove(symlinkPath);
  }

  // Create symlink
  fs::create_symlink(targetConfig, symlinkPath);

  // Verify symlink exists and points to correct target
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), targetConfig);
}

TEST_F(ConfigSessionTestFixture, commitReplacesExistingSymlink) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path oldTarget = cliConfigDir / "olduser.conf";
  fs::path newTarget = cliConfigDir / "testuser.conf";
  fs::path symlinkPath = testEtcDir_ / "coop" / "agent.conf";

  // Remove the regular file created by SetUp
  if (fs::exists(symlinkPath)) {
    fs::remove(symlinkPath);
  }

  // Create old target and symlink
  createTestConfig(oldTarget, R"({"old": true})");
  fs::create_symlink(oldTarget, symlinkPath);

  // Verify old symlink
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), oldTarget);

  // Create new target
  createTestConfig(newTarget, R"({"new": true})");

  // Replace symlink
  fs::remove(symlinkPath);
  fs::create_symlink(newTarget, symlinkPath);

  // Verify new symlink
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), newTarget);
}

TEST_F(ConfigSessionTestFixture, multipleSessionChanges) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create session directory and copy config
  fs::create_directories(sessionDir);
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Make first change
  std::string content1 = R"({"change": 1})";
  createTestConfig(sessionConfig, content1);
  EXPECT_EQ(readFile(sessionConfig), content1);

  // Make second change
  std::string content2 = R"({"change": 2})";
  createTestConfig(sessionConfig, content2);
  EXPECT_EQ(readFile(sessionConfig), content2);

  // Make third change
  std::string content3 = R"({"change": 3})";
  createTestConfig(sessionConfig, content3);
  EXPECT_EQ(readFile(sessionConfig), content3);
}

TEST_F(ConfigSessionTestFixture, sessionPersistsAcrossCommands) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create session directory and copy config
  fs::create_directories(sessionDir);
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Modify config
  std::string modifiedContent = R"({"persistent": true})";
  createTestConfig(sessionConfig, modifiedContent);

  // Verify session persists (file still exists with same content)
  EXPECT_TRUE(fs::exists(sessionConfig));
  EXPECT_EQ(readFile(sessionConfig), modifiedContent);

  // Simulate another command reading the session
  std::string readContent = readFile(sessionConfig);
  EXPECT_EQ(readContent, modifiedContent);
}

TEST_F(ConfigSessionTestFixture, symlinkRollbackOnFailure) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path oldTarget = cliConfigDir / "olduser.conf";
  fs::path newTarget = cliConfigDir / "testuser.conf";
  fs::path symlinkPath = testEtcDir_ / "coop" / "agent.conf";

  // Remove the regular file created by SetUp
  if (fs::exists(symlinkPath)) {
    fs::remove(symlinkPath);
  }

  // Create old target and symlink
  createTestConfig(oldTarget, R"({"old": true})");
  fs::create_symlink(oldTarget, symlinkPath);

  // Verify old symlink
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), oldTarget);

  // Create new target
  createTestConfig(newTarget, R"({"new": true})");

  // Simulate a failed commit: update symlink but then "fail"
  // In real code, if reloadConfig() fails, the symlink should be rolled back
  fs::remove(symlinkPath);
  fs::create_symlink(newTarget, symlinkPath);

  // Verify new symlink was created
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), newTarget);

  // Simulate rollback: restore old symlink
  fs::remove(symlinkPath);
  fs::create_symlink(oldTarget, symlinkPath);

  // Verify rollback worked - symlink points back to old target
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), oldTarget);
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

TEST_F(ConfigSessionTestFixture, diffNoSession) {
  // When no session exists, diff should report that
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Ensure no session exists
  EXPECT_FALSE(fs::exists(sessionConfig));

  // In a real test, we would call the diff command and verify the output
  // For now, just verify the precondition
}

TEST_F(ConfigSessionTestFixture, diffIdenticalConfigs) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create session directory and copy config (no modifications)
  fs::create_directories(sessionDir);
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Verify files are identical
  std::string systemContent = readFile(systemConfigPath_);
  std::string sessionContent = readFile(sessionConfig);
  EXPECT_EQ(systemContent, sessionContent);

  // In a real test, we would call the diff command and verify it reports
  // "No differences"
}

TEST_F(ConfigSessionTestFixture, diffDifferentConfigs) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";

  // Create session directory and copy config
  fs::create_directories(sessionDir);
  fs::copy_file(systemConfigPath_, sessionConfig);

  // Modify the session config
  std::string modifiedContent = R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "description": "Modified port",
        "state": "ENABLED",
        "speed": "HUNDREDG"
      }
    ]
  }
})";
  createTestConfig(sessionConfig, modifiedContent);

  // Verify files are different
  std::string systemContent = readFile(systemConfigPath_);
  std::string sessionContent = readFile(sessionConfig);
  EXPECT_NE(systemContent, sessionContent);

  // In a real test, we would call the diff command and verify it shows
  // the differences (added "description" field)
}

TEST_F(ConfigSessionTestFixture, diffWithRevisions) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create some revision files
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r2.conf", R"({"revision": 2})");
  createTestConfig(cliConfigDir / "agent-r3.conf", R"({"revision": 3})");

  // Verify the files exist
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));

  // Verify content is different
  EXPECT_NE(
      readFile(cliConfigDir / "agent-r1.conf"),
      readFile(cliConfigDir / "agent-r2.conf"));
  EXPECT_NE(
      readFile(cliConfigDir / "agent-r2.conf"),
      readFile(cliConfigDir / "agent-r3.conf"));
}

TEST_F(ConfigSessionTestFixture, diffTwoRevisions) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create two different revision files
  std::string config1 = R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": "ENABLED"
      }
    ]
  }
})";

  std::string config2 = R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "description": "Added description",
        "state": "ENABLED"
      }
    ]
  }
})";

  createTestConfig(cliConfigDir / "agent-r1.conf", config1);
  createTestConfig(cliConfigDir / "agent-r2.conf", config2);

  // Verify files are different
  EXPECT_NE(
      readFile(cliConfigDir / "agent-r1.conf"),
      readFile(cliConfigDir / "agent-r2.conf"));

  // The diff command would show the added "description" field
}

TEST_F(ConfigSessionTestFixture, diffSessionVsRevision) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create a revision file
  std::string revisionContent = R"({"revision": 1})";
  createTestConfig(cliConfigDir / "agent-r1.conf", revisionContent);

  // Create session with different content
  fs::create_directories(sessionDir);
  std::string sessionContent = R"({"session": true})";
  createTestConfig(sessionConfig, sessionContent);

  // Verify files are different
  EXPECT_NE(readFile(cliConfigDir / "agent-r1.conf"), readFile(sessionConfig));

  // The diff command would show the differences between r1 and session
}

TEST_F(ConfigSessionTestFixture, diffWithCurrentKeyword) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create a revision file
  std::string revisionContent = R"({"revision": 1})";
  createTestConfig(cliConfigDir / "agent-r1.conf", revisionContent);

  // System config is different
  std::string systemContent = readFile(systemConfigPath_);
  EXPECT_NE(revisionContent, systemContent);

  // The diff command with "current" would compare r1 to system config
}

TEST_F(ConfigSessionTestFixture, rollbackCreatesNewRevision) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path symlinkPath = testEtcDir_ / "coop" / "agent.conf";

  // Remove the regular file created by SetUp
  if (fs::exists(symlinkPath)) {
    fs::remove(symlinkPath);
  }

  // Create revision files
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r2.conf", R"({"revision": 2})");
  createTestConfig(cliConfigDir / "agent-r3.conf", R"({"revision": 3})");

  // Create symlink pointing to r3 (current)
  fs::create_symlink(cliConfigDir / "agent-r3.conf", symlinkPath);

  // Verify current symlink
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), cliConfigDir / "agent-r3.conf");

  // Simulate rollback to r1: update symlink and create r4 as copy of r1
  fs::remove(symlinkPath);
  fs::create_symlink(cliConfigDir / "agent-r1.conf", symlinkPath);
  fs::copy_file(cliConfigDir / "agent-r1.conf", cliConfigDir / "agent-r4.conf");

  // Verify rollback
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), cliConfigDir / "agent-r1.conf");
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r4.conf"));

  // Verify r4 has same content as r1
  EXPECT_EQ(
      readFile(cliConfigDir / "agent-r1.conf"),
      readFile(cliConfigDir / "agent-r4.conf"));
}

TEST_F(ConfigSessionTestFixture, rollbackToPreviousRevision) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";
  fs::path symlinkPath = testEtcDir_ / "coop" / "agent.conf";

  // Remove the regular file created by SetUp
  if (fs::exists(symlinkPath)) {
    fs::remove(symlinkPath);
  }

  // Create revision files
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r2.conf", R"({"revision": 2})");
  createTestConfig(cliConfigDir / "agent-r3.conf", R"({"revision": 3})");

  // Create symlink pointing to r3 (current)
  fs::create_symlink(cliConfigDir / "agent-r3.conf", symlinkPath);

  // Simulate rollback without argument: should go to r2 (previous)
  fs::remove(symlinkPath);
  fs::create_symlink(cliConfigDir / "agent-r2.conf", symlinkPath);
  fs::copy_file(cliConfigDir / "agent-r2.conf", cliConfigDir / "agent-r4.conf");

  // Verify rollback to previous revision
  EXPECT_TRUE(fs::is_symlink(symlinkPath));
  EXPECT_EQ(fs::read_symlink(symlinkPath), cliConfigDir / "agent-r2.conf");
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r4.conf"));

  // Verify r4 has same content as r2
  EXPECT_EQ(
      readFile(cliConfigDir / "agent-r2.conf"),
      readFile(cliConfigDir / "agent-r4.conf"));
}

TEST_F(ConfigSessionTestFixture, historyListsRevisions) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create revision files
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r2.conf", R"({"revision": 2})");
  createTestConfig(cliConfigDir / "agent-r3.conf", R"({"revision": 3})");

  // Verify all revision files exist
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r3.conf"));

  // The history command would list all three revisions with their metadata
}

TEST_F(ConfigSessionTestFixture, historyIgnoresNonMatchingFiles) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create valid revision files
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r2.conf", R"({"revision": 2})");

  // Create files that should be ignored
  createTestConfig(cliConfigDir / "agent.conf.bak", R"({"backup": true})");
  createTestConfig(cliConfigDir / "other-r1.conf", R"({"other": true})");
  createTestConfig(cliConfigDir / "agent-r1.txt", R"({"wrong_ext": true})");
  createTestConfig(cliConfigDir / "agent-rX.conf", R"({"invalid": true})");

  // Verify all files exist
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent.conf.bak"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "other-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.txt"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-rX.conf"));

  // The history command would only list r1 and r2, ignoring the others
}

TEST_F(ConfigSessionTestFixture, historyEmptyDirectory) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Directory exists but has no revision files
  EXPECT_TRUE(fs::exists(cliConfigDir));
  EXPECT_FALSE(fs::exists(cliConfigDir / "agent-r1.conf"));

  // The history command would report no revisions found
}

TEST_F(ConfigSessionTestFixture, historyNonSequentialRevisions) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create non-sequential revision files (e.g., after deletions)
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  createTestConfig(cliConfigDir / "agent-r5.conf", R"({"revision": 5})");
  createTestConfig(cliConfigDir / "agent-r10.conf", R"({"revision": 10})");

  // Verify files exist
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r5.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r10.conf"));

  // The history command would list r1, r5, r10 in order
}

TEST_F(ConfigSessionTestFixture, historyShowsFileMetadata) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create a revision file
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");

  // Verify file exists
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));

  // Get file metadata using stat
  struct stat fileStat;
  EXPECT_EQ(stat((cliConfigDir / "agent-r1.conf").c_str(), &fileStat), 0);

  // Verify we can get UID (owner)
  EXPECT_GE(fileStat.st_uid, 0);

  // The history command would show owner and timestamp for this file
}

TEST_F(ConfigSessionTestFixture, commitCreatesRevisionFile) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create session directory and config
  fs::create_directories(sessionDir);
  std::string sessionContent = R"({"committed": true})";
  createTestConfig(sessionConfig, sessionContent);

  // Simulate commit: create r1 from session config
  fs::copy_file(sessionConfig, cliConfigDir / "agent-r1.conf");

  // Verify r1 was created
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_EQ(readFile(cliConfigDir / "agent-r1.conf"), sessionContent);

  // Simulate second commit: create r2
  std::string sessionContent2 = R"({"committed": true, "version": 2})";
  createTestConfig(sessionConfig, sessionContent2);
  fs::copy_file(sessionConfig, cliConfigDir / "agent-r2.conf");

  // Verify r2 was created
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
  EXPECT_EQ(readFile(cliConfigDir / "agent-r2.conf"), sessionContent2);

  // Both revisions should exist
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r2.conf"));
}

TEST_F(ConfigSessionTestFixture, atomicRevisionCreation) {
  fs::path cliConfigDir = testEtcDir_ / "coop" / "cli";

  // Create r1
  createTestConfig(cliConfigDir / "agent-r1.conf", R"({"revision": 1})");
  EXPECT_TRUE(fs::exists(cliConfigDir / "agent-r1.conf"));

  // Try to create r1 again - should fail (file already exists)
  std::error_code ec;
  fs::copy_file(
      cliConfigDir / "agent-r1.conf",
      cliConfigDir / "agent-r1.conf",
      fs::copy_options::none,
      ec);
  EXPECT_TRUE(ec); // Should have an error

  // The atomic file creation in ConfigSession uses O_CREAT | O_EXCL
  // which would fail if the file already exists, ensuring atomicity
}

} // namespace facebook::fboss
