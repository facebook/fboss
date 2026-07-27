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
 *
 * These tests do not assume the DUT is configured any particular way: the
 * fixture picks an ethernet port and provisions the VLANs it needs (see
 * SetUp / ensureCommittedVlan / TearDown), so they run on both L2- and pure
 * routed-mode DUTs instead of skipping.
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
  // These tests must not assume the DUT is configured any particular way, so
  // the fixture provisions everything they need:
  //   - existingVlanPort_: any ethernet port. static-mac / switchport resolve
  //     a port by logical ID, so L2-VLAN membership is irrelevant to them —
  //     any eth port works, on L2- and routed-mode DUTs alike.
  //   - kExistingVlanId (kExistingVlanId): a VLAN created + committed on demand
  //     via ensureCommittedVlan(), for the cases that need a VLAN that is
  //     already present in the running config.
  // TearDown() removes every VLAN the fixture may have created so runs stay
  // idempotent and the DUT is left as we found it.
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    existingVlanPort_ = getRandomInterfacePortName();
    XLOG(INFO) << "SetUp: port=" << existingVlanPort_
               << " (existing-VLAN id=" << kExistingVlanId << ")";
  }

  void TearDown() override {
    // Remove any VLAN this fixture created + committed. Session-only VLANs
    // (kAccessVlanId) never reach the running config, so they need no cleanup.
    for (int vlanId : {kExistingVlanId, kNewVlanId, kCreateVlanId}) {
      cleanupVlanFromConfig(vlanId);
    }
    Fboss2IntegrationTest::TearDown();
  }

  // VLAN the fixture creates + commits for the "already exists" cases.
  static constexpr int kExistingVlanId = 3996;
  // A VLAN ID used to test auto-create; cleaned up before each run
  static constexpr int kNewVlanId = 3999;
  // VLAN ID for the "config vlan <id>" create test (committed, then removed
  // at the start of the next run by cleanupVlanFromConfig()).
  static constexpr int kCreateVlanId = 3998;
  // VLAN ID for the switchport auto-create test (session only, never
  // committed).
  static constexpr int kAccessVlanId = 3997;
  static constexpr const char* kTestMac = "02:00:00:00:27:01";

  // Set in SetUp().
  std::string existingVlanPort_;

  // Create `vlanId` in the running config (idempotent) so a fresh session sees
  // it as already existing. Creating a barebone VLAN + its cfg::Interface is
  // accepted by the agent regardless of L2/L3 mode (same path exercised by
  // AutoCreateVlanInSession and Fboss2IntegrationTest::ensureUnderlayIntfId).
  void ensureCommittedVlan(int vlanId) {
    auto hasVlan = [vlanId](const folly::dynamic& c) {
      if (!c.isObject() || !c.count("sw") || !c["sw"].count("vlans")) {
        return false;
      }
      for (const auto& v : c["sw"]["vlans"]) {
        if (v.count("id") && v["id"].asInt() == vlanId) {
          return true;
        }
      }
      return false;
    };

    // Nothing to commit if the VLAN is already present (e.g. leftover from a
    // prior run that didn't reach TearDown) — a no-op commit would error.
    // discardSession() (not the "config session clear" CLI command) both
    // clears the on-disk session files and resets the in-process
    // ConfigSession singleton — see the comment in cleanupVlanFromConfig()
    // for why the CLI command alone is not enough.
    if (hasVlan(getRunningConfig())) {
      discardSession();
      return;
    }

    discardSession();
    auto result = runCli({"config", "vlan", std::to_string(vlanId)});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to create VLAN " << vlanId << ": " << result.stderr;
    commitConfig();

    // A commit that creates a VLAN can warmboot the agent; wait until the VLAN
    // is visible in the running config before the test relies on it.
    auto cfg = waitForRunningConfig(hasVlan);
    ASSERT_TRUE(hasVlan(cfg))
        << "VLAN " << vlanId << " not in running config after commit";

    // Start the test proper from a clean session built on the committed config.
    discardSession();
  }

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

    // Initialize the session file from the running config via a benign
    // idempotent delete (a delete on a missing VLAN is a no-op, and the
    // session is materialized on first access regardless).
    runCli(
        {"config",
         "vlan",
         std::to_string(vlanId),
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
      // Discard the session we just created (nothing to commit). Use
      // discardSession() rather than the "config session clear" CLI command:
      // that command only deletes the on-disk session files, it does not
      // reset the in-process ConfigSession singleton. Since this fixture
      // calls cleanupVlanFromConfig() repeatedly in TearDown(), a stale
      // singleton would make the *next* call's getAgentConfig() return the
      // cached in-memory config without touching disk at all — so a
      // subsequent no-op delete (nothing found, no saveConfig()) would never
      // recreate the session file, and this method's own ASSERT_TRUE above
      // would fail on the following call. discardSession() also resets the
      // singleton, avoiding that trap.
      discardSession();
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
  // Provision the "existing" VLAN so the add below operates on a VLAN that is
  // already present (no "Created VLAN" message expected).
  ensureCommittedVlan(kExistingVlanId);
  XLOG(INFO) << "[Step 1] Using VLAN=" << kExistingVlanId
             << " port=" << existingVlanPort_;

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
  deleteStaticMac(kExistingVlanId, kTestMac);
  XLOG(INFO) << "  Cleanup complete. TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, AutoCreateVlanInSession) {
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

TEST_F(ConfigVlanCreateTest, CreateVlanCommand) {
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
  discardSession();
  XLOG(INFO) << "  TEST PASSED (VLAN " << kCreateVlanId
             << " remains; removed by cleanup on next run)";
}

TEST_F(ConfigVlanCreateTest, CreateVlanAlreadyExists) {
  // Provision a VLAN in the running config, then verify 'config vlan' reports
  // it as already existing.
  ensureCommittedVlan(kExistingVlanId);
  XLOG(INFO) << "Testing 'config vlan' on existing VLAN " << kExistingVlanId
             << "...";
  auto result = runCli({"config", "vlan", std::to_string(kExistingVlanId)});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "VLAN " + std::to_string(kExistingVlanId) + " already exists"));
  EXPECT_THAT(
      result.stdout,
      ::testing::Not(::testing::HasSubstr("Successfully created")));
  // Nothing was modified; discard the session.
  discardSession();
  XLOG(INFO) << "  Output: " << result.stdout << " TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, SwitchportAccessVlanAutoCreates) {
  XLOG(INFO) << "[Step 1] Using port=" << existingVlanPort_;

  // Step 0: Start from a clean session so kAccessVlanId isn't left over from
  // a previous (aborted) run. This test never commits, so the running config
  // is untouched.
  discardSession();

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
  discardSession();
  XLOG(INFO) << "  TEST PASSED";
}
