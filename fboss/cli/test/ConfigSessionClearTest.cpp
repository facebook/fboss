// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2-dev config session clear'
 *
 * This test:
 * 1. Creates a config session by making a config change
 * 2. Verifies the session exists
 * 3. Clears the session using 'config session clear'
 * 4. Verifies the session was cleared
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include "fboss/cli/test/CliTest.h"

namespace fs = std::filesystem;

using namespace facebook::fboss;

class ConfigSessionClearTest : public CliTest {
 protected:
  fs::path getSessionConfigPath() const {
    // NOLINTNEXTLINE(concurrency-mt-unsafe): HOME is read-only in practice
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
      throw std::runtime_error("HOME environment variable not set");
    }
    return fs::path(home) / ".fboss2" / "agent.conf";
  }

  fs::path getSessionMetadataPath() const {
    // NOLINTNEXTLINE(concurrency-mt-unsafe): HOME is read-only in practice
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
      throw std::runtime_error("HOME environment variable not set");
    }
    return fs::path(home) / ".fboss2" / "cli_metadata.json";
  }

  bool sessionExists() const {
    return fs::exists(getSessionConfigPath());
  }
};

TEST_F(ConfigSessionClearTest, CreateAndClearSession) {
  // Step 1: Find an interface to make a config change
  XLOG(INFO) << "[Step 1] Finding an interface to create a session...";
  Interface interface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << interface.name;

  // Step 2: Make a config change to create a session
  XLOG(INFO) << "[Step 2] Making a config change to create a session...";
  std::string testDescription = "CLI_E2E_SESSION_CLEAR_TEST";
  auto result = runCli(
      {"config", "interface", interface.name, "description", testDescription});
  ASSERT_EQ(result.exitCode, 0)
      << "Failed to set description: " << result.stderr;
  XLOG(INFO) << "  Config change made successfully";

  // Step 3: Verify session files exist
  XLOG(INFO) << "[Step 3] Verifying session exists...";
  EXPECT_TRUE(sessionExists()) << "Session config file should exist";
  XLOG(INFO) << "  Session exists: " << getSessionConfigPath().string();

  // Step 4: Clear the session
  XLOG(INFO) << "[Step 4] Clearing the session...";
  result = runCli({"config", "session", "clear"});
  ASSERT_EQ(result.exitCode, 0) << "Failed to clear session: " << result.stderr;
  XLOG(INFO) << "  Session cleared successfully";

  // Step 5: Verify session files are removed
  XLOG(INFO) << "[Step 5] Verifying session was cleared...";
  EXPECT_FALSE(sessionExists()) << "Session config file should be removed";
  EXPECT_FALSE(fs::exists(getSessionMetadataPath()))
      << "Session metadata file should be removed";
  XLOG(INFO) << "  Session files removed";

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigSessionClearTest, ClearNonExistentSession) {
  // Step 1: Ensure no session exists by clearing any existing session
  XLOG(INFO) << "[Step 1] Ensuring no session exists...";
  if (sessionExists()) {
    auto result = runCli({"config", "session", "clear"});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to clear existing session: " << result.stderr;
  }
  ASSERT_FALSE(sessionExists()) << "Session should not exist";
  XLOG(INFO) << "  No session exists";

  // Step 2: Try to clear non-existent session
  XLOG(INFO) << "[Step 2] Clearing non-existent session...";
  auto result = runCli({"config", "session", "clear"});
  ASSERT_EQ(result.exitCode, 0)
      << "Clear should succeed even with no session: " << result.stderr;
  XLOG(INFO) << "  Command succeeded";

  // Step 3: Verify output message
  XLOG(INFO) << "[Step 3] Verifying output message...";
  EXPECT_TRUE(
      result.stdout.find("No config session exists") != std::string::npos)
      << "Expected 'No config session exists' message, got: " << result.stdout;
  XLOG(INFO) << "  Output: " << result.stdout;

  XLOG(INFO) << "TEST PASSED";
}
