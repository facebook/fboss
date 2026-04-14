/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/filesystem/operations.hpp>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <filesystem>
#include <fstream>

#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionClear.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

class ConfigSessionClearTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directory for each test to avoid conflicts when
    // running tests in parallel
    auto tempBase = fs::temp_directory_path();
    auto uniquePath =
        boost::filesystem::unique_path("fboss_clear_test_%%%%-%%%%-%%%%-%%%%");
    testHomeDir_ = tempBase / (uniquePath.string() + "_home");

    // Clean up any previous test artifacts (shouldn't exist with unique names)
    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }

    // Create test directory
    fs::create_directories(testHomeDir_);

    // Set environment variable so ConfigSession uses our test directory
    // NOLINTNEXTLINE(concurrency-mt-unsafe) - acceptable in unit tests
    setenv("HOME", testHomeDir_.c_str(), 1);
  }

  void TearDown() override {
    // Clean up test directory
    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }

    CmdHandlerTestBase::TearDown();
  }

 protected:
  void createTestFile(const fs::path& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
    file.close();
  }

  fs::path testHomeDir_;
};

TEST_F(ConfigSessionClearTestFixture, clearExistingSession) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "cli_metadata.json";

  // Create session files manually
  fs::create_directories(sessionDir);
  createTestFile(sessionConfig, R"({"sw": {"ports": []}})");
  createTestFile(metadataFile, R"({"action":{"WEDGE_AGENT":"HITLESS"}})");

  // Verify session files exist
  EXPECT_TRUE(fs::exists(sessionConfig));
  EXPECT_TRUE(fs::exists(metadataFile));

  // Create the clear command and execute it
  CmdConfigSessionClear cmd;
  auto result = cmd.queryClient(localhost());

  // Verify the result message
  EXPECT_EQ(result, "Config session cleared successfully.");

  // Verify session files were removed
  EXPECT_FALSE(fs::exists(sessionConfig));
  EXPECT_FALSE(fs::exists(metadataFile));

  // Verify session directory still exists (we only remove files, not the dir)
  EXPECT_TRUE(fs::exists(sessionDir));
}

TEST_F(ConfigSessionClearTestFixture, clearWhenNoSessionExists) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "cli_metadata.json";

  // Ensure no session files exist
  EXPECT_FALSE(fs::exists(sessionConfig));
  EXPECT_FALSE(fs::exists(metadataFile));

  // Create the clear command and execute it
  CmdConfigSessionClear cmd;
  auto result = cmd.queryClient(localhost());

  // Verify the result message indicates nothing to clear
  EXPECT_EQ(result, "No config session exists. Nothing to clear.");
}

TEST_F(ConfigSessionClearTestFixture, clearOnlyConfigFile) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "cli_metadata.json";

  // Create only the config file (not metadata)
  fs::create_directories(sessionDir);
  createTestFile(sessionConfig, R"({"sw": {"ports": []}})");

  // Verify only config file exists
  EXPECT_TRUE(fs::exists(sessionConfig));
  EXPECT_FALSE(fs::exists(metadataFile));

  // Create the clear command and execute it
  CmdConfigSessionClear cmd;
  auto result = cmd.queryClient(localhost());

  // Verify the result message
  EXPECT_EQ(result, "Config session cleared successfully.");

  // Verify config file was removed
  EXPECT_FALSE(fs::exists(sessionConfig));
}

TEST_F(ConfigSessionClearTestFixture, clearOnlyMetadataFile) {
  fs::path sessionDir = testHomeDir_ / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path metadataFile = sessionDir / "cli_metadata.json";

  // Create only the metadata file (not config)
  fs::create_directories(sessionDir);
  createTestFile(metadataFile, R"({"action":{"WEDGE_AGENT":"HITLESS"}})");

  // Verify only metadata file exists
  EXPECT_FALSE(fs::exists(sessionConfig));
  EXPECT_TRUE(fs::exists(metadataFile));

  // Create the clear command and execute it
  CmdConfigSessionClear cmd;
  auto result = cmd.queryClient(localhost());

  // Verify the result message
  EXPECT_EQ(result, "Config session cleared successfully.");

  // Verify metadata file was removed
  EXPECT_FALSE(fs::exists(metadataFile));
}

} // namespace facebook::fboss
