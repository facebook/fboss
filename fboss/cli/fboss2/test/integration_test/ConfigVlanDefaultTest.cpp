/**
 * End-to-end tests for 'fboss2-dev config vlan default <vlan-id>'.
 *
 * Covers the happy-path workflow (move ports, set default, commit, verify via
 * thrift), the no-op case when the target equals the current default, and a
 * safety guard that refuses the command when the precondition is unsafe.
 *
 * Branch-level coverage of CmdConfigVlanDefault (target VLAN exists vs.
 * created, old VLAN removal, missing defaultVlan field) lives in the fast unit
 * test CmdConfigVlanDefaultTest; the e2e tests here only verify that a
 * committed change survives the agent warmboot, so a single happy-path e2e
 * suffices rather than one per branch.
 */

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigVlanDefaultTest : public Fboss2IntegrationTest {};

/**
 * Sets a new default VLAN, commits, verifies via thrift, and restores.
 *
 * CmdConfigVlanDefault::queryClient auto-creates the target VLAN if it
 * doesn't exist, so no manual VLAN creation is needed. Ports on the old
 * default VLAN must be moved off first (the command refuses to change when
 * ports use the old default but it has no interface).
 */
TEST_F(ConfigVlanDefaultTest, SetDefaultVlanTo300) {
  waitForAgentReady();

  auto initialConfig = getRunningConfig();
  int64_t currentDefault =
      initialConfig["sw"].getDefault("defaultVlan", 1).asInt();
  const std::string targetVlan = (currentDefault == 300) ? "301" : "300";
  XLOG(INFO) << "[Test] currentDefault=" << currentDefault
             << " targetVlan=" << targetVlan;

  // Step 0: Create the target VLAN if it doesn't already exist.
  // VlanManager auto-creates the VLAN entry and a barebone cfg::Interface so
  // the agent accepts the commit. The dummy MAC is deleted right away; only
  // the VLAN and its interface are kept.
  const std::string kDummyMac = "02:00:00:00:01:2c";
  {
    bool exists = false;
    if (initialConfig["sw"].count("vlans")) {
      for (const auto& vlan : initialConfig["sw"]["vlans"]) {
        if (vlan.getDefault("id", -1).asInt() == std::stoi(targetVlan)) {
          exists = true;
          break;
        }
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
      auto macCleanup = runCli(
          {"config", "vlan", targetVlan, "static-mac", "delete", kDummyMac});
      EXPECT_EQ(macCleanup.exitCode, 0)
          << "Failed to delete dummy MAC from VLAN " << targetVlan << ": "
          << macCleanup.stderr;
      XLOG(INFO) << "[Step 0] " << createResult.stdout;
    }
  }

  // Step 1: Move every eth port whose ingressVlan is the current default VLAN
  // onto targetVlan (created in Step 0). targetVlan is intentionally the move
  // target — it always exists by now, so no other side VLAN has to be found.
  // This is required because the command refuses to change the default while
  // ports still use the old default VLAN and it has no interface (safety
  // guard against an agent crash), so the old default is vacated first.
  std::vector<std::string> movedPorts;
  for (const auto& port : initialConfig["sw"]["ports"]) {
    auto name = port.getDefault("name", "").asString();
    if (name.rfind("eth", 0) != 0) {
      continue;
    }
    if (port.getDefault("ingressVlan", -1).asInt() != currentDefault) {
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
    if (result.exitCode == 0) {
      movedPorts.push_back(name);
    }
  }
  XLOG(INFO) << "[Step 1] Moved " << movedPorts.size()
             << " ports off default VLAN";

  // Step 2: Set default VLAN — the command auto-creates the VLAN if needed
  auto setResult = runCli({"config", "vlan", "default", targetVlan});
  ASSERT_EQ(setResult.exitCode, 0) << setResult.stderr;
  EXPECT_THAT(
      setResult.stdout,
      ::testing::HasSubstr("Successfully set default VLAN to " + targetVlan));
  XLOG(INFO) << "[Step 2] " << setResult.stdout;

  // Step 3: Commit and verify
  commitConfig();
  waitForAgentReady();
  EXPECT_EQ(getSwConfigField<int>("defaultVlan"), std::stoi(targetVlan));
  XLOG(INFO) << "[Step 3] Verified defaultVlan="
             << getSwConfigField<int>("defaultVlan");

  // Restore: reset defaultVlan first, then move ports back.
  XLOG(INFO) << "[Restore] Reverting defaultVlan and ports to "
             << currentDefault;

  // 'config vlan default' recreates currentDefault as a non-routable
  // placeholder (no interface) when the earlier default change erased it, so
  // it has to run before the ports are moved back — 'switchport access vlan'
  // throws on a VLAN that is absent from the config. Recreating the VLAN via
  // a static-mac add would instead attach a cfg::Interface to it, silently
  // turning the interface-less default VLAN state this suite's refusal test
  // depends on into an unreachable one.
  auto restoreDefault =
      runCli({"config", "vlan", "default", std::to_string(currentDefault)});
  ASSERT_EQ(restoreDefault.exitCode, 0)
      << "[Restore] Failed to reset default VLAN: " << restoreDefault.stderr;

  for (const auto& name : movedPorts) {
    runCli(
        {"config",
         "interface",
         name,
         "switchport",
         "access",
         "vlan",
         std::to_string(currentDefault)});
  }

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
  int64_t currentDefault =
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
 * The test sets up the precondition by moving an eth port onto the default
 * VLAN using `switchport access vlan`. Since the default VLAN typically has
 * no interface, this creates the "dangerous" state the guard protects against.
 */
TEST_F(ConfigVlanDefaultTest, RefuseWhenPortOnDefaultVlanWithNoInterface) {
  waitForAgentReady();

  auto initialConfig = getRunningConfig();
  int64_t currentDefault =
      initialConfig["sw"].getDefault("defaultVlan", 1).asInt();

  // Check that the default VLAN has no interface (required precondition).
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
    GTEST_SKIP() << "Default VLAN has an interface — dangerous state cannot "
                    "be created";
  }

  // Move one eth port onto the default VLAN to create the precondition.
  auto testPort = findFirstEthInterface();
  int64_t originalPortVlan = -1;
  for (const auto& port : initialConfig["sw"]["ports"]) {
    if (port.getDefault("name", "").asString() == testPort.name) {
      originalPortVlan = port.getDefault("ingressVlan", -1).asInt();
      break;
    }
  }
  ASSERT_NE(originalPortVlan, -1) << "Could not find port " << testPort.name;

  if (originalPortVlan != currentDefault) {
    auto moveResult = runCli(
        {"config",
         "interface",
         testPort.name,
         "switchport",
         "access",
         "vlan",
         std::to_string(currentDefault)});
    ASSERT_EQ(moveResult.exitCode, 0)
        << "Failed to move " << testPort.name
        << " to default VLAN: " << moveResult.stderr;
    XLOG(INFO) << "[Setup] Moved " << testPort.name << " to VLAN "
               << currentDefault;
  }

  // Now the guard should refuse: port on default VLAN, no interface.
  const std::string nextVlan = (currentDefault == 300) ? "301" : "300";
  auto result = runCli({"config", "vlan", "default", nextVlan});
  EXPECT_NE(result.exitCode, 0);
  EXPECT_THAT(
      result.stderr, ::testing::HasSubstr("Refusing to change default VLAN"));
  XLOG(INFO) << "[NegativeGuard] stderr='" << result.stderr << "'";

  // Restore the port to its original VLAN.
  if (originalPortVlan != currentDefault) {
    runCli(
        {"config",
         "interface",
         testPort.name,
         "switchport",
         "access",
         "vlan",
         std::to_string(originalPortVlan)});
  }
  discardSession();
}

/**
 * Verifies that changing the default VLAN succeeds when ports have already
 * been moved off the current default VLAN (ingressVlan != defaultVlan).
 *
 * Sets up the precondition by first moving a port onto the default VLAN,
 * then moving it off to a side VLAN, ensuring at least one port's
 * ingressVlan differs from defaultVlan.
 */
TEST_F(ConfigVlanDefaultTest, ChangeDefaultVlanWithPortInNonDefaultVlan) {
  waitForAgentReady();

  auto initialConfig = getRunningConfig();
  int64_t currentDefault =
      initialConfig["sw"].getDefault("defaultVlan", 1).asInt();

  // Find an existing non-default VLAN to use as the side VLAN.
  int64_t sideVlan = -1;
  for (const auto& vlan : initialConfig["sw"]["vlans"]) {
    int64_t vid = vlan.getDefault("id", -1).asInt();
    if (vid > 0 && vid != currentDefault) {
      sideVlan = vid;
      break;
    }
  }
  if (sideVlan == -1) {
    GTEST_SKIP() << "No VLAN other than the default found in the VLAN table";
  }
  XLOG(INFO) << "currentDefault=" << currentDefault << " sideVlan=" << sideVlan;

  // Pick a test port and record its original VLAN.
  auto testPort = findFirstEthInterface();
  int64_t originalPortVlan = -1;
  for (const auto& port : initialConfig["sw"]["ports"]) {
    if (port.getDefault("name", "").asString() == testPort.name) {
      originalPortVlan = port.getDefault("ingressVlan", -1).asInt();
      break;
    }
  }
  ASSERT_NE(originalPortVlan, -1) << "Could not find port " << testPort.name;

  // Move the port onto the default VLAN first, then off to the side VLAN.
  // This ensures at least one port has ingressVlan != defaultVlan.
  if (originalPortVlan != currentDefault) {
    auto r = runCli(
        {"config",
         "interface",
         testPort.name,
         "switchport",
         "access",
         "vlan",
         std::to_string(currentDefault)});
    ASSERT_EQ(r.exitCode, 0)
        << "Failed to move " << testPort.name << " to default VLAN";
  }
  auto moveOff = runCli(
      {"config",
       "interface",
       testPort.name,
       "switchport",
       "access",
       "vlan",
       std::to_string(sideVlan)});
  ASSERT_EQ(moveOff.exitCode, 0)
      << "Failed to move " << testPort.name << " to side VLAN " << sideVlan;
  XLOG(INFO) << "[Step 1] Moved " << testPort.name << " to VLAN " << sideVlan;

  // Set a new default VLAN — the command must succeed even when no ports
  // remain on the old default VLAN.
  const int64_t newDefault = 4093;
  auto result =
      runCli({"config", "vlan", "default", std::to_string(newDefault)});
  ASSERT_EQ(result.exitCode, 0)
      << "failed to set default VLAN: " << result.stderr;
  XLOG(INFO) << "[Step 2] " << result.stdout;

  // Step 3: Commit and verify.
  commitConfig();
  waitForAgentReady();
  EXPECT_EQ(getSwConfigField<int>("defaultVlan"), newDefault);

  // Restore: move the port back and reset defaultVlan.
  XLOG(INFO) << "[Restore] Moving " << testPort.name << " back to VLAN "
             << originalPortVlan;
  runCli(
      {"config",
       "interface",
       testPort.name,
       "switchport",
       "access",
       "vlan",
       std::to_string(originalPortVlan)});

  auto restoreDefault =
      runCli({"config", "vlan", "default", std::to_string(currentDefault)});
  ASSERT_EQ(restoreDefault.exitCode, 0)
      << "[Restore] Failed to reset default VLAN: " << restoreDefault.stderr;

  commitConfig();
  waitForAgentReady();
  discardSession();
}
