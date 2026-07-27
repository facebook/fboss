// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for VLAN static MAC configuration and VlanManager behavior.
 *
 * Tests:
 *
 * 1. StaticMacAddDeleteOnExistingVlan
 *    - Add a static MAC entry to an existing VLAN (provisioned on demand)
 *    - Commit and verify
 *    - Delete the entry and commit
 *    This is the primary use case for static-mac add/delete.
 *
 * 2. AutoCreateVlanInSession
 *    - Add a static MAC entry to a VLAN that does not exist
 *    - VlanManager auto-creates the VLAN and a barebone cfg::Interface
 *    - Verify "Created VLAN" message appears on first command
 *    - Verify second command does NOT print the creation message again
 *    - Commit the session (both VLAN and interface are present, so the agent
 *      accepts the commit)
 *    - Clean up: delete the static MAC entry, then the VLAN and its interface
 *
 *    The VLAN ID comes from pickUnusedVlanId() rather than a constant: the
 *    auto-created interface consumes a Linux route table, and an ID outside
 *    the supported window aborts the agent (T284228086).
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
 * SetUpTestSuite / ensureCommittedVlan / TearDown), so they run on both L2-
 * and pure routed-mode DUTs instead of skipping.
 */

#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

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
      int getUnusedVlanId(const std::set<int>& exclude = {}) {
        return pickUnusedVlanId(exclude);
      }
    };
    Helper h;
    s_testInterfaceName_ = h.getFirstEthInterface();
    s_newVlanId_ = h.getUnusedVlanId();
    s_existingVlanId_ = h.getUnusedVlanId({s_newVlanId_});
    XLOG(INFO) << "SetUpTestSuite: cached test interface = "
               << s_testInterfaceName_
               << ", auto-create VLAN = " << s_newVlanId_
               << ", existing VLAN = " << s_existingVlanId_;
  }

 protected:
  void TearDown() override {
    // The VLANs this suite creates are backed by a cfg::Interface, and the
    // agent gives that interface a TUN device that outlives the process.
    // Leaving one behind leaks kernel state into every later test and agent
    // restart.
    for (int vlanId : {s_newVlanId_, s_existingVlanId_}) {
      if (vlanId != 0) {
        deleteVlanIfPresent(vlanId);
      }
    }
    Fboss2IntegrationTest::TearDown();
  }

  static constexpr const char* kTestMac = "02:00:00:00:27:01";

  // Interface name cached by SetUpTestSuite — valid for the lifetime of
  // the test suite, even if the agent is reloading between individual tests.
  static std::string s_testInterfaceName_;
  // VLAN ID used to exercise auto-create, chosen from the running config by
  // SetUpTestSuite. 0 when the platform has none free.
  static int s_newVlanId_;
  // VLAN the fixture creates + commits (via ensureCommittedVlan) for the
  // cases that need a VLAN already present in the running config. 0 when the
  // platform has none free.
  static int s_existingVlanId_;

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
};

std::string ConfigVlanCreateTest::s_testInterfaceName_;
int ConfigVlanCreateTest::s_newVlanId_ = 0;
int ConfigVlanCreateTest::s_existingVlanId_ = 0;

TEST_F(ConfigVlanCreateTest, StaticMacAddDeleteOnExistingVlan) {
  if (s_existingVlanId_ == 0) {
    GTEST_SKIP() << "No VLAN ID free in [" << kTestVlanMin << ", "
                 << kTestVlanMax << "] on this switch";
  }
  // Provision the "existing" VLAN so the add below operates on a VLAN that is
  // already present (no "Created VLAN" message expected).
  ensureCommittedVlan(s_existingVlanId_);
  XLOG(INFO) << "[Step 1] Using VLAN=" << s_existingVlanId_
             << " port=" << s_testInterfaceName_;

  // Step 2: Add a static MAC to an existing VLAN - no creation message expected
  XLOG(INFO) << "[Step 2] Adding static MAC to existing VLAN "
             << s_existingVlanId_ << "...";
  auto addResult = runCli(
      {"config",
       "vlan",
       std::to_string(s_existingVlanId_),
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
  deleteStaticMac(s_existingVlanId_, kTestMac);
  XLOG(INFO) << "  Cleanup complete. TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, AutoCreateVlanInSession) {
  if (s_newVlanId_ == 0) {
    GTEST_SKIP() << "No VLAN ID free in [" << kTestVlanMin << ", "
                 << kTestVlanMax << "] on this switch";
  }
  XLOG(INFO) << "[Step 1] Using pre-cached interface: " << s_testInterfaceName_;

  // Step 0: Ensure the VLAN is absent (idempotency across test runs).
  XLOG(INFO) << "[Step 0] Ensuring VLAN " << s_newVlanId_
             << " is absent from running config...";
  deleteVlanIfPresent(s_newVlanId_);

  // Step 2: Add a static MAC to the non-existent VLAN. VlanManager
  // auto-creates both the VLAN entry and a barebone cfg::Interface.
  XLOG(INFO) << "[Step 2] Adding static MAC on non-existent VLAN "
             << s_newVlanId_ << "...";
  auto result = runCli(
      {"config",
       "vlan",
       std::to_string(s_newVlanId_),
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
       std::to_string(s_newVlanId_),
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

  // Step 5: Clean up the static MAC entry. TearDown() then removes the VLAN
  // and its backing interface so no TUN device is left in the kernel.
  XLOG(INFO) << "[Step 5] Cleaning up static MAC entry...";
  deleteStaticMac(s_newVlanId_, kTestMac);
  XLOG(INFO) << "  Cleanup complete. TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, DeleteOnNonExistentVlanIsIdempotent) {
  if (s_newVlanId_ == 0) {
    GTEST_SKIP() << "No VLAN ID free in [" << kTestVlanMin << ", "
                 << kTestVlanMax << "] on this switch";
  }
  XLOG(INFO) << "Testing idempotent delete on non-existent VLAN...";
  auto result = runCli(
      {"config",
       "vlan",
       std::to_string(s_newVlanId_),
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
  if (s_newVlanId_ == 0) {
    GTEST_SKIP() << "No VLAN ID free in [" << kTestVlanMin << ", "
                 << kTestVlanMax << "] on this switch";
  }

  // Step 0: Ensure the test VLAN is absent (idempotency across runs).
  XLOG(INFO) << "[Step 0] Ensuring VLAN " << s_newVlanId_
             << " is absent from running config...";
  deleteVlanIfPresent(s_newVlanId_);

  // Step 1: "config vlan <id>" with no subcommand creates the VLAN.
  XLOG(INFO) << "[Step 1] Creating VLAN " << s_newVlanId_
             << " via 'config vlan'...";
  auto result = runCli({"config", "vlan", std::to_string(s_newVlanId_)});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "Successfully created VLAN " + std::to_string(s_newVlanId_)));
  XLOG(INFO) << "  Output: " << result.stdout;

  // Step 2: Re-running in the same session reports "already exists".
  XLOG(INFO) << "[Step 2] Re-running create (already exists expected)...";
  auto result2 = runCli({"config", "vlan", std::to_string(s_newVlanId_)});
  ASSERT_EQ(result2.exitCode, 0) << "Command failed: " << result2.stderr;
  EXPECT_THAT(
      result2.stdout,
      ::testing::HasSubstr(
          "VLAN " + std::to_string(s_newVlanId_) + " already exists"));
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
  auto result3 = runCli({"config", "vlan", std::to_string(s_newVlanId_)});
  ASSERT_EQ(result3.exitCode, 0) << "Command failed: " << result3.stderr;
  EXPECT_THAT(result3.stdout, ::testing::HasSubstr("already exists"));
  discardSession();
  XLOG(INFO) << "  TEST PASSED (VLAN " << s_newVlanId_
             << " removed by TearDown)";
}

TEST_F(ConfigVlanCreateTest, CreateVlanAlreadyExists) {
  if (s_existingVlanId_ == 0) {
    GTEST_SKIP() << "No VLAN ID free in [" << kTestVlanMin << ", "
                 << kTestVlanMax << "] on this switch";
  }
  // Provision a VLAN in the running config, then verify 'config vlan' reports
  // it as already existing.
  ensureCommittedVlan(s_existingVlanId_);
  XLOG(INFO) << "Testing 'config vlan' on existing VLAN " << s_existingVlanId_
             << "...";
  auto result = runCli({"config", "vlan", std::to_string(s_existingVlanId_)});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "VLAN " + std::to_string(s_existingVlanId_) + " already exists"));
  EXPECT_THAT(
      result.stdout,
      ::testing::Not(::testing::HasSubstr("Successfully created")));
  // Nothing was modified; discard the session.
  discardSession();
  XLOG(INFO) << "  Output: " << result.stdout << " TEST PASSED";
}

TEST_F(ConfigVlanCreateTest, SwitchportAccessVlanAutoCreates) {
  if (s_newVlanId_ == 0) {
    GTEST_SKIP() << "No VLAN ID free in [" << kTestVlanMin << ", "
                 << kTestVlanMax << "] on this switch";
  }
  XLOG(INFO) << "[Step 1] Using port=" << s_testInterfaceName_;

  // Step 0: Start from a clean session so the VLAN isn't left over from a
  // previous (aborted) run. This test never commits, so the running config
  // is untouched.
  discardSession();

  // Step 2: Assign the port to a non-existent VLAN — it gets auto-created.
  XLOG(INFO) << "[Step 2] switchport access vlan " << s_newVlanId_
             << " (auto-create expected)...";
  auto result = runCli(
      {"config",
       "interface",
       s_testInterfaceName_,
       "switchport",
       "access",
       "vlan",
       std::to_string(s_newVlanId_)});
  ASSERT_EQ(result.exitCode, 0) << "Command failed: " << result.stderr;
  EXPECT_THAT(
      result.stdout, ::testing::HasSubstr("Successfully set access VLAN"));
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "(VLAN " + std::to_string(s_newVlanId_) + " created)"));
  XLOG(INFO) << "  Output: " << result.stdout;

  // Step 3: Second run in the same session — VLAN now exists, no suffix.
  XLOG(INFO) << "[Step 3] Second run (no creation suffix expected)...";
  auto result2 = runCli(
      {"config",
       "interface",
       s_testInterfaceName_,
       "switchport",
       "access",
       "vlan",
       std::to_string(s_newVlanId_)});
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
