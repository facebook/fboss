// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for config session commit with agent restart.
 *
 * Verifies that:
 * - Commands requiring AGENT_WARMBOOT (e.g. switchport access vlan) trigger a
 *   warmboot on commit and the agent comes back healthy.
 * - Commands requiring AGENT_COLDBOOT (e.g. l2 learning-mode) trigger a
 *   coldboot on commit and the agent comes back healthy.
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 * - These tests restart the agent and are therefore disruptive; they should
 *   only be run in environments where a transient agent restart is acceptable.
 */

#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

// ============================================================
// Warmboot test
// Uses 'config interface <name> switchport access vlan <id>'
// which records AGENT_WARMBOOT, so commit restarts the agent
// via warmboot and reports it in the commit message.
// ============================================================
class ConfigSessionWarmbootTest : public Fboss2IntegrationTest {};

TEST_F(ConfigSessionWarmbootTest, CommitTriggersWarmboot) {
  // Step 1: Find a suitable interface with a known VLAN
  XLOG(INFO) << "[Step 1] Finding an interface with a VLAN...";
  Interface intf = findFirstEthInterface();
  ASSERT_TRUE(intf.vlan.has_value()) << "Interface must have a VLAN";
  int originalVlan = *intf.vlan;
  // Pick a different VLAN so the config actually changes
  int testVlan = (originalVlan == 4094) ? originalVlan - 1 : originalVlan + 1;
  XLOG(INFO) << "  Using interface: " << intf.name << " (VLAN: " << originalVlan
             << " -> " << testVlan << ")";

  // Step 2: Set access VLAN to a different value (records AGENT_WARMBOOT)
  XLOG(INFO) << "[Step 2] Setting access VLAN to " << testVlan << "...";
  auto result = runCli(
      {"config",
       "interface",
       intf.name,
       "switchport",
       "access",
       "vlan",
       std::to_string(testVlan)});
  ASSERT_EQ(result.exitCode, 0) << "Failed to set VLAN: " << result.stderr;

  // Step 3: Commit - should trigger warmboot
  XLOG(INFO) << "[Step 3] Committing (expect warmboot)...";
  result = runCli({"config", "session", "commit"});
  ASSERT_EQ(result.exitCode, 0) << "Commit failed: " << result.stderr;
  XLOG(INFO) << "  Commit output: " << result.stdout;
  EXPECT_THAT(result.stdout, HasSubstr("warmboot"))
      << "Expected 'warmboot' in commit output";

  // Step 4: Wait for agent to come back after warmboot
  XLOG(INFO) << "[Step 4] Waiting for agent to recover from warmboot...";
  waitForAgentReady();

  // Step 5: Restore original VLAN
  XLOG(INFO) << "[Step 5] Restoring original VLAN (" << originalVlan << ")...";
  result = runCli(
      {"config",
       "interface",
       intf.name,
       "switchport",
       "access",
       "vlan",
       std::to_string(originalVlan)});
  ASSERT_EQ(result.exitCode, 0) << "Failed to restore VLAN: " << result.stderr;
  commitConfig();
  waitForAgentReady();
  XLOG(INFO) << "  Restored original VLAN";

  XLOG(INFO) << "TEST PASSED";
}

// ============================================================
// Coldboot test
// Uses 'config l2 learning-mode' which records AGENT_COLDBOOT,
// so commit restarts the agent via coldboot and reports it in
// the commit message.
// ============================================================
class ConfigSessionColdbootTest : public Fboss2IntegrationTest {};

TEST_F(ConfigSessionColdbootTest, CommitTriggersColdboot) {
  // Step 1: Change L2 learning mode to a different value (records
  // AGENT_COLDBOOT) Try 'software' first; if already software, use 'hardware'
  // instead.
  XLOG(INFO) << "[Step 1] Changing L2 learning mode...";
  auto result = runCli({"config", "l2", "learning-mode", "software"});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;

  std::string restoreMode;
  if (result.stdout.find("already") != std::string::npos) {
    // Already software - switch to hardware instead
    XLOG(INFO) << "  Already 'software', switching to 'hardware'";
    discardSession();
    result = runCli({"config", "l2", "learning-mode", "hardware"});
    ASSERT_EQ(result.exitCode, 0) << result.stderr;
    restoreMode = "software";
  } else {
    restoreMode = "hardware";
  }
  XLOG(INFO) << "  L2 learning mode changed (will restore to '" << restoreMode
             << "')";

  // Step 2: Commit - should trigger coldboot
  XLOG(INFO) << "[Step 2] Committing (expect coldboot)...";
  result = runCli({"config", "session", "commit"});
  ASSERT_EQ(result.exitCode, 0) << "Commit failed: " << result.stderr;
  XLOG(INFO) << "  Commit output: " << result.stdout;
  EXPECT_THAT(result.stdout, HasSubstr("coldboot"))
      << "Expected 'coldboot' in commit output";

  // Step 3: Wait for agent to come back after coldboot
  XLOG(INFO) << "[Step 3] Waiting for agent to recover from coldboot...";
  waitForAgentReady();

  // Step 4: Restore original L2 learning mode
  XLOG(INFO) << "[Step 4] Restoring L2 learning mode to '" << restoreMode
             << "'...";
  result = runCli({"config", "l2", "learning-mode", restoreMode});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  if (result.stdout.find("already") == std::string::npos) {
    commitConfig();
    waitForAgentReady();
  }
  XLOG(INFO) << "  Restored L2 learning mode";

  XLOG(INFO) << "TEST PASSED";
}
