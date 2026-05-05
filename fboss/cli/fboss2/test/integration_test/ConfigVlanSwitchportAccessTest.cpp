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
#include <set>
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

  // Pick a testVlan that is both a real VLAN (sw.vlans[].id) and has an
  // L3 interface configured (sw.interfaces[].vlanID). Filtering on
  // sw.interfaces alone is unsafe: non-VLAN-typed interfaces (e.g.
  // system-port interfaces) appear there with vlanID=0, which the CLI
  // rejects. Using `originalVlan ± 1` blindly is also unsafe: if that ID
  // does not exist in sw.vlans / sw.interfaces, the agent's config
  // validator (`VLAN <id> has no interface, even when corresp port is
  // enabled`) SIGABRTs on the next warmboot.
  auto config = getRunningConfig();
  const auto& sw = config["sw"];
  std::set<int> realVlans;
  if (sw.count("vlans")) {
    for (const auto& v : sw["vlans"]) {
      if (v.count("id") && !v["id"].isNull()) {
        realVlans.insert(static_cast<int>(v["id"].asInt()));
      }
    }
  }
  std::set<int> candidateVlans;
  if (sw.count("interfaces")) {
    for (const auto& i : sw["interfaces"]) {
      if (i.count("vlanID") && !i["vlanID"].isNull()) {
        int id = static_cast<int>(i["vlanID"].asInt());
        if (realVlans.count(id)) {
          candidateVlans.insert(id);
        }
      }
    }
  }
  candidateVlans.erase(originalVlan);
  ASSERT_FALSE(candidateVlans.empty())
      << "Need at least 2 VLANs with L3 interfaces to test transition";
  int testVlan = *candidateVlans.begin();
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
