// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2-dev config interface <name> switchport trunk
 * allowed vlan add|remove <vlan-ids>'
 *
 * This test:
 * 1. Picks an interface from the running system
 * 2. Finds an existing VLAN from the config
 * 3. Adds the VLAN to the trunk allowed list
 * 4. Adds the same VLAN again (verifies no duplicate is created)
 * 5. Removes the VLAN from trunk (VLAN still exists, only VlanPort is removed)
 *
 * Note: VLANs must be pre-configured with their corresponding interfaces.
 * This command does not auto-create or auto-delete VLANs.
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// System config path
const std::string kSystemConfigPath = "/etc/coop/agent.conf";
} // namespace

class ConfigInterfaceSwitchportTrunkAllowedVlanTest
    : public Fboss2IntegrationTest {
 protected:
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
  }

  void TearDown() override {
    // Clean up any VlanPort entries we created
    cleanupTestVlanPorts();
    Fboss2IntegrationTest::TearDown();
  }

  std::string getSessionConfigPath() const {
    // NOLINTNEXTLINE(concurrency-mt-unsafe): HOME is read-only in practice
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
      throw std::runtime_error("HOME environment variable not set");
    }
    return std::string(home) + "/.fboss2/agent.conf";
  }

  std::string getSessionMetadataPath() const {
    // NOLINTNEXTLINE(concurrency-mt-unsafe): HOME is read-only in practice
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
      throw std::runtime_error("HOME environment variable not set");
    }
    return std::string(home) + "/.fboss2/cli_metadata.json";
  }

  // Find the first port from the config.
  // Returns the port name and logical ID.
  std::pair<std::string, int> findFirstPort() {
    auto ports = findPorts(1);
    if (ports.empty()) {
      throw std::runtime_error("No ports found in config");
    }
    return ports[0];
  }

  // Find multiple ports from the config.
  // Returns up to 'count' ports as (name, logicalID) pairs.
  std::vector<std::pair<std::string, int>> findPorts(int count) {
    std::string configStr;
    if (!folly::readFile(kSystemConfigPath.c_str(), configStr)) {
      throw std::runtime_error("Failed to read system config");
    }
    auto config = folly::parseJson(configStr);
    const auto& ports = config["sw"]["ports"];
    if (ports.empty()) {
      return {};
    }

    std::vector<std::pair<std::string, int>> result;
    for (const auto& port : ports) {
      result.emplace_back(port["name"].asString(), port["logicalID"].asInt());
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
  std::vector<int> findVlansWithInterfaceNotOnPort(
      int portLogicalId,
      int count) {
    std::string configStr;
    if (!folly::readFile(kSystemConfigPath.c_str(), configStr)) {
      return {};
    }
    auto config = folly::parseJson(configStr);
    const auto& vlans = config["sw"]["vlans"];
    if (vlans.empty()) {
      return {};
    }

    // Build a set of VLAN IDs that have interfaces
    std::set<int> vlansWithInterfaces;
    if (config["sw"].count("interfaces")) {
      const auto& interfaces = config["sw"]["interfaces"];
      for (const auto& intf : interfaces) {
        if (intf.count("vlanID")) {
          vlansWithInterfaces.insert(intf["vlanID"].asInt());
        }
      }
    }

    // Build a set of VLAN IDs already associated with this port
    std::set<int> vlansOnPort;
    if (config["sw"].count("vlanPorts")) {
      const auto& vlanPorts = config["sw"]["vlanPorts"];
      for (const auto& vp : vlanPorts) {
        if (vp["logicalPort"].asInt() == portLogicalId) {
          vlansOnPort.insert(vp["vlanID"].asInt());
        }
      }
    }

    // Find VLANs that have interfaces but are NOT on this port
    std::vector<int> result;
    for (const auto& vlan : vlans) {
      int vlanId = vlan["id"].asInt();
      if (vlansWithInterfaces.count(vlanId) > 0 &&
          vlansOnPort.count(vlanId) == 0) {
        result.push_back(vlanId);
        if (static_cast<int>(result.size()) >= count) {
          break;
        }
      }
    }

    return result;
  }

  // Find multiple VLANs that have interfaces but are NOT associated with any
  // of the given ports. Returns up to 'count' VLANs.
  std::vector<int> findVlansWithInterfaceNotOnPorts(
      const std::vector<int>& portLogicalIds,
      int count) {
    std::string configStr;
    if (!folly::readFile(kSystemConfigPath.c_str(), configStr)) {
      return {};
    }
    auto config = folly::parseJson(configStr);
    const auto& vlans = config["sw"]["vlans"];
    if (vlans.empty()) {
      return {};
    }

    // Build a set of VLAN IDs that have interfaces
    std::set<int> vlansWithInterfaces;
    if (config["sw"].count("interfaces")) {
      const auto& interfaces = config["sw"]["interfaces"];
      for (const auto& intf : interfaces) {
        if (intf.count("vlanID")) {
          vlansWithInterfaces.insert(intf["vlanID"].asInt());
        }
      }
    }

    // Build a set of port logical IDs for quick lookup
    std::set<int> portIdSet(portLogicalIds.begin(), portLogicalIds.end());

    // Build a set of VLAN IDs already associated with any of the given ports
    std::set<int> vlansOnPorts;
    if (config["sw"].count("vlanPorts")) {
      const auto& vlanPorts = config["sw"]["vlanPorts"];
      for (const auto& vp : vlanPorts) {
        if (portIdSet.count(vp["logicalPort"].asInt()) > 0) {
          vlansOnPorts.insert(vp["vlanID"].asInt());
        }
      }
    }

    // Find VLANs that have interfaces but are NOT on any of the given ports
    std::vector<int> result;
    for (const auto& vlan : vlans) {
      int vlanId = vlan["id"].asInt();
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

  // Verify that a VlanPort entry exists for the given VLAN and port
  bool vlanPortExists(int vlanId, int portLogicalId) {
    std::string configStr;
    if (!folly::readFile(kSystemConfigPath.c_str(), configStr)) {
      return false;
    }
    auto config = folly::parseJson(configStr);
    const auto& vlanPorts = config["sw"]["vlanPorts"];
    for (const auto& vp : vlanPorts) {
      if (vp["vlanID"].asInt() == vlanId &&
          vp["logicalPort"].asInt() == portLogicalId) {
        return true;
      }
    }
    return false;
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
  XLOG(INFO) << "  VLAN " << *vlanId << " added to trunk allowed list";

  // Step 4: Remove the VLAN from trunk allowed list
  // Note: VLAN still exists, only VlanPort is removed
  XLOG(INFO) << "[Step 4] Removing VLAN " << *vlanId
             << " from trunk allowed list...";
  removeTrunkVlan(portName, vlanIdStr);
  XLOG(INFO) << "  VLAN " << *vlanId << " removed from trunk allowed list";

  // Step 5: Verify the VlanPort entry was deleted
  XLOG(INFO) << "[Step 5] Verifying VlanPort entry was deleted...";
  ASSERT_FALSE(vlanPortExists(*vlanId, logicalPortId))
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
  ASSERT_TRUE(vlanPortExists(vlans[0], logicalPortId))
      << "VlanPort for VLAN " << vlans[0] << " should exist";
  ASSERT_TRUE(vlanPortExists(vlans[1], logicalPortId))
      << "VlanPort for VLAN " << vlans[1] << " should exist";
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
  ASSERT_TRUE(vlanPortExists(vlans[0], logicalPortId));
  ASSERT_TRUE(vlanPortExists(vlans[1], logicalPortId));

  // Remove first VLAN
  XLOG(INFO) << "[Step 4] Removing first VLAN " << vlans[0] << "...";
  removeTrunkVlan(portName, std::to_string(vlans[0]));
  ASSERT_FALSE(vlanPortExists(vlans[0], logicalPortId))
      << "VlanPort for VLAN " << vlans[0] << " should be removed";
  ASSERT_TRUE(vlanPortExists(vlans[1], logicalPortId))
      << "VlanPort for VLAN " << vlans[1] << " should still exist";
  XLOG(INFO) << "  First VLAN removed, second still exists";

  // Remove second VLAN
  XLOG(INFO) << "[Step 5] Removing second VLAN " << vlans[1] << "...";
  removeTrunkVlan(portName, std::to_string(vlans[1]));
  ASSERT_FALSE(vlanPortExists(vlans[1], logicalPortId))
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
  ASSERT_FALSE(vlanPortExists(*vlanId, logicalPortId))
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
  ASSERT_TRUE(vlanPortExists(*vlanId, testPortLogicalIds_[0]))
      << "VlanPort for VLAN " << *vlanId << " on port "
      << testInterfaceNames_[0] << " should exist";
  ASSERT_TRUE(vlanPortExists(*vlanId, testPortLogicalIds_[1]))
      << "VlanPort for VLAN " << *vlanId << " on port "
      << testInterfaceNames_[1] << " should exist";
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
  ASSERT_TRUE(vlanPortExists(*vlanId, testPortLogicalIds_[0]));
  ASSERT_TRUE(vlanPortExists(*vlanId, testPortLogicalIds_[1]));

  // Remove VLAN from multiple ports in one command
  XLOG(INFO) << "[Step 4] Removing VLAN " << *vlanId
             << " from ports: " << folly::join(", ", testInterfaceNames_)
             << "...";
  removeTrunkVlanMultiplePorts(testInterfaceNames_, vlanIdStr);

  // Verify VlanPort was removed from both ports
  XLOG(INFO) << "[Step 5] Verifying VlanPort entries were removed...";
  ASSERT_FALSE(vlanPortExists(*vlanId, testPortLogicalIds_[0]))
      << "VlanPort for VLAN " << *vlanId << " on port "
      << testInterfaceNames_[0] << " should be removed";
  ASSERT_FALSE(vlanPortExists(*vlanId, testPortLogicalIds_[1]))
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
      ASSERT_TRUE(vlanPortExists(vlanId, testPortLogicalIds_[i]))
          << "VlanPort for VLAN " << vlanId << " on port "
          << testInterfaceNames_[i] << " should exist";
    }
  }
  XLOG(INFO) << "  All VlanPort entries created";

  XLOG(INFO) << "TEST PASSED";
}
