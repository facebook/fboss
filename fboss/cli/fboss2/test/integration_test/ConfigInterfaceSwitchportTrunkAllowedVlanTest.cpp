// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2-dev config interface <name> switchport trunk
 * allowed vlan add|remove <vlan-ids>'
 *
 * This test:
 * 1. Picks an interface from the running system
 * 2. Finds an existing VLAN from the running config
 * 3. Adds the VLAN to the trunk allowed list
 * 4. Adds the same VLAN again (verifies no duplicate is created)
 * 5. Removes the VLAN from trunk (VLAN still exists, only VlanPort is removed)
 *
 * Note: VLANs must be pre-configured with their corresponding interfaces.
 * This command does not auto-create or auto-delete VLANs.
 *
 * Commits trigger an agent warmboot restart, so post-commit verification
 * polls the running config via waitForRunningConfig() until the agent is
 * back up and the expected state is observed.
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/String.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceSwitchportTrunkAllowedVlanTest
    : public Fboss2IntegrationTest {
 protected:
  void TearDown() override {
    // Clean up any VlanPort entries we created
    cleanupTestVlanPorts();
    Fboss2IntegrationTest::TearDown();
  }

  // Find the first port from the running config.
  // Returns the port name and logical ID.
  std::pair<std::string, int> findFirstPort() {
    auto ports = findPorts(1);
    if (ports.empty()) {
      throw std::runtime_error("No ports found in running config");
    }
    return ports[0];
  }

  // Find multiple ports from the running config.
  // Returns up to 'count' ports as (name, logicalID) pairs.
  std::vector<std::pair<std::string, int>> findPorts(int count) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    std::vector<std::pair<std::string, int>> result;
    if (!sw.count("ports")) {
      return result;
    }
    for (const auto& port : sw["ports"]) {
      result.emplace_back(
          port["name"].asString(), static_cast<int>(port["logicalID"].asInt()));
      if (static_cast<int>(result.size()) >= count) {
        break;
      }
    }
    return result;
  }

  // Find a VLAN that has an interface but is NOT associated with the given
  // port. VLANs must have interfaces to be added to trunk ports.
  std::optional<int> findVlanWithInterfaceNotOnPort(int portLogicalId) {
    auto vlans = findVlansWithInterfaceNotOnPort(portLogicalId, 1);
    if (vlans.empty()) {
      return std::nullopt;
    }
    return vlans[0];
  }

  // Find a VLAN that has an interface but is NOT associated with any of the
  // given ports.
  std::optional<int> findVlanWithInterfaceNotOnPorts(
      const std::vector<int>& portLogicalIds) {
    auto vlans = findVlansWithInterfaceNotOnPorts(portLogicalIds, 1);
    if (vlans.empty()) {
      return std::nullopt;
    }
    return vlans[0];
  }

  // Find multiple VLANs that have interfaces but are NOT associated with the
  // given port. Returns up to 'count' VLANs.
  std::vector<int> findVlansWithInterfaceNotOnPort(int portLogicalId, int count)
      const {
    return findVlansWithInterfaceNotOnPorts({portLogicalId}, count);
  }

  // Find multiple VLANs that have interfaces but are NOT associated with any
  // of the given ports. Returns up to 'count' VLANs.
  std::vector<int> findVlansWithInterfaceNotOnPorts(
      const std::vector<int>& portLogicalIds,
      int count) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("vlans")) {
      return {};
    }

    // Build a set of VLAN IDs that have interfaces
    std::set<int> vlansWithInterfaces;
    if (sw.count("interfaces")) {
      for (const auto& intf : sw["interfaces"]) {
        if (intf.count("vlanID")) {
          vlansWithInterfaces.insert(static_cast<int>(intf["vlanID"].asInt()));
        }
      }
    }

    // Build a set of port logical IDs for quick lookup
    std::set<int> portIdSet(portLogicalIds.begin(), portLogicalIds.end());

    // Build a set of VLAN IDs already associated with any of the given ports
    std::set<int> vlansOnPorts;
    if (sw.count("vlanPorts")) {
      for (const auto& vp : sw["vlanPorts"]) {
        if (portIdSet.count(static_cast<int>(vp["logicalPort"].asInt())) > 0) {
          vlansOnPorts.insert(static_cast<int>(vp["vlanID"].asInt()));
        }
      }
    }

    // Find VLANs that have interfaces but are NOT on any of the given ports
    std::vector<int> result;
    for (const auto& vlan : sw["vlans"]) {
      int vlanId = static_cast<int>(vlan["id"].asInt());
      if (vlansWithInterfaces.count(vlanId) > 0 &&
          vlansOnPorts.count(vlanId) == 0) {
        result.push_back(vlanId);
        if (static_cast<int>(result.size()) >= count) {
          break;
        }
      }
    }

    return result;
  }

  // Clean up VlanPort entries we created during the test
  void cleanupTestVlanPorts() {
    if (testVlanIds_.empty() || testInterfaceNames_.empty()) {
      return; // Nothing to clean up
    }

    XLOG(INFO) << "Cleaning up test VlanPort entries...";

    // Build comma-separated list of VLAN IDs
    std::vector<std::string> vlanIdStrs;
    vlanIdStrs.reserve(testVlanIds_.size());
    for (int vlanId : testVlanIds_) {
      vlanIdStrs.push_back(std::to_string(vlanId));
    }
    std::string vlanList = folly::join(",", vlanIdStrs);

    // Use the remove command to clean up what we added
    // This is safer than manually editing the config
    auto result = runCliWithInterfaces(testInterfaceNames_, "remove", vlanList);
    if (result.exitCode == 0) {
      commitConfig();
      XLOG(INFO) << "Test VlanPort cleanup committed";
    } else {
      // VlanPort may already be removed, that's OK
      XLOG(INFO) << "VlanPort cleanup skipped (may already be removed)";
    }
  }

  // Helper to build CLI args with interface names as separate arguments
  Result runCliWithInterfaces(
      const std::vector<std::string>& interfaceNames,
      const std::string& action,
      const std::string& vlanIds) {
    std::vector<std::string> args;
    // Reserve space: "config", "interface", N interface names, 4 more args
    args.reserve(2 + interfaceNames.size() + 5);
    args.emplace_back("config");
    args.emplace_back("interface");
    // Add each interface name as a separate argument
    for (const auto& name : interfaceNames) {
      args.emplace_back(name);
    }
    args.emplace_back("switchport");
    args.emplace_back("trunk");
    args.emplace_back("allowed");
    args.emplace_back("vlan");
    args.emplace_back(action);
    args.emplace_back(vlanIds);
    return runCli(args);
  }

  void addTrunkVlan(
      const std::string& interfaceName,
      const std::string& vlanIds) {
    auto result = runCliWithInterfaces({interfaceName}, "add", vlanIds);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to add trunk VLAN: " << result.stderr;
    commitConfig();
  }

  void addTrunkVlanMultiplePorts(
      const std::vector<std::string>& interfaceNames,
      const std::string& vlanIds) {
    auto result = runCliWithInterfaces(interfaceNames, "add", vlanIds);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to add trunk VLAN: " << result.stderr;
    commitConfig();
  }

  void removeTrunkVlan(
      const std::string& interfaceName,
      const std::string& vlanIds,
      bool commit = true) {
    auto result = runCliWithInterfaces({interfaceName}, "remove", vlanIds);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to remove trunk VLAN: " << result.stderr;
    if (commit) {
      commitConfig();
    }
  }

  void removeTrunkVlanMultiplePorts(
      const std::vector<std::string>& interfaceNames,
      const std::string& vlanIds,
      bool commit = true) {
    auto result = runCliWithInterfaces(interfaceNames, "remove", vlanIds);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to remove trunk VLAN: " << result.stderr;
    if (commit) {
      commitConfig();
    }
  }

  // Returns true if the given running config has a VlanPort entry for the
  // VLAN and port with emitTags set — the core trunk semantic: trunk member
  // ports must emit tagged frames for the VLAN.
  static bool
  vlanPortExists(const folly::dynamic& config, int vlanId, int portLogicalId) {
    if (!config.isObject() || !config.count("sw")) {
      return false;
    }
    const auto& sw = config["sw"];
    if (!sw.count("vlanPorts")) {
      return false;
    }
    for (const auto& vp : sw["vlanPorts"]) {
      if (vp["vlanID"].asInt() == vlanId &&
          vp["logicalPort"].asInt() == portLogicalId && vp.count("emitTags") &&
          vp["emitTags"].asBool()) {
        return true;
      }
    }
    return false;
  }

  // Poll the agent's running config until the trunk VlanPort entry for
  // (vlanId, portLogicalId) is present (expectPresent=true) or absent
  // (expectPresent=false). Commits trigger an agent warmboot restart, so
  // this tolerates the agent being briefly unreachable. Returns true if
  // the expected state was observed before the timeout.
  bool waitForVlanPort(int vlanId, int portLogicalId, bool expectPresent)
      const {
    auto config = waitForRunningConfig(
        [&](const folly::dynamic& c) {
          return vlanPortExists(c, vlanId, portLogicalId) == expectPresent;
        },
        std::chrono::seconds(120));
    return vlanPortExists(config, vlanId, portLogicalId) == expectPresent;
  }

  // Track what we created for cleanup
  std::vector<int> testVlanIds_;
  std::vector<int> testPortLogicalIds_;
  std::vector<std::string> testInterfaceNames_;
};

TEST_F(ConfigInterfaceSwitchportTrunkAllowedVlanTest, AddAndRemoveTrunkVlan) {
  // Step 1: Find the first port
  XLOG(INFO) << "[Step 1] Finding the first port...";
  auto [portName, logicalPortId] = findFirstPort();
  XLOG(INFO) << "  Using port: " << portName;
  testInterfaceNames_.push_back(portName);
  testPortLogicalIds_.push_back(logicalPortId);
  XLOG(INFO) << "  Logical port ID: " << logicalPortId;

  // Step 2: Find a VLAN that has an interface but is not on this port
  XLOG(INFO) << "[Step 2] Finding a VLAN with interface not on this port...";
  auto vlanId = findVlanWithInterfaceNotOnPort(logicalPortId);
  if (!vlanId.has_value()) {
    GTEST_SKIP() << "No VLAN found that has an interface and is not already "
                 << "associated with port " << portName;
  }
  testVlanIds_.push_back(*vlanId);
  std::string vlanIdStr = std::to_string(*vlanId);
  XLOG(INFO) << "  Using VLAN: " << *vlanId;

  // Step 3: Add the VLAN to trunk allowed list
  XLOG(INFO) << "[Step 3] Adding VLAN " << *vlanId
             << " to trunk allowed list...";
  addTrunkVlan(portName, vlanIdStr);
  ASSERT_TRUE(waitForVlanPort(*vlanId, logicalPortId, true))
      << "VlanPort entry for VLAN " << *vlanId << " and port " << logicalPortId
      << " with emitTags should have been created";
  XLOG(INFO) << "  VLAN " << *vlanId << " added to trunk allowed list";

  // Step 4: Remove the VLAN from trunk allowed list
  // Note: VLAN still exists, only VlanPort is removed
  XLOG(INFO) << "[Step 4] Removing VLAN " << *vlanId
             << " from trunk allowed list...";
  removeTrunkVlan(portName, vlanIdStr);
  XLOG(INFO) << "  VLAN " << *vlanId << " removed from trunk allowed list";

  // Step 5: Verify the VlanPort entry was deleted
  XLOG(INFO) << "[Step 5] Verifying VlanPort entry was deleted...";
  ASSERT_TRUE(waitForVlanPort(*vlanId, logicalPortId, false))
      << "VlanPort entry for VLAN " << *vlanId << " and port " << logicalPortId
      << " should have been deleted";
  XLOG(INFO) << "  VlanPort entry deleted";

  // Clear tracking since we already cleaned up
  testVlanIds_.clear();
  testInterfaceNames_.clear();
  testPortLogicalIds_.clear();

  XLOG(INFO) << "TEST PASSED";
}

// Test: Adding multiple VLANs to trunk in a single command
TEST_F(ConfigInterfaceSwitchportTrunkAllowedVlanTest, AddMultipleVlans) {
  XLOG(INFO) << "[Step 1] Finding the first port...";
  auto [portName, logicalPortId] = findFirstPort();
  testInterfaceNames_.push_back(portName);
  testPortLogicalIds_.push_back(logicalPortId);
  XLOG(INFO) << "  Using port: " << portName;

  // Find at least 2 VLANs
  XLOG(INFO) << "[Step 2] Finding multiple VLANs with interfaces...";
  auto vlans = findVlansWithInterfaceNotOnPort(logicalPortId, 2);
  if (vlans.size() < 2) {
    GTEST_SKIP() << "Need at least 2 VLANs with interfaces not on port "
                 << portName << ", found " << vlans.size();
  }
  testVlanIds_ = vlans;
  std::string vlanList =
      std::to_string(vlans[0]) + "," + std::to_string(vlans[1]);
  XLOG(INFO) << "  Using VLANs: " << vlanList;

  // Add multiple VLANs in one command
  XLOG(INFO) << "[Step 3] Adding multiple VLANs to trunk...";
  addTrunkVlan(portName, vlanList);

  // Verify both VLANs were added
  XLOG(INFO) << "[Step 4] Verifying VlanPort entries were created...";
  ASSERT_TRUE(waitForVlanPort(vlans[0], logicalPortId, true))
      << "VlanPort for VLAN " << vlans[0] << " with emitTags should exist";
  ASSERT_TRUE(waitForVlanPort(vlans[1], logicalPortId, true))
      << "VlanPort for VLAN " << vlans[1] << " with emitTags should exist";
  XLOG(INFO) << "  Both VlanPort entries created";

  XLOG(INFO) << "TEST PASSED";
}

// Test: Adding an existing VLAN to trunk (should report no changes)
TEST_F(ConfigInterfaceSwitchportTrunkAllowedVlanTest, AddExistingVlan) {
  XLOG(INFO) << "[Step 1] Finding the first port...";
  auto [portName, logicalPortId] = findFirstPort();
  testInterfaceNames_.push_back(portName);
  testPortLogicalIds_.push_back(logicalPortId);
  XLOG(INFO) << "  Using port: " << portName;

  XLOG(INFO) << "[Step 2] Finding a VLAN with interface...";
  auto vlanId = findVlanWithInterfaceNotOnPort(logicalPortId);
  if (!vlanId.has_value()) {
    GTEST_SKIP() << "No VLAN found that has an interface and is not already "
                 << "associated with port " << portName;
  }
  testVlanIds_.push_back(*vlanId);
  std::string vlanIdStr = std::to_string(*vlanId);
  XLOG(INFO) << "  Using VLAN: " << *vlanId;

  // Add the VLAN first
  XLOG(INFO) << "[Step 3] Adding VLAN " << *vlanId << " to trunk...";
  addTrunkVlan(portName, vlanIdStr);
  ASSERT_TRUE(waitForVlanPort(*vlanId, logicalPortId, true))
      << "VlanPort for VLAN " << *vlanId << " with emitTags should exist";

  // Add the same VLAN again - should report no changes
  XLOG(INFO) << "[Step 4] Adding same VLAN again (should report no changes)...";
  auto result = runCli(
      {"config",
       "interface",
       portName,
       "switchport",
       "trunk",
       "allowed",
       "vlan",
       "add",
       vlanIdStr});
  ASSERT_EQ(result.exitCode, 0)
      << "Failed to add trunk VLAN: " << result.stderr;
  EXPECT_TRUE(
      result.stdout.find("No changes made") != std::string::npos ||
      result.stdout.find("already") != std::string::npos)
      << "Expected 'No changes made' or 'already' in output: " << result.stdout;
  XLOG(INFO) << "  Result: " << result.stdout;

  XLOG(INFO) << "TEST PASSED";
}

// Test: Removing VLANs one by one from trunk
TEST_F(ConfigInterfaceSwitchportTrunkAllowedVlanTest, RemoveVlansOneByOne) {
  XLOG(INFO) << "[Step 1] Finding the first port...";
  auto [portName, logicalPortId] = findFirstPort();
  testInterfaceNames_.push_back(portName);
  testPortLogicalIds_.push_back(logicalPortId);
  XLOG(INFO) << "  Using port: " << portName;

  // Find at least 2 VLANs
  XLOG(INFO) << "[Step 2] Finding multiple VLANs with interfaces...";
  auto vlans = findVlansWithInterfaceNotOnPort(logicalPortId, 2);
  if (vlans.size() < 2) {
    GTEST_SKIP() << "Need at least 2 VLANs with interfaces not on port "
                 << portName << ", found " << vlans.size();
  }
  testVlanIds_ = vlans;
  std::string vlanList =
      std::to_string(vlans[0]) + "," + std::to_string(vlans[1]);
  XLOG(INFO) << "  Using VLANs: " << vlanList;

  // Add both VLANs
  XLOG(INFO) << "[Step 3] Adding VLANs to trunk...";
  addTrunkVlan(portName, vlanList);
  ASSERT_TRUE(waitForVlanPort(vlans[0], logicalPortId, true));
  ASSERT_TRUE(waitForVlanPort(vlans[1], logicalPortId, true));

  // Remove first VLAN
  XLOG(INFO) << "[Step 4] Removing first VLAN " << vlans[0] << "...";
  removeTrunkVlan(portName, std::to_string(vlans[0]));
  ASSERT_TRUE(waitForVlanPort(vlans[0], logicalPortId, false))
      << "VlanPort for VLAN " << vlans[0] << " should be removed";
  ASSERT_TRUE(vlanPortExists(getRunningConfig(), vlans[1], logicalPortId))
      << "VlanPort for VLAN " << vlans[1] << " should still exist";
  XLOG(INFO) << "  First VLAN removed, second still exists";

  // Remove second VLAN
  XLOG(INFO) << "[Step 5] Removing second VLAN " << vlans[1] << "...";
  removeTrunkVlan(portName, std::to_string(vlans[1]));
  ASSERT_TRUE(waitForVlanPort(vlans[1], logicalPortId, false))
      << "VlanPort for VLAN " << vlans[1] << " should be removed";
  XLOG(INFO) << "  Second VLAN removed";

  // Clear tracking since we already cleaned up
  testVlanIds_.clear();
  testInterfaceNames_.clear();
  testPortLogicalIds_.clear();

  XLOG(INFO) << "TEST PASSED";
}

// Test: Removing a non-existing VLAN from trunk (should report no changes)
TEST_F(ConfigInterfaceSwitchportTrunkAllowedVlanTest, RemoveNonExistingVlan) {
  XLOG(INFO) << "[Step 1] Finding the first port...";
  auto [portName, logicalPortId] = findFirstPort();
  testInterfaceNames_.push_back(portName);
  testPortLogicalIds_.push_back(logicalPortId);
  XLOG(INFO) << "  Using port: " << portName;

  XLOG(INFO) << "[Step 2] Finding a VLAN with interface...";
  auto vlanId = findVlanWithInterfaceNotOnPort(logicalPortId);
  if (!vlanId.has_value()) {
    GTEST_SKIP() << "No VLAN found that has an interface and is not already "
                 << "associated with port " << portName;
  }
  std::string vlanIdStr = std::to_string(*vlanId);
  XLOG(INFO) << "  Using VLAN: " << *vlanId;

  // Verify the VLAN is not on the port (it shouldn't be since we found it)
  ASSERT_FALSE(vlanPortExists(getRunningConfig(), *vlanId, logicalPortId))
      << "VLAN " << *vlanId << " should not be on port " << portName;

  // Try to remove a VLAN that's not on the trunk - should report no changes
  XLOG(INFO) << "[Step 3] Removing non-existing VLAN (should report no "
             << "changes)...";
  auto result = runCli(
      {"config",
       "interface",
       portName,
       "switchport",
       "trunk",
       "allowed",
       "vlan",
       "remove",
       vlanIdStr});
  ASSERT_EQ(result.exitCode, 0)
      << "Failed to remove trunk VLAN: " << result.stderr;
  EXPECT_TRUE(
      result.stdout.find("No changes made") != std::string::npos ||
      result.stdout.find("not associated") != std::string::npos)
      << "Expected 'No changes made' or 'not associated' in output: "
      << result.stdout;
  XLOG(INFO) << "  Result: " << result.stdout;

  // No cleanup needed since we didn't add anything
  testInterfaceNames_.clear();
  testPortLogicalIds_.clear();

  XLOG(INFO) << "TEST PASSED";
}

// Test: Adding a VLAN to multiple ports in a single command
TEST_F(ConfigInterfaceSwitchportTrunkAllowedVlanTest, AddVlanToMultiplePorts) {
  XLOG(INFO) << "[Step 1] Finding multiple ports...";
  auto ports = findPorts(2);
  if (ports.size() < 2) {
    GTEST_SKIP() << "Need at least 2 ports, found " << ports.size();
  }
  for (const auto& [name, logicalId] : ports) {
    testInterfaceNames_.push_back(name);
    testPortLogicalIds_.push_back(logicalId);
  }
  XLOG(INFO) << "  Using ports: " << testInterfaceNames_[0] << ", "
             << testInterfaceNames_[1];

  // Find a VLAN that is not on any of these ports
  XLOG(INFO) << "[Step 2] Finding a VLAN with interface not on any port...";
  auto vlanId = findVlanWithInterfaceNotOnPorts(testPortLogicalIds_);
  if (!vlanId.has_value()) {
    GTEST_SKIP() << "No VLAN found that has an interface and is not already "
                 << "associated with any of the ports";
  }
  testVlanIds_.push_back(*vlanId);
  std::string vlanIdStr = std::to_string(*vlanId);
  XLOG(INFO) << "  Using VLAN: " << *vlanId;

  // Add VLAN to multiple ports in one command (ports as separate arguments)
  XLOG(INFO) << "[Step 3] Adding VLAN " << *vlanId
             << " to ports: " << folly::join(", ", testInterfaceNames_)
             << "...";
  addTrunkVlanMultiplePorts(testInterfaceNames_, vlanIdStr);

  // Verify VlanPort was created on both ports
  XLOG(INFO) << "[Step 4] Verifying VlanPort entries were created...";
  ASSERT_TRUE(waitForVlanPort(*vlanId, testPortLogicalIds_[0], true))
      << "VlanPort for VLAN " << *vlanId << " on port "
      << testInterfaceNames_[0] << " with emitTags should exist";
  ASSERT_TRUE(waitForVlanPort(*vlanId, testPortLogicalIds_[1], true))
      << "VlanPort for VLAN " << *vlanId << " on port "
      << testInterfaceNames_[1] << " with emitTags should exist";
  XLOG(INFO) << "  VlanPort entries created on both ports";

  XLOG(INFO) << "TEST PASSED";
}

// Test: Removing a VLAN from multiple ports in a single command
TEST_F(
    ConfigInterfaceSwitchportTrunkAllowedVlanTest,
    RemoveVlanFromMultiplePorts) {
  XLOG(INFO) << "[Step 1] Finding multiple ports...";
  auto ports = findPorts(2);
  if (ports.size() < 2) {
    GTEST_SKIP() << "Need at least 2 ports, found " << ports.size();
  }
  for (const auto& [name, logicalId] : ports) {
    testInterfaceNames_.push_back(name);
    testPortLogicalIds_.push_back(logicalId);
  }
  XLOG(INFO) << "  Using ports: " << testInterfaceNames_[0] << ", "
             << testInterfaceNames_[1];

  // Find a VLAN that is not on any of these ports
  XLOG(INFO) << "[Step 2] Finding a VLAN with interface not on any port...";
  auto vlanId = findVlanWithInterfaceNotOnPorts(testPortLogicalIds_);
  if (!vlanId.has_value()) {
    GTEST_SKIP() << "No VLAN found that has an interface and is not already "
                 << "associated with any of the ports";
  }
  testVlanIds_.push_back(*vlanId);
  std::string vlanIdStr = std::to_string(*vlanId);
  XLOG(INFO) << "  Using VLAN: " << *vlanId;

  // Add VLAN to multiple ports first
  XLOG(INFO) << "[Step 3] Adding VLAN " << *vlanId
             << " to ports: " << folly::join(", ", testInterfaceNames_)
             << "...";
  addTrunkVlanMultiplePorts(testInterfaceNames_, vlanIdStr);
  ASSERT_TRUE(waitForVlanPort(*vlanId, testPortLogicalIds_[0], true));
  ASSERT_TRUE(waitForVlanPort(*vlanId, testPortLogicalIds_[1], true));

  // Remove VLAN from multiple ports in one command
  XLOG(INFO) << "[Step 4] Removing VLAN " << *vlanId
             << " from ports: " << folly::join(", ", testInterfaceNames_)
             << "...";
  removeTrunkVlanMultiplePorts(testInterfaceNames_, vlanIdStr);

  // Verify VlanPort was removed from both ports
  XLOG(INFO) << "[Step 5] Verifying VlanPort entries were removed...";
  ASSERT_TRUE(waitForVlanPort(*vlanId, testPortLogicalIds_[0], false))
      << "VlanPort for VLAN " << *vlanId << " on port "
      << testInterfaceNames_[0] << " should be removed";
  ASSERT_TRUE(waitForVlanPort(*vlanId, testPortLogicalIds_[1], false))
      << "VlanPort for VLAN " << *vlanId << " on port "
      << testInterfaceNames_[1] << " should be removed";
  XLOG(INFO) << "  VlanPort entries removed from both ports";

  // Clear tracking since we already cleaned up
  testVlanIds_.clear();
  testInterfaceNames_.clear();
  testPortLogicalIds_.clear();

  XLOG(INFO) << "TEST PASSED";
}

// Test: Adding multiple VLANs to multiple ports in a single command
TEST_F(
    ConfigInterfaceSwitchportTrunkAllowedVlanTest,
    AddMultipleVlansToMultiplePorts) {
  XLOG(INFO) << "[Step 1] Finding multiple ports...";
  auto ports = findPorts(2);
  if (ports.size() < 2) {
    GTEST_SKIP() << "Need at least 2 ports, found " << ports.size();
  }
  for (const auto& [name, logicalId] : ports) {
    testInterfaceNames_.push_back(name);
    testPortLogicalIds_.push_back(logicalId);
  }
  XLOG(INFO) << "  Using ports: " << testInterfaceNames_[0] << ", "
             << testInterfaceNames_[1];

  // Find multiple VLANs that are not on any of these ports
  XLOG(INFO) << "[Step 2] Finding multiple VLANs not on any port...";
  auto vlans = findVlansWithInterfaceNotOnPorts(testPortLogicalIds_, 2);
  if (vlans.size() < 2) {
    GTEST_SKIP() << "Need at least 2 VLANs with interfaces not on any port, "
                 << "found " << vlans.size();
  }
  testVlanIds_ = vlans;
  std::string vlanList =
      std::to_string(vlans[0]) + "," + std::to_string(vlans[1]);
  XLOG(INFO) << "  Using VLANs: " << vlanList;

  // Add multiple VLANs to multiple ports in one command
  XLOG(INFO) << "[Step 3] Adding VLANs " << vlanList
             << " to ports: " << folly::join(", ", testInterfaceNames_)
             << "...";
  addTrunkVlanMultiplePorts(testInterfaceNames_, vlanList);

  // Verify VlanPort was created for all VLAN-port combinations
  XLOG(INFO) << "[Step 4] Verifying VlanPort entries were created...";
  for (int vlanId : vlans) {
    for (size_t i = 0; i < testPortLogicalIds_.size(); ++i) {
      ASSERT_TRUE(waitForVlanPort(vlanId, testPortLogicalIds_[i], true))
          << "VlanPort for VLAN " << vlanId << " on port "
          << testInterfaceNames_[i] << " with emitTags should exist";
    }
  }
  XLOG(INFO) << "  All VlanPort entries created";

  XLOG(INFO) << "TEST PASSED";
}
