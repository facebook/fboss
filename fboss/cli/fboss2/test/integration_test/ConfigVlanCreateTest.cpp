// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for VLAN static MAC configuration and VlanManager behavior.
 *
 * Tests:
 *
 * 1. StaticMacAddDeleteOnExistingVlan
 *    - Add a static MAC entry to an existing VLAN (discovered dynamically)
 *    - Commit and verify
 *    - Delete the entry and commit
 *    This is the primary use case for static-mac add/delete.
 *
 * 2. AutoCreateVlanInSession
 *    - Ensure VLAN 3999 does not exist (cleanup from any prior run)
 *    - Add a static MAC entry to non-existent VLAN 3999
 *    - VlanManager auto-creates VLAN 3999 and a barebone cfg::Interface
 *    - Verify "Created VLAN" message appears on first command
 *    - Verify second command does NOT print the creation message again
 *    - Commit the session (both VLAN and interface are present, so the agent
 *      accepts the commit)
 *    - Clean up: delete the static MAC entry and commit
 *    - VLAN 3999 + interface remain (no delete-VLAN command yet); next run
 *      starts by removing them for idempotency
 *
 * 3. DeleteOnNonExistentVlanIsIdempotent
 *    - Delete on a VLAN that doesn't exist returns "does not exist"
 *    - No VLAN is auto-created for a delete operation
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 * - Tests that need an existing L2 VLAN skip on pure routed-mode DUTs
 */

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

namespace fs = std::filesystem;

using namespace facebook::fboss;

class ConfigVlanCreateTest : public Fboss2IntegrationTest {
 protected:
  // Discover an existing VLAN + port before each test. existingVlanId_ stays
  // -1 on pure routed-mode DUTs (no L2 VLANs).
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    if (auto vp = findConfiguredVlanPort()) {
      existingVlanId_ = vp->first;
      existingVlanPort_ = vp->second;
    }
    XLOG(INFO) << "SetUp: existing VLAN=" << existingVlanId_
               << " port=" << existingVlanPort_;
  }

  // A VLAN ID used to test auto-create; cleaned up before each run
  static constexpr int kNewVlanId = 3999;
  static constexpr const char* kTestMac = "02:00:00:00:27:01";

  // Discovered in SetUp(). -1/"" means no L2 VLAN on this DUT.
  int existingVlanId_ = -1;
  std::string existingVlanPort_;

  void
  addStaticMac(int vlanId, const std::string& mac, const std::string& port) {
    auto result = runCli(
        {"config",
         "vlan",
         std::to_string(vlanId),
         "static-mac",
         "add",
         mac,
         port});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to add static MAC: " << result.stderr;
    commitConfig();
  }

  void deleteStaticMac(int vlanId, const std::string& mac) {
    auto result = runCli(
        {"config",
         "vlan",
         std::to_string(vlanId),
         "static-mac",
         "delete",
         mac});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to delete static MAC: " << result.stderr;
    commitConfig();
  }

  /**
   * Remove vlanId from the running config (if present) so the auto-create
   * test is idempotent across multiple runs.
   *
   * Strategy:
   *  1. Run a benign config command on existingVlanId_ to initialize the
   *     session file (~/.fboss2/agent.conf) from the current running config.
   *  2. Read the JSON session file and filter out any vlans, interfaces, and
   *     staticMacAddrs entries that reference vlanId.
   *  3. If vlanId was found, write the modified JSON back and commit it.
   *     Otherwise, clear the session (no commit needed).
   */
  void cleanupVlanFromConfig(int vlanId) {
    XLOG(INFO) << "[Cleanup] Removing VLAN " << vlanId
               << " from config (if present)...";

    // Initialize session from running config via a benign idempotent delete
    runCli(
        {"config",
         "vlan",
         std::to_string(existingVlanId_),
         "static-mac",
         "delete",
         "00:00:00:00:00:00"});

    // Locate the session file
    // NOLINTNEXTLINE(concurrency-mt-unsafe): Used only in test setup
    const char* home = std::getenv("HOME");
    ASSERT_NE(home, nullptr) << "HOME env var not set";
    fs::path sessionConfig = fs::path(home) / ".fboss2" / "agent.conf";
    ASSERT_TRUE(fs::exists(sessionConfig))
        << "Session config not found after init: " << sessionConfig;

    // Read and parse the session JSON
    std::string content;
    {
      std::ifstream ifs(sessionConfig);
      ASSERT_TRUE(ifs.is_open()) << "Cannot read session config";
      content.assign(
          std::istreambuf_iterator<char>(ifs),
          std::istreambuf_iterator<char>{});
    }

    auto cfg = folly::parseJson(content);
    auto& sw = cfg["sw"];

    // Helper: filter an array, removing entries where field == vlanId
    auto filterByField =
        [vlanId](const folly::dynamic& arr, const std::string& field) {
          folly::dynamic result = folly::dynamic::array;
          for (const auto& item : arr) {
            if (!item.count(field) || item[field].asInt() != vlanId) {
              result.push_back(item);
            }
          }
          return result;
        };

    // Check if vlanId exists in the vlans list
    bool vlanFound = false;
    if (sw.count("vlans")) {
      for (const auto& v : sw["vlans"]) {
        if (v.count("id") && v["id"].asInt() == vlanId) {
          vlanFound = true;
          break;
        }
      }
    }

    if (!vlanFound) {
      XLOG(INFO) << "[Cleanup]   VLAN " << vlanId
                 << " not in running config — no cleanup needed.";
      // Discard the session we just created (nothing to commit)
      runCli({"config", "session", "clear"});
      return;
    }

    // Remove VLAN, its interface, and any static MACs for this vlanId
    if (sw.count("vlans")) {
      sw["vlans"] = filterByField(sw["vlans"], "id");
    }
    if (sw.count("interfaces")) {
      sw["interfaces"] = filterByField(sw["interfaces"], "vlanID");
    }
    if (sw.count("staticMacAddrs")) {
      sw["staticMacAddrs"] = filterByField(sw["staticMacAddrs"], "vlanID");
    }

    // Write modified JSON back to the session file
    {
      std::ofstream ofs(sessionConfig);
      ASSERT_TRUE(ofs.is_open()) << "Cannot write session config";
      ofs << folly::toJson(cfg);
    }

    // Commit the cleaned-up config to the agent
    commitConfig();
    XLOG(INFO) << "[Cleanup]   VLAN " << vlanId << " removed and committed.";
  }
};

TEST_F(ConfigVlanCreateTest, StaticMacAddDeleteOnExistingVlan) {
  if (existingVlanId_ < 0) {
    GTEST_SKIP()
        << "No L2 VLAN with port members on this DUT (pure routed mode)";
  }
  XLOG(INFO) << "[Step 1] Using VLAN=" << existingVlanId_
             << " port=" << existingVlanPort_;

  // Step 2: Add a static MAC to an existing VLAN - no creation message expected
  XLOG(INFO) << "[Step 2] Adding static MAC to existing VLAN "
             << existingVlanId_ << "...";
  auto addResult = runCli(
      {"config",
       "vlan",
       std::to_string(existingVlanId_),
       "static-mac",
       "add",
       kTestMac,
       existingVlanPort_});
  ASSERT_EQ(addResult.exitCode, 0)
      << "Failed to add static MAC: " << addResult.stderr;
  EXPECT_THAT(
      addResult.stdout, ::testing::Not(::testing::HasSubstr("Created VLAN")))
      << "Should not print VLAN creation message for existing VLAN";
  EXPECT_THAT(addResult.stdout, ::testing::HasSubstr("Successfully added"))
      << "Expected success message";
  XLOG(INFO) << "  Output: " << addResult.stdout;

  commitConfig();
  XLOG(INFO) << "  Config committed.";

  // Step 3: Clean up - delete the static MAC entry
  XLOG(INFO) << "[Step 3] Cleaning up static MAC entry...";
  deleteStaticMac(existingVlanId_, kTestMac);
  XLOG(INFO) << "  Cleanup complete. TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, AutoCreateVlanInSession) {
  if (existingVlanId_ < 0) {
    GTEST_SKIP()
        << "No L2 VLAN with port members on this DUT (pure routed mode)";
  }
  XLOG(INFO) << "[Step 1] Using port=" << existingVlanPort_;

  // Step 0: Ensure VLAN kNewVlanId is absent (idempotency across test runs).
  XLOG(INFO) << "[Step 0] Ensuring VLAN " << kNewVlanId
             << " is absent from running config...";
  cleanupVlanFromConfig(kNewVlanId);

  // Step 2: Add a static MAC to non-existent VLAN kNewVlanId.
  // VlanManager auto-creates both the VLAN entry and a barebone cfg::Interface.
  XLOG(INFO) << "[Step 2] Adding static MAC on non-existent VLAN " << kNewVlanId
             << "...";
  auto result = runCli(
      {"config",
       "vlan",
       std::to_string(kNewVlanId),
       "static-mac",
       "add",
       kTestMac,
       existingVlanPort_});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(result.stdout, ::testing::HasSubstr("Created VLAN"))
      << "Expected VLAN creation message for new VLAN";
  XLOG(INFO) << "  Output: " << result.stdout;

  // Step 3: Run a second command on the same VLAN within the same session.
  // The VLAN already exists in the session config, so no creation message.
  XLOG(INFO)
      << "[Step 3] Second command on same session VLAN (no creation expected)...";
  auto result2 = runCli(
      {"config",
       "vlan",
       std::to_string(kNewVlanId),
       "static-mac",
       "add",
       kTestMac,
       existingVlanPort_});
  ASSERT_EQ(result2.exitCode, 0) << "Command failed: " << result2.stderr;
  EXPECT_THAT(
      result2.stdout, ::testing::Not(::testing::HasSubstr("Created VLAN")))
      << "Should NOT print creation message on second command (VLAN exists in session)";
  XLOG(INFO) << "  Output: " << result2.stdout;

  // Step 4: Commit — VlanManager created both the VLAN entry and a barebone
  // cfg::Interface, so the agent accepts the commit.
  XLOG(INFO) << "[Step 4] Committing session...";
  commitConfig();
  XLOG(INFO) << "  Config committed.";

  // Step 5: Clean up the static MAC entry (VLAN + interface remain; they
  // will be removed by cleanupVlanFromConfig() at the start of the next run).
  XLOG(INFO) << "[Step 5] Cleaning up static MAC entry...";
  deleteStaticMac(kNewVlanId, kTestMac);
  XLOG(INFO) << "  Cleanup complete. TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, DeleteOnNonExistentVlanIsIdempotent) {
  XLOG(INFO) << "Testing idempotent delete on non-existent VLAN...";
  auto result = runCli(
      {"config",
       "vlan",
       std::to_string(kNewVlanId),
       "static-mac",
       "delete",
       kTestMac});
  ASSERT_EQ(result.exitCode, 0)
      << "Delete on non-existent VLAN should succeed: " << result.stderr;
  EXPECT_THAT(result.stdout, ::testing::HasSubstr("does not exist"));
  EXPECT_THAT(
      result.stdout, ::testing::Not(::testing::HasSubstr("Created VLAN")));
  XLOG(INFO) << "  Output: " << result.stdout << " TEST PASSED";
}
