// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config interface <name> switchport access vlan <id>
 *
 * NewVlanAcrossPorts attaches a brand-new VLAN to one port (auto-create
 * path), then the now-existing VLAN to a second port, verifying ingressVlan
 * and the untagged sw.vlanPorts[] membership after each warmboot commit.
 * SetUp() snapshots the CLI's config and TearDown() restores it in a single
 * commit, so the DUT is left as found even when the test fails mid-way.
 */

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigVlanSwitchportAccessTest : public Fboss2IntegrationTest {
 protected:
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    originalConfigJson_ = snapshotConfig();
  }

  void TearDown() override {
    restoreConfig(originalConfigJson_);
    Fboss2IntegrationTest::TearDown();
  }

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

  // The port must have exactly one untagged sw.vlanPorts[] membership, in
  // the given VLAN.
  void checkUntaggedVlanPort(const std::string& portName, int vlanId) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    ASSERT_TRUE(sw.count("ports"));
    int logicalPort = -1;
    for (const auto& port : sw["ports"]) {
      if (port.count("name") && port["name"].asString() == portName) {
        ASSERT_TRUE(port.count("logicalID"));
        logicalPort = static_cast<int>(port["logicalID"].asInt());
        break;
      }
    }
    ASSERT_NE(logicalPort, -1) << "Port " << portName << " not found";
    ASSERT_TRUE(sw.count("vlanPorts")) << "Running config has no sw.vlanPorts";
    int untaggedCount = 0;
    int untaggedVlan = -1;
    for (const auto& vp : sw["vlanPorts"]) {
      if (static_cast<int>(vp["logicalPort"].asInt()) != logicalPort) {
        continue;
      }
      bool tagged = vp.count("emitTags") && vp["emitTags"].asBool();
      if (!tagged) {
        untaggedCount++;
        untaggedVlan = static_cast<int>(vp["vlanID"].asInt());
      }
    }
    EXPECT_EQ(untaggedCount, 1)
        << "Expected exactly one untagged vlanPorts entry for " << portName
        << " (logicalPort " << logicalPort << "), found " << untaggedCount;
    EXPECT_EQ(untaggedVlan, vlanId);
  }

  // First VLAN ID unused by both sw.vlans and sw.interfaces.
  static int findUnusedVlanId(const folly::dynamic& sw) {
    std::set<int> used;
    if (sw.count("vlans")) {
      for (const auto& v : sw["vlans"]) {
        if (v.count("id") && !v["id"].isNull()) {
          used.insert(static_cast<int>(v["id"].asInt()));
        }
      }
    }
    if (sw.count("interfaces")) {
      for (const auto& i : sw["interfaces"]) {
        if (i.count("vlanID") && !i["vlanID"].isNull()) {
          used.insert(static_cast<int>(i["vlanID"].asInt()));
        }
      }
    }
    for (int id = 2; id < 4094; ++id) {
      if (!used.count(id)) {
        return id;
      }
    }
    return -1;
  }

  static bool vlanExists(const folly::dynamic& config, int vlanId) {
    const auto& sw = config["sw"];
    if (!sw.count("vlans")) {
      return false;
    }
    for (const auto& v : sw["vlans"]) {
      if (v.count("id") && v["id"].asInt() == vlanId) {
        return true;
      }
    }
    return false;
  }

  std::string setAccessVlan(const std::string& portName, int vlanId) {
    auto result = runCli(
        {"config",
         "interface",
         portName,
         "switchport",
         "access",
         "vlan",
         std::to_string(vlanId)});
    EXPECT_EQ(result.exitCode, 0)
        << "Failed to set access VLAN " << vlanId << " on " << portName << ": "
        << result.stderr;
    return result.stdout;
  }

  std::string originalConfigJson_;
};

TEST_F(ConfigVlanSwitchportAccessTest, NewVlanAcrossPorts) {
  XLOG(INFO) << "[Step 1] Picking two ports and an unused VLAN...";
  auto ports = getRandomInterfacePortNames(2);
  ASSERT_EQ(ports.size(), 2) << "Need two INTERFACE_PORTs";
  const std::string& port1 = ports[0];
  const std::string& port2 = ports[1];

  auto origVlan2 = getIngressVlan(port2);
  ASSERT_TRUE(origVlan2.has_value())
      << "Port " << port2 << " has no ingressVlan in running config";

  // Pick against the snapshot, not the running config: the agent prunes
  // memberless VLANs from its running view but sessions still see them.
  int testVlan = findUnusedVlanId(folly::parseJson(originalConfigJson_)["sw"]);
  ASSERT_GT(testVlan, 0) << "No unused VLAN ID available";
  XLOG(INFO) << "  Ports: " << port1 << ", " << port2
             << "; new VLAN: " << testVlan;

  XLOG(INFO) << "[Step 2] Attaching new VLAN " << testVlan << " to " << port1
             << " (auto-create path)...";
  auto output = setAccessVlan(port1, testVlan);
  EXPECT_TRUE(
      output.find("(VLAN " + std::to_string(testVlan) + " created)") !=
      std::string::npos)
      << "Expected auto-create notice in output: " << output;
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 2] Verifying...";
  EXPECT_TRUE(vlanExists(getRunningConfig(), testVlan))
      << "VLAN " << testVlan << " not in running config after commit";
  auto readBack1 = getIngressVlan(port1);
  ASSERT_TRUE(readBack1.has_value());
  EXPECT_EQ(*readBack1, testVlan);
  checkUntaggedVlanPort(port1, testVlan);
  auto readBack2 = getIngressVlan(port2);
  ASSERT_TRUE(readBack2.has_value());
  EXPECT_EQ(*readBack2, *origVlan2);

  XLOG(INFO) << "[Step 3] Attaching now-existing VLAN " << testVlan << " to "
             << port2 << "...";
  output = setAccessVlan(port2, testVlan);
  EXPECT_TRUE(output.find("created") == std::string::npos)
      << "VLAN already exists; no auto-create notice expected: " << output;
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 3] Verifying...";
  readBack1 = getIngressVlan(port1);
  ASSERT_TRUE(readBack1.has_value());
  EXPECT_EQ(*readBack1, testVlan);
  readBack2 = getIngressVlan(port2);
  ASSERT_TRUE(readBack2.has_value());
  EXPECT_EQ(*readBack2, testVlan);
  checkUntaggedVlanPort(port1, testVlan);
  checkUntaggedVlanPort(port2, testVlan);

  XLOG(INFO) << "TEST PASSED (TearDown restores the original config)";
}
