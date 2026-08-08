// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for VLAN static MAC configuration and VlanManager behavior.
 *
 * Tests:
 *
 * 1. StaticMacAddDeleteOnExistingVlan
 *    - Add a static MAC entry to an existing VLAN (2001)
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
 * 4. CreateVlanCommand
 *    - "config vlan <id>" with no subcommand creates the VLAN in the session
 *    - Re-running the command in the same session reports "already exists"
 *    - Commit succeeds (VlanManager also created the barebone interface)
 *
 * 5. CreateVlanAlreadyExists
 *    - "config vlan <id>" on a VLAN present in the running config reports
 *      "already exists" and does not create anything
 *
 * 6. SwitchportAccessVlanAutoCreates
 *    - "config interface <port> switchport access vlan <id>" on a missing
 *      VLAN auto-creates it and appends "(VLAN <id> created)" to the output
 *    - Second run in the same session does not print the created suffix
 *    - Session is cleared without committing (the DUT is left untouched)
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 * - VLAN 2001 must exist in the config (standard fboss103 setup)
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
 public:
  // Find and cache an eth interface name once before any test in this suite
  // runs. This avoids calling show interface during the agent reload window
  // that follows test 1's commit.
  static void SetUpTestSuite() {
    // Concrete subclass to access protected Fboss2IntegrationTest methods.
    struct Helper : public Fboss2IntegrationTest {
      void TestBody() override {}
      std::string getFirstEthInterface() {
        return getRandomInterfacePortName();
      }
    };
    Helper h;
    s_testInterfaceName_ = h.getFirstEthInterface();
    XLOG(INFO) << "SetUpTestSuite: cached test interface = "
               << s_testInterfaceName_;
  }

 protected:
  // An existing VLAN with a proper L3 interface — safe to add static MACs to
  static constexpr int kExistingVlanId = 2001;
  // A VLAN ID used to test auto-create; cleaned up before each run
  static constexpr int kNewVlanId = 3999;
  // VLAN ID for the "config vlan <id>" create test (committed, then removed
  // at the start of the next run by cleanupVlanFromConfig()).
  static constexpr int kCreateVlanId = 3998;
  // VLAN ID for the switchport auto-create test (session only, never
  // committed).
  static constexpr int kAccessVlanId = 3997;
  static constexpr const char* kTestMac = "02:00:00:00:27:01";

  // Interface name cached by SetUpTestSuite — valid for the lifetime of
  // the test suite, even if the agent is reloading between individual tests.
  static std::string s_testInterfaceName_;

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
   *  1. Run a benign config command on kExistingVlanId to initialize the
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
         std::to_string(kExistingVlanId),
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

std::string ConfigVlanCreateTest::s_testInterfaceName_;

TEST_F(ConfigVlanCreateTest, StaticMacAddDeleteOnExistingVlan) {
  XLOG(INFO) << "[Step 1] Using pre-cached interface: " << s_testInterfaceName_
             << " (VLAN: " << kExistingVlanId << ")";

  // Step 2: Add a static MAC to an existing VLAN - no creation message expected
  XLOG(INFO) << "[Step 2] Adding static MAC to existing VLAN "
             << kExistingVlanId << "...";
  auto addResult = runCli(
      {"config",
       "vlan",
       std::to_string(kExistingVlanId),
       "static-mac",
       "add",
       kTestMac,
       s_testInterfaceName_});
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
  deleteStaticMac(kExistingVlanId, kTestMac);
  XLOG(INFO) << "  Cleanup complete. TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, AutoCreateVlanInSession) {
  XLOG(INFO) << "[Step 1] Using pre-cached interface: " << s_testInterfaceName_;

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
       s_testInterfaceName_});
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
       s_testInterfaceName_});
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

TEST_F(ConfigVlanCreateTest, CreateVlanCommand) {
  if (existingVlanId_ < 0) {
    GTEST_SKIP()
        << "No L2 VLAN with port members on this DUT (pure routed mode)";
  }

  // Step 0: Ensure the test VLAN is absent (idempotency across runs).
  XLOG(INFO) << "[Step 0] Ensuring VLAN " << kCreateVlanId
             << " is absent from running config...";
  cleanupVlanFromConfig(kCreateVlanId);

  // Step 1: "config vlan <id>" with no subcommand creates the VLAN.
  XLOG(INFO) << "[Step 1] Creating VLAN " << kCreateVlanId
             << " via 'config vlan'...";
  auto result = runCli({"config", "vlan", std::to_string(kCreateVlanId)});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "Successfully created VLAN " + std::to_string(kCreateVlanId)));
  XLOG(INFO) << "  Output: " << result.stdout;

  // Step 2: Re-running in the same session reports "already exists".
  XLOG(INFO) << "[Step 2] Re-running create (already exists expected)...";
  auto result2 = runCli({"config", "vlan", std::to_string(kCreateVlanId)});
  ASSERT_EQ(result2.exitCode, 0) << "Command failed: " << result2.stderr;
  EXPECT_THAT(
      result2.stdout,
      ::testing::HasSubstr(
          "VLAN " + std::to_string(kCreateVlanId) + " already exists"));
  EXPECT_THAT(
      result2.stdout,
      ::testing::Not(::testing::HasSubstr("Successfully created")));
  XLOG(INFO) << "  Output: " << result2.stdout;

  // Step 3: Commit — VlanManager created both the VLAN and its barebone
  // interface, so the agent accepts the commit.
  XLOG(INFO) << "[Step 3] Committing session...";
  commitConfig();
  XLOG(INFO) << "  Config committed.";

  // Step 4: A new session on the committed config also reports the VLAN as
  // existing (the create actually persisted).
  XLOG(INFO) << "[Step 4] Verifying VLAN persisted after commit...";
  auto result3 = runCli({"config", "vlan", std::to_string(kCreateVlanId)});
  ASSERT_EQ(result3.exitCode, 0) << "Command failed: " << result3.stderr;
  EXPECT_THAT(result3.stdout, ::testing::HasSubstr("already exists"));
  runCli({"config", "session", "clear"});
  XLOG(INFO) << "  TEST PASSED (VLAN " << kCreateVlanId
             << " remains; removed by cleanup on next run)";
}

TEST_F(ConfigVlanCreateTest, CreateVlanAlreadyExists) {
  if (existingVlanId_ < 0) {
    GTEST_SKIP()
        << "No L2 VLAN with port members on this DUT (pure routed mode)";
  }
  XLOG(INFO) << "Testing 'config vlan' on existing VLAN " << existingVlanId_
             << "...";
  auto result = runCli({"config", "vlan", std::to_string(existingVlanId_)});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "VLAN " + std::to_string(existingVlanId_) + " already exists"));
  EXPECT_THAT(
      result.stdout,
      ::testing::Not(::testing::HasSubstr("Successfully created")));
  // Nothing was modified; discard the session.
  runCli({"config", "session", "clear"});
  XLOG(INFO) << "  Output: " << result.stdout << " TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, SwitchportAccessVlanAutoCreates) {
  if (existingVlanId_ < 0) {
    GTEST_SKIP()
        << "No L2 VLAN with port members on this DUT (pure routed mode)";
  }
  XLOG(INFO) << "[Step 1] Using port=" << existingVlanPort_;

  // Step 0: Start from a clean session so kAccessVlanId isn't left over from
  // a previous (aborted) run. This test never commits, so the running config
  // is untouched.
  runCli({"config", "session", "clear"});

  // Step 2: Assign the port to a non-existent VLAN — it gets auto-created.
  XLOG(INFO) << "[Step 2] switchport access vlan " << kAccessVlanId
             << " (auto-create expected)...";
  auto result = runCli(
      {"config",
       "interface",
       existingVlanPort_,
       "switchport",
       "access",
       "vlan",
       std::to_string(kAccessVlanId)});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(
      result.stdout, ::testing::HasSubstr("Successfully set access VLAN"));
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "(VLAN " + std::to_string(kAccessVlanId) + " created)"));
  XLOG(INFO) << "  Output: " << result.stdout;

  // Step 3: Second run in the same session — VLAN now exists, no suffix.
  XLOG(INFO) << "[Step 3] Second run (no creation suffix expected)...";
  auto result2 = runCli(
      {"config",
       "interface",
       existingVlanPort_,
       "switchport",
       "access",
       "vlan",
       std::to_string(kAccessVlanId)});
  ASSERT_EQ(result2.exitCode, 0) << "Command failed: " << result2.stderr;
  EXPECT_THAT(
      result2.stdout, ::testing::HasSubstr("Successfully set access VLAN"));
  EXPECT_THAT(result2.stdout, ::testing::Not(::testing::HasSubstr("created")));
  XLOG(INFO) << "  Output: " << result2.stdout;

  // Step 4: Discard the session — never committed, DUT is unchanged.
  XLOG(INFO) << "[Step 4] Clearing session (no commit)...";
  auto clearResult = runCli({"config", "session", "clear"});
  ASSERT_EQ(clearResult.exitCode, 0)
      << "Failed to clear session: " << clearResult.stderr;
  XLOG(INFO) << "  TEST PASSED";
}
