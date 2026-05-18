/**
 * End-to-end tests for 'fboss2-dev config vlan default <vlan-id>'.
 *
 * Covers the happy-path workflow (move ports, set default, commit, verify via
 * thrift), the no-op case when the target equals the current default, a safety
 * guard that refuses the command when the precondition is unsafe, and the case
 * where ports have already been moved off the current default VLAN.
 */

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace facebook::fboss;

class ConfigVlanDefaultTest : public Fboss2IntegrationTest {
 protected:
  folly::dynamic getRunningConfig() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string configStr;
    client->sync_getRunningConfig(configStr);
    return folly::parseJson(configStr);
  }
};

/**
 * Moves all eth ports to a target VLAN, sets it as the new default, commits
 * once, and verifies via thrift. Restores original VLAN membership and
 * defaultVlan at the end.
 */
TEST_F(ConfigVlanDefaultTest, SetDefaultVlanTo300) {
  waitForAgentReady();

  // Pick a target that differs from the current default so the commit is
  // never a no-op.
  auto initialConfig = getRunningConfig();
  int32_t currentDefault =
      initialConfig["sw"].getDefault("defaultVlan", 1).asInt();
  const std::string targetVlan = (currentDefault == 300) ? "301" : "300";
  XLOG(INFO) << "[Test] currentDefault=" << currentDefault
             << " targetVlan=" << targetVlan;

  // Step 0: Create the target VLAN if it doesn't already exist.
  // VlanManager auto-creates the VLAN entry and a barebone cfg::Interface so
  // the agent accepts the commit. The dummy MAC is removed in restore below.
  const std::string kDummyMac = "02:00:00:00:01:2c";
  bool targetVlanCreated = false;
  {
    bool exists = false;
    for (const auto& vlan : initialConfig["sw"]["vlans"]) {
      if (vlan.getDefault("id", -1).asInt() == std::stoi(targetVlan)) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      auto firstEthPort = findFirstEthInterface().name;
      ASSERT_FALSE(firstEthPort.empty())
          << "No eth port found for VLAN creation";
      auto createResult = runCli(
          {"config",
           "vlan",
           targetVlan,
           "static-mac",
           "add",
           kDummyMac,
           firstEthPort});
      ASSERT_EQ(createResult.exitCode, 0)
          << "Failed to create VLAN " << targetVlan << ": "
          << createResult.stderr;
      XLOG(INFO) << "[Step 0] " << createResult.stdout;
      targetVlanCreated = true;
    }
  }

  // Step 1: Move all eth ports whose ingressVlan matches the current default
  // VLAN
  for (const auto& port : initialConfig["sw"]["ports"]) {
    auto name = port.getDefault("name", "").asString();
    if (name.rfind("eth", 0) != 0) {
      continue;
    }
    if (port.getDefault("ingressVlan", currentDefault).asInt() !=
        currentDefault) {
      continue;
    }
    auto result = runCli(
        {"config",
         "interface",
         name,
         "switchport",
         "access",
         "vlan",
         targetVlan});
    if (result.exitCode != 0) {
      XLOG(WARN) << "Skipping " << name << ": " << result.stderr;
      continue;
    }
  }
  // Step 2: Set default VLAN
  auto setResult = runCli({"config", "vlan", "default", targetVlan});
  ASSERT_EQ(setResult.exitCode, 0) << setResult.stderr;
  EXPECT_THAT(
      setResult.stdout,
      ::testing::HasSubstr("Successfully set default VLAN to " + targetVlan));
  XLOG(INFO) << "[Step 2] " << setResult.stdout;

  // Step 3: Single commit — one restart, then wait only for the thrift server
  // to load the config (not full ASIC init, which takes much longer).
  commitConfig();
  waitForAgentReady();
  // Step 4: Verify via running config.
  // sync_getRunningConfig reflects the committed config as soon as the agent
  // loads it, independently of ASIC programming progress.
  auto config = getRunningConfig();
  EXPECT_EQ(config["sw"]["defaultVlan"].asInt(), std::stoi(targetVlan));
  XLOG(INFO) << "[Step 4] Verified defaultVlan="
             << config["sw"]["defaultVlan"].asInt();

  // Restore: move every eth port that originally had ingressVlan ==
  // currentDefault back to its original VLAN, reset defaultVlan, and commit so
  // the next test starts from the original state.
  XLOG(INFO) << "[Restore] Reverting ports and defaultVlan to "
             << currentDefault;
  if (targetVlanCreated) {
    runCli({"config", "vlan", targetVlan, "static-mac", "delete", kDummyMac});
  }
  for (const auto& port : initialConfig["sw"]["ports"]) {
    auto name = port.getDefault("name", "").asString();
    if (name.rfind("eth", 0) != 0) {
      continue;
    }
    auto origVlan = port.getDefault("ingressVlan", currentDefault).asInt();
    if (origVlan != currentDefault) {
      continue;
    }
    auto r = runCli(
        {"config",
         "interface",
         name,
         "switchport",
         "access",
         "vlan",
         std::to_string(origVlan)});
    if (r.exitCode != 0) {
      XLOG(WARN) << "[Restore] Failed to move " << name << " back to VLAN "
                 << origVlan << ": " << r.stderr;
    }
  }

  auto restoreDefault =
      runCli({"config", "vlan", "default", std::to_string(currentDefault)});
  ASSERT_EQ(restoreDefault.exitCode, 0)
      << "[Restore] Failed to reset default VLAN: " << restoreDefault.stderr;

  commitConfig();
  waitForAgentReady();
  discardSession();
}

/**
 * Idempotency test: setting the default VLAN to its current value should be a
 * no-op and return a clear message, without changing the running config.
 */
TEST_F(ConfigVlanDefaultTest, NoOpWhenDefaultVlanUnchanged) {
  waitForAgentReady();

  auto initialConfig = getRunningConfig();
  int32_t currentDefault =
      initialConfig["sw"].getDefault("defaultVlan", 1).asInt();

  auto result =
      runCli({"config", "vlan", "default", std::to_string(currentDefault)});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  EXPECT_THAT(
      result.stdout,
      ::testing::HasSubstr(
          "Default VLAN is already set to " + std::to_string(currentDefault)));

  // No commit needed (command is a no-op), but ensure we don't leave a stale
  // session lying around for subsequent tests.
  discardSession();
}

/**
 * Validation guard: if the current default VLAN has at least one port whose
 * ingressVlan equals it but has no matching interface, the command must refuse
 * with a descriptive message and leave the config unchanged.
 *
 * This is the path guarded by `oldVlanUsedAsIngress && !oldVlanHasInterface`
 * in CmdConfigVlanDefault::queryClient.
 *
 * To keep the test simple, we do not try to synthesize this "dangerous" state.
 * Instead, we only run the negative test when the running config already has
 * a default VLAN that is used as ingressVlan by at least one port and has no
 * interface. If those conditions are not met, the test is skipped.
 */
TEST_F(ConfigVlanDefaultTest, RefuseWhenPortOnDefaultVlanWithNoInterface) {
  waitForAgentReady();

  auto initialConfig = getRunningConfig();
  int32_t currentDefault =
      initialConfig["sw"].getDefault("defaultVlan", 1).asInt();

  // Check if any port uses the current default VLAN as its ingressVlan.
  bool portOnDefault = false;
  for (const auto& port : initialConfig["sw"]["ports"]) {
    if (port.getDefault("ingressVlan", currentDefault).asInt() ==
        currentDefault) {
      portOnDefault = true;
      break;
    }
  }
  if (!portOnDefault) {
    GTEST_SKIP() << "No port uses the default VLAN as ingressVlan — cannot "
                    "exercise guard";
  }

  // Check that there is *no* interface mapped to the current default VLAN.
  bool interfaceOnDefault = false;
  if (initialConfig["sw"].count("interfaces")) {
    for (const auto& intf : initialConfig["sw"]["interfaces"]) {
      if (intf.getDefault("vlanID", -1).asInt() == currentDefault) {
        interfaceOnDefault = true;
        break;
      }
    }
  }
  if (interfaceOnDefault) {
    GTEST_SKIP() << "Default VLAN has an interface — dangerous state not "
                    "present";
  }

  // At this point the running config matches the guard's precondition:
  //  - At least one port has ingressVlan == currentDefault
  //  - No interface has vlanID == currentDefault
  // Any attempt to change the default VLAN must be refused.
  const std::string nextVlan = (currentDefault == 300) ? "301" : "300";
  auto result = runCli({"config", "vlan", "default", nextVlan});
  EXPECT_THAT(
      result.stdout, ::testing::HasSubstr("Refusing to change default VLAN"));
  XLOG(INFO) << "[NegativeGuard] stdout='" << result.stdout << "'";

  // Nothing was committed — discard is sufficient to restore state.
  discardSession();
}

/**
 * Verifies that changing the default VLAN succeeds when at least one port has
 * an ingressVlan that differs from the current defaultVlan.
 *
 * Uses an existing VLAN (one with an interface already mapped) so the VLAN
 * table entry is guaranteed to exist when we set it as the new default.
 */
TEST_F(ConfigVlanDefaultTest, ChangeDefaultVlanWithPortInNonDefaultVlan) {
  waitForAgentReady();

  auto initialConfig = getRunningConfig();
  int32_t currentDefault =
      initialConfig["sw"].getDefault("defaultVlan", 1).asInt();

  // Find an existing non-default VLAN to use as the port reassignment target.
  int32_t sideVlan = -1;
  for (const auto& vlan : initialConfig["sw"]["vlans"]) {
    int32_t vid = vlan.getDefault("id", -1).asInt();
    if (vid > 0 && vid != currentDefault) {
      sideVlan = vid;
      break;
    }
  }
  if (sideVlan == -1) {
    GTEST_SKIP() << "No VLAN other than the default found in the VLAN table";
  }
  XLOG(INFO) << "currentDefault=" << currentDefault << " sideVlan=" << sideVlan;

  // Reassign ports off the current default VLAN to exercise the case where
  // ingressVlan differs from defaultVlan.
  int portsMoved = 0;
  for (const auto& port : initialConfig["sw"]["ports"]) {
    auto name = port.getDefault("name", "").asString();
    if (name.rfind("eth", 0) != 0) {
      continue;
    }
    if (port.getDefault("ingressVlan", currentDefault).asInt() !=
        currentDefault) {
      continue;
    }
    auto r = runCli(
        {"config",
         "interface",
         name,
         "switchport",
         "access",
         "vlan",
         std::to_string(sideVlan)});
    if (r.exitCode == 0) {
      ++portsMoved;
    }
  }
  if (portsMoved == 0) {
    GTEST_SKIP() << "No eth ports on the default VLAN to move";
  }
  XLOG(INFO) << "[Step 1] Moved " << portsMoved << " ports from VLAN "
             << currentDefault << " to " << sideVlan;

  // Set a new default VLAN — the command must succeed even when no ports
  // remain on the old default VLAN.
  const int32_t newDefault = 4093;
  auto result =
      runCli({"config", "vlan", "default", std::to_string(newDefault)});
  ASSERT_EQ(result.exitCode, 0)
      << "failed to set default VLAN: " << result.stderr;
  XLOG(INFO) << "[Step 2] " << result.stdout;

  // Step 3: Commit and verify.
  commitConfig();
  waitForAgentReady();
  auto postCommitConfig = getRunningConfig();
  EXPECT_EQ(postCommitConfig["sw"]["defaultVlan"].asInt(), newDefault);

  // Restore: move every eth port currently on sideVlan back to currentDefault
  // and reset defaultVlan, then commit so the next test starts from the
  // original state.
  XLOG(INFO) << "[Restore] Moving ports from VLAN " << sideVlan << " back to "
             << currentDefault;
  for (const auto& port : postCommitConfig["sw"]["ports"]) {
    auto name = port.getDefault("name", "").asString();
    if (name.rfind("eth", 0) != 0) {
      continue;
    }
    if (port.getDefault("ingressVlan", sideVlan).asInt() != sideVlan) {
      continue;
    }
    auto r = runCli(
        {"config",
         "interface",
         name,
         "switchport",
         "access",
         "vlan",
         std::to_string(currentDefault)});
    if (r.exitCode != 0) {
      XLOG(WARN) << "[Restore] Failed to move " << name << " back to VLAN "
                 << currentDefault << ": " << r.stderr;
    }
  }

  auto restoreDefault =
      runCli({"config", "vlan", "default", std::to_string(currentDefault)});
  ASSERT_EQ(restoreDefault.exitCode, 0)
      << "[Restore] Failed to reset default VLAN: " << restoreDefault.stderr;

  commitConfig();
  waitForAgentReady();
  discardSession();
}
