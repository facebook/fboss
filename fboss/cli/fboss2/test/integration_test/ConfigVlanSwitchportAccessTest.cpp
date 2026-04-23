// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config interface <name> switchport access vlan <id>
 *
 * Gets the current access VLAN, sets a different one, verifies via
 * 'show interface', and restores the original. Commits trigger warmboot
 * so the test waits for agent recovery.
 *
 * Distinct from ConfigSessionRestartTest::CommitTriggersWarmboot which
 * uses the same command as a vehicle for testing the restart signal;
 * this test focuses on the feature's read-back correctness.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigVlanSwitchportAccessTest : public Fboss2IntegrationTest {
 protected:
  // 'switchport access vlan' writes to sw.ports[].ingressVlan in the running
  // config. show-interface's 'vlan' field is derived from a different source
  // (L3 interface / portID mapping), so is not a reliable read-back.
  std::optional<int> getIngressVlan(const std::string& portName) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("ports")) {
      return std::nullopt;
    }
    for (const auto& port : sw["ports"]) {
      if (port.count("name") && port["name"].asString() == portName) {
        if (port.count("ingressVlan") && !port["ingressVlan"].isNull()) {
          return static_cast<int>(port["ingressVlan"].asInt());
        }
        return std::nullopt;
      }
    }
    return std::nullopt;
  }
};

TEST_F(ConfigVlanSwitchportAccessTest, SetAndVerifyAccessVlan) {
  XLOG(INFO) << "[Step 1] Finding an interface...";
  Interface intf = findFirstEthInterface();
  auto originalOpt = getIngressVlan(intf.name);
  ASSERT_TRUE(originalOpt.has_value())
      << "Port " << intf.name << " has no ingressVlan in running config";
  int originalVlan = *originalOpt;
  int testVlan = (originalVlan == 4094) ? originalVlan - 1 : originalVlan + 1;
  XLOG(INFO) << "  Using " << intf.name << " (ingressVlan: " << originalVlan
             << " -> " << testVlan << ")";

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
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 3] Verifying ingressVlan in running config...";
  auto readBack = getIngressVlan(intf.name);
  ASSERT_TRUE(readBack.has_value());
  EXPECT_EQ(*readBack, testVlan);

  XLOG(INFO) << "[Step 4] Restoring original VLAN (" << originalVlan << ")...";
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

  auto restored = getIngressVlan(intf.name);
  ASSERT_TRUE(restored.has_value());
  EXPECT_EQ(*restored, originalVlan);
  XLOG(INFO) << "TEST PASSED";
}
