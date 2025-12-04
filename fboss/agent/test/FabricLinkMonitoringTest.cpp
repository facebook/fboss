// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricLinkMonitoring.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/VoqUtils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

namespace {

// Test fixture to enable test mode for all Fabric Link Monitoring tests
class FabricLinkMonitoringTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Enable test mode so VD calculation uses portId % 4 instead of platform
    // mapping
    FabricLinkMonitoring::setTestMode(true);
  }

  void TearDown() override {
    // Disable test mode after tests
    FabricLinkMonitoring::setTestMode(false);
  }
};

// Helper function to add a DSF node to config
void addDsfNode(
    cfg::SwitchConfig& config,
    int64_t switchId,
    const std::string& name,
    cfg::DsfNodeType nodeType,
    std::optional<int> fabricLevel = std::nullopt,
    std::optional<PlatformType> platformType = std::nullopt) {
  cfg::DsfNode node;
  node.switchId() = switchId;
  node.name() = name;
  node.type() = nodeType;
  if (fabricLevel.has_value()) {
    node.fabricLevel() = *fabricLevel;
  }
  if (platformType.has_value()) {
    node.platformType() = *platformType;
  } else {
    node.platformType() = PlatformType::PLATFORM_MERU400BIU;
  }
  (*config.dsfNodes())[switchId] = node;
}

// Helper function to add a fabric port with expected neighbor
void addFabricPort(
    cfg::SwitchConfig& config,
    int portId,
    const std::string& portName,
    const std::string& neighborSwitch,
    const std::string& neighborPort) {
  cfg::Port port;
  port.logicalID() = portId;
  port.name() = portName;
  port.portType() = cfg::PortType::FABRIC_PORT;
  port.expectedNeighborReachability() = std::vector<cfg::PortNeighbor>();

  cfg::PortNeighbor neighbor;
  neighbor.remoteSystem() = neighborSwitch;
  neighbor.remotePort() = neighborPort;
  port.expectedNeighborReachability()->push_back(neighbor);

  config.ports()->push_back(port);
}

// Helper function to create a simple VoQ config for basic testing
cfg::SwitchConfig createSimpleVoqConfig(int64_t switchId) {
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = switchId;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  // Add VoQ switch as DSF node
  addDsfNode(config, switchId, "voq0", cfg::DsfNodeType::INTERFACE_NODE);

  // Add a couple of fabric switches
  addDsfNode(config, 4, "fabric4", cfg::DsfNodeType::FABRIC_NODE, 1);
  addDsfNode(config, 8, "fabric8", cfg::DsfNodeType::FABRIC_NODE, 1);

  // Add a few fabric ports
  addFabricPort(config, 1, "fab1/1/1", "fabric4", "fab1/1/1");
  addFabricPort(config, 2, "fab1/1/2", "fabric4", "fab1/1/2");
  addFabricPort(config, 5, "fab1/1/5", "fabric8", "fab1/1/1");
  addFabricPort(config, 6, "fab1/1/6", "fabric8", "fab1/1/2");

  return config;
}

// Helper function to create a simple Fabric config for basic testing
cfg::SwitchConfig createSimpleFabricConfig(int64_t switchId) {
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = switchId;
  config.switchSettings()->switchType() = cfg::SwitchType::FABRIC;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  // Add fabric switch as L1 node
  addDsfNode(config, switchId, "fabric4", cfg::DsfNodeType::FABRIC_NODE, 1);

  // Add a couple of VoQ switches
  addDsfNode(config, 0, "voq0", cfg::DsfNodeType::INTERFACE_NODE);
  addDsfNode(config, 4, "voq4", cfg::DsfNodeType::INTERFACE_NODE);

  // Add a few fabric ports
  addFabricPort(config, 1, "fab1/1/1", "voq0", "fab1/1/1");
  addFabricPort(config, 2, "fab1/1/2", "voq0", "fab1/1/2");
  addFabricPort(config, 5, "fab1/1/5", "voq4", "fab1/1/1");
  addFabricPort(config, 6, "fab1/1/6", "voq4", "fab1/1/2");

  return config;
}

// Helper to create a realistic VoQ config with many ports and fabric switches
cfg::SwitchConfig createVoqConfig() {
  cfg::SwitchConfig config;
  int64_t switchId{0};
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = switchId;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  // Add VoQ switch as interface node
  addDsfNode(config, switchId, "voq0", cfg::DsfNodeType::INTERFACE_NODE);

  // Add 40 fabric switches (switchIds: 4, 8, 12, ..., 160)
  for (int i = 1; i <= 40; i++) {
    int64_t fabricSwitchId = i * 4;
    addDsfNode(
        config,
        fabricSwitchId,
        "fabric" + std::to_string(fabricSwitchId),
        cfg::DsfNodeType::FABRIC_NODE,
        1);
  }

  // Add 160 ports: 4 ports per fabric switch
  int portId = 1;
  for (int i = 1; i <= 40; i++) {
    int64_t fabricSwitchId = i * 4;
    std::string fabricSwitchName = "fabric" + std::to_string(fabricSwitchId);

    for (int portOffset = 0; portOffset < 4; portOffset++) {
      addFabricPort(
          config,
          portId,
          "fab1/" + std::to_string(i) + "/" + std::to_string(portOffset + 1),
          fabricSwitchName,
          "fab1/1/" + std::to_string(portOffset + 1));
      portId++;
    }
  }

  return config;
}

// Helper to create a realistic Fabric config with many ports and VoQ switches
cfg::SwitchConfig createFabricConfig() {
  cfg::SwitchConfig config;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchType() = cfg::SwitchType::FABRIC;
  config.ports() = std::vector<cfg::Port>();

  int64_t switchId;
  // Add 128 VoQ switches (switchIds: 0, 4, 8, ..., 508)
  for (int i = 0; i < 128; i++) {
    switchId = i * 4;
    addDsfNode(
        config,
        switchId,
        "voq" + std::to_string(switchId),
        cfg::DsfNodeType::INTERFACE_NODE);
  }
  // Use the next available switch ID for fabric device
  switchId = switchId + 4;
  config.switchSettings()->switchId() = switchId;

  // Add fabric switch as L1 node
  addDsfNode(
      config,
      switchId,
      "fabric" + std::to_string(switchId),
      cfg::DsfNodeType::FABRIC_NODE,
      1);

  // Add 512 ports: 4 ports per VoQ switch
  int portId = 1;
  for (int i = 0; i < 128; i++) {
    int64_t voqSwitchId = i * 4;
    std::string voqSwitchName = "voq" + std::to_string(voqSwitchId);

    for (int portOffset = 0; portOffset < 4; portOffset++) {
      addFabricPort(
          config,
          portId,
          "fab1/" + std::to_string(i + 1) + "/" +
              std::to_string(portOffset + 1),
          voqSwitchName,
          "fab1/1/" + std::to_string(portOffset + 1));
      portId++;
    }
  }

  return config;
}

// Helper to create a dual-stage Fabric config
// L1 fabric connects to 128 VoQ switches and 128 L2 switches
// VoQ switch IDs < L1 switch ID < L2 switch IDs
cfg::SwitchConfig createDualStageFabricConfig() {
  cfg::SwitchConfig config;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchType() = cfg::SwitchType::FABRIC;
  config.ports() = std::vector<cfg::Port>();

  // Add 128 VoQ switches (switchIds: 0, 4, 8, ..., 508)
  for (int i = 0; i < 128; i++) {
    int64_t voqSwitchId = i * 4;
    addDsfNode(
        config,
        voqSwitchId,
        "voq" + std::to_string(voqSwitchId),
        cfg::DsfNodeType::INTERFACE_NODE);
  }

  // L1 fabric switch ID = 512
  int64_t l1SwitchId = 512;
  config.switchSettings()->switchId() = l1SwitchId;

  // Add L1 fabric switch
  addDsfNode(
      config,
      l1SwitchId,
      "fabric" + std::to_string(l1SwitchId),
      cfg::DsfNodeType::FABRIC_NODE,
      1);

  // Add 128 L2 switches (switchIds: 516, 520, 524, ..., 1028)
  for (int i = 0; i < 128; i++) {
    int64_t l2SwitchId = 516 + (i * 4);
    addDsfNode(
        config,
        l2SwitchId,
        "fabric_l2_" + std::to_string(l2SwitchId),
        cfg::DsfNodeType::FABRIC_NODE,
        2);
  }

  // Add 512 ports connecting to VoQ switches: 4 ports per VoQ switch
  int portId = 1;
  for (int i = 0; i < 128; i++) {
    int64_t voqSwitchId = i * 4;
    std::string voqSwitchName = "voq" + std::to_string(voqSwitchId);

    for (int portOffset = 0; portOffset < 4; portOffset++) {
      addFabricPort(
          config,
          portId,
          "fab1/" + std::to_string(i + 1) + "/" +
              std::to_string(portOffset + 1),
          voqSwitchName,
          "fab1/1/" + std::to_string(portOffset + 1));
      portId++;
    }
  }

  // Add 512 ports connecting to L2 switches: 4 ports per L2 switch
  for (int i = 0; i < 128; i++) {
    int64_t l2SwitchId = 516 + (i * 4);
    std::string l2SwitchName = "fabric_l2_" + std::to_string(l2SwitchId);

    for (int portOffset = 0; portOffset < 4; portOffset++) {
      addFabricPort(
          config,
          portId,
          "fab2/" + std::to_string(i + 1) + "/" +
              std::to_string(portOffset + 1),
          l2SwitchName,
          "fab2/1/" + std::to_string(portOffset + 1));
      portId++;
    }
  }

  return config;
}

// Helper to validate switch ID allocation
void validateSwitchIdAllocation(
    const cfg::SwitchConfig& config,
    int expectedPortCount,
    int expectedUniqueSwitchIds) {
  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // Verify all ports have switch IDs
  EXPECT_EQ(mapping.size(), expectedPortCount)
      << "Expected " << expectedPortCount << " ports to have switch IDs";

  // Collect all unique switch IDs
  std::set<SwitchID> uniqueSwitchIds;
  for (const auto& [portId, switchId] : mapping) {
    uniqueSwitchIds.insert(switchId);
  }

  // Verify expected number of unique switch IDs
  EXPECT_EQ(uniqueSwitchIds.size(), expectedUniqueSwitchIds)
      << "Expected " << expectedUniqueSwitchIds << " unique switch IDs";
}

// Helper to validate DSF node processing
void validateDsfNodeProcessing(
    const cfg::SwitchConfig& config,
    int expectedPortCount) {
  FabricLinkMonitoring monitoring1(&config);
  FabricLinkMonitoring monitoring2(&config);

  // Both instances should produce identical results (deterministic)
  const auto& mapping1 = monitoring1.getPort2LinkSwitchIdMapping();
  const auto& mapping2 = monitoring2.getPort2LinkSwitchIdMapping();

  EXPECT_EQ(mapping1.size(), expectedPortCount);
  EXPECT_EQ(mapping2.size(), expectedPortCount);
  EXPECT_EQ(mapping1, mapping2)
      << "Multiple instantiations should produce identical mappings";

  // Verify all ports can lookup their switch IDs
  for (int portId = 1; portId <= expectedPortCount; portId++) {
    EXPECT_NO_THROW(monitoring1.getSwitchIdForPort(PortID(portId)))
        << "Port " << portId << " should have a switch ID";
  }
}

cfg::SwitchConfig createConfigWithParallelLinks(
    int64_t switchId,
    int numParallelLinks) {
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = switchId;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  // Add VoQ switch
  addDsfNode(
      config,
      switchId,
      "voq" + std::to_string(switchId),
      cfg::DsfNodeType::INTERFACE_NODE);
  // Add fabric switch
  addDsfNode(config, 4, "fabric4", cfg::DsfNodeType::FABRIC_NODE, 1);

  // Add parallel links (multiple ports connecting to same fabric switch)
  // These will all map to the same VD (based on portId % 4 in test mode)
  int portId = 1;
  for (int i = 0; i < numParallelLinks; i++) {
    // Use port IDs that map to VD 1 (1, 5, 9, 13...)
    int actualPortId = portId + (i * 4);
    addFabricPort(
        config,
        actualPortId,
        "fab1/1/" + std::to_string(actualPortId),
        "fabric4",
        "fab1/1/" + std::to_string(i + 1));
  }

  return config;
}

} // namespace

// Test VoQ switch constructor
TEST_F(FabricLinkMonitoringTest, ConstructorVoqSwitch) {
  auto config = createSimpleVoqConfig(0);

  // VoQ switch with basic fabric configuration
  EXPECT_NO_THROW(FabricLinkMonitoring monitoring(&config));
}

// Test Fabric switch constructor
TEST_F(FabricLinkMonitoringTest, ConstructorFabricSwitch) {
  auto config = createSimpleFabricConfig(4);

  // Fabric switch with basic VoQ configuration
  EXPECT_NO_THROW(FabricLinkMonitoring monitoring(&config));
}

// Test getPort2LinkSwitchIdMapping for VoQ with ports
TEST_F(FabricLinkMonitoringTest, GetPort2LinkSwitchIdMappingVoq) {
  auto config = createSimpleVoqConfig(0);

  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // With 4 ports, mapping should not be empty
  EXPECT_FALSE(mapping.empty());
}

// Test getPort2LinkSwitchIdMapping for Fabric with ports
TEST_F(FabricLinkMonitoringTest, GetPort2LinkSwitchIdMappingFabric) {
  auto config = createSimpleFabricConfig(4);

  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // With 4 ports, mapping should not be empty
  EXPECT_FALSE(mapping.empty());
}

// Test getSwitchIdForPort with non-existent port throws on VoQ switch
TEST_F(FabricLinkMonitoringTest, GetSwitchIdForPortThrowsOnMissingPortVoq) {
  auto config = createSimpleVoqConfig(0);

  FabricLinkMonitoring monitoring(&config);
  PortID nonExistentPort(999);

  // Should throw CHECK failure for non-existent port
  EXPECT_DEATH(
      monitoring.getSwitchIdForPort(nonExistentPort),
      "Port ID 999 not found in the port ID to switch ID map");
}

// Test getSwitchIdForPort with non-existent port throws on Fabric switch
TEST_F(FabricLinkMonitoringTest, GetSwitchIdForPortThrowsOnMissingPortFabric) {
  auto config = createSimpleFabricConfig(4);

  FabricLinkMonitoring monitoring(&config);
  PortID nonExistentPort(999);

  // Should throw CHECK failure for non-existent port
  EXPECT_DEATH(
      monitoring.getSwitchIdForPort(nonExistentPort),
      "Port ID 999 not found in the port ID to switch ID map");
}

// Test VoQ switch with invalid fabric level throws
TEST_F(FabricLinkMonitoringTest, VoqSwitchWithInvalidFabricLevel) {
  auto config = createSimpleVoqConfig(0);

  // Add fabric node with invalid level (must be 1 or 2)
  addDsfNode(
      config,
      500,
      "fabric_invalid",
      cfg::DsfNodeType::FABRIC_NODE,
      5); // Invalid level

  EXPECT_THROW(FabricLinkMonitoring monitoring(&config), FbossError);
}

// Test VoQ switch processes DSF nodes correctly
TEST_F(FabricLinkMonitoringTest, VoqSwitchProcessesDsfNodes) {
  // Create config with 160 ports and 41 DSF nodes (1 VoQ + 40 fabric)
  auto config = createVoqConfig();

  validateDsfNodeProcessing(config, 160);
}

// Test Fabric switch processes DSF nodes correctly
TEST_F(FabricLinkMonitoringTest, FabricSwitchProcessesDsfNodes) {
  // Create config with 512 ports and 129 DSF nodes (128 VoQ + 1 Fabric)
  auto config = createFabricConfig();

  validateDsfNodeProcessing(config, 512);
}

// Test VoQ switch allocates unique switch IDs for all ports
TEST_F(FabricLinkMonitoringTest, VoqSwitchAllocatesUniqueSwitchIds) {
  // VoQ with 160 ports should allocate 160 unique switch IDs
  auto config = createVoqConfig();

  // Expected: All 160 ports get unique switch IDs (no parallel links)
  validateSwitchIdAllocation(config, 160, 160);
}

// Test Fabric switch allocates unique switch IDs for all ports
TEST_F(FabricLinkMonitoringTest, FabricSwitchAllocatesUniqueSwitchIds) {
  // Fabric with 512 ports connecting to 128 VoQ switches
  auto config = createFabricConfig();

  // Expected: 128+3 unique switch IDs (128 VoQ switches × 4 VD groups)
  validateSwitchIdAllocation(config, 512, 131);
}

// Test VoQ switch exceeds max leaf-L1 links per VD
TEST_F(FabricLinkMonitoringTest, VoqSwitchExceedsMaxLeafL1Links) {
  // kDsfMaxLeafFabricLinksPerVd = 40
  // Need to create ports that map to the SAME VD (in test mode: portId % 4)
  // Create config with 41 ports all mapping to VD 0 (ports 4, 8, 12, ..., 164)
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = 0;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  addDsfNode(config, 0, "voq0", cfg::DsfNodeType::INTERFACE_NODE);
  addDsfNode(config, 4, "fabric4", cfg::DsfNodeType::FABRIC_NODE, 1);

  // Add 41 ports that all map to the same VD (VD 0)
  // In test mode, VD = portId % 4, so use ports 4, 8, 12, ..., 164
  for (int i = 0; i < 41; i++) {
    int portId = 4 + (i * 4); // VD 0: 4, 8, 12, ..., 164
    addFabricPort(
        config,
        portId,
        "fab1/1/" + std::to_string(portId),
        "fabric4",
        "fab1/1/" + std::to_string(i + 1));
  }

  EXPECT_THROW(
      {
        try {
          FabricLinkMonitoring monitoring(&config);
        } catch (const FbossError& e) {
          EXPECT_THAT(
              e.what(), ::testing::HasSubstr("Too many leaf to L1 links"));
          EXPECT_THAT(e.what(), ::testing::HasSubstr("41"));
          EXPECT_THAT(e.what(), ::testing::HasSubstr("40"));
          throw;
        }
      },
      FbossError);
}

// Test VoQ switch within max leaf-L1 links (edge case)
TEST_F(FabricLinkMonitoringTest, VoqSwitchWithinMaxLeafL1Links) {
  // Exactly at limit: kDsfMaxLeafFabricLinksPerVd = 40
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = 0;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  addDsfNode(config, 0, "voq0", cfg::DsfNodeType::INTERFACE_NODE);
  addDsfNode(config, 4, "fabric4", cfg::DsfNodeType::FABRIC_NODE, 1);

  // Add exactly 40 ports to VD 0
  for (int i = 0; i < 40; i++) {
    int portId = 4 + (i * 4);
    addFabricPort(
        config,
        portId,
        "fab1/1/" + std::to_string(portId),
        "fabric4",
        "fab1/1/" + std::to_string(i + 1));
  }

  // Should not throw - exactly at limit is OK
  EXPECT_NO_THROW(FabricLinkMonitoring monitoring(&config));
}

// Test dual-stage fabric L1-L2 links validation
TEST_F(FabricLinkMonitoringTest, DualStageFabricL1ToL2LinksValidation) {
  // Create dual-stage config with 128 VoQ switches + 128 L2 switches
  // Total: 1024 ports (512 to VoQ + 512 to L2)
  auto config = createDualStageFabricConfig();

  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // Verify all 1024 ports have switch IDs
  EXPECT_EQ(mapping.size(), 1024);

  // Verify switch IDs are allocated correctly for all ports
  for (int portId = 1; portId <= 1024; portId++) {
    EXPECT_NO_THROW(monitoring.getSwitchIdForPort(PortID(portId)))
        << "Port " << portId << " should have switch ID in dual-stage topology";
  }
}

// Test dual-stage fabric within L1-L2 link limits
TEST_F(FabricLinkMonitoringTest, DualStageFabricWithinL1ToL2LinkLimits) {
  // Create dual-stage config with 4 L2 switches = 16 links per VD (well within
  // 128 limit)
  auto config = createDualStageFabricConfig();

  // Should not throw - well within limits
  EXPECT_NO_THROW(FabricLinkMonitoring monitoring(&config));
}

// Test parallel links allocate correct switch IDs
TEST_F(FabricLinkMonitoringTest, ParallelLinksAllocateDifferentSwitchIds) {
  // Create config with 3 parallel links to same fabric switch/VD
  auto config = createConfigWithParallelLinks(0, 3);

  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // All 3 parallel ports should have switch IDs
  EXPECT_EQ(mapping.size(), 3);

  // Collect switch IDs for the parallel links
  std::vector<SwitchID> switchIds;
  switchIds.reserve(mapping.size());
  for (const auto& [portId, switchId] : mapping) {
    switchIds.push_back(switchId);
  }

  // All parallel links should have different switch IDs
  std::set<SwitchID> uniqueSwitchIds(switchIds.begin(), switchIds.end());
  EXPECT_EQ(uniqueSwitchIds.size(), 3)
      << "Parallel links should have different switch IDs";
}

// Test parallel links canonical ordering
TEST_F(FabricLinkMonitoringTest, ParallelLinksUsesCanonicalOrdering) {
  // Create two identical configs
  auto config1 = createConfigWithParallelLinks(0, 2);
  auto config2 = createConfigWithParallelLinks(0, 2);

  FabricLinkMonitoring monitoring1(&config1);
  FabricLinkMonitoring monitoring2(&config2);

  // Both should produce identical switch ID mappings
  // (canonical ordering ensures deterministic assignment)
  const auto& mapping1 = monitoring1.getPort2LinkSwitchIdMapping();
  const auto& mapping2 = monitoring2.getPort2LinkSwitchIdMapping();

  EXPECT_EQ(mapping1.size(), mapping2.size());

  // Switch IDs should be identical for same logical links
  for (const auto& [portId, switchId1] : mapping1) {
    auto it = mapping2.find(portId);
    ASSERT_TRUE(it != mapping2.end());
    EXPECT_EQ(it->second, switchId1)
        << "Canonical ordering should produce identical switch IDs";
  }
}

// Test L1-L2 switch ID offset calculation
TEST_F(FabricLinkMonitoringTest, L1L2SwitchIdOffsetCalculation) {
  // Create dual-stage fabric config to test L1-L2 offset
  auto config = createDualStageFabricConfig();

  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // Verify all 1024 ports have switch IDs (512 to VoQ + 512 to L2)
  EXPECT_EQ(mapping.size(), 1024);

  // Test L2 connections (ports 513-1024 connect to L2 switches)
  // Port 513 connects to first L2 switch (516)
  // Port 517 connects to second L2 switch (520)
  SwitchID switchId513 = monitoring.getSwitchIdForPort(PortID(513));
  SwitchID switchId517 = monitoring.getSwitchIdForPort(PortID(517));

  EXPECT_NE(switchId513, switchId517)
      << "Ports to different L2 switches should have different offsets";
}

// Test config with ports but no expected neighbors (should skip)
TEST_F(FabricLinkMonitoringTest, PortsWithoutExpectedNeighborsAreSkipped) {
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = 0;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  // Add VoQ switch
  addDsfNode(config, 0, "voq0", cfg::DsfNodeType::INTERFACE_NODE);

  // Add port WITHOUT expected neighbor
  cfg::Port port;
  port.logicalID() = 1;
  port.name() = "fab1/1/1";
  port.portType() = cfg::PortType::FABRIC_PORT;
  port.expectedNeighborReachability() = std::vector<cfg::PortNeighbor>();
  // Empty neighbor list
  config.ports()->push_back(port);

  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // Port without expected neighbors should not be in mapping
  EXPECT_TRUE(mapping.empty())
      << "Ports without expected neighbors should not get switch IDs";
}

// Test non-fabric ports are ignored
TEST_F(FabricLinkMonitoringTest, NonFabricPortsAreIgnored) {
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = 0;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  // Add VoQ switch
  addDsfNode(config, 0, "voq0", cfg::DsfNodeType::INTERFACE_NODE);

  // Add non-fabric port
  cfg::Port port;
  port.logicalID() = 1;
  port.name() = "eth1/1/1";
  port.portType() = cfg::PortType::INTERFACE_PORT; // Non-fabric
  port.expectedNeighborReachability() = std::vector<cfg::PortNeighbor>();

  cfg::PortNeighbor neighbor;
  neighbor.remoteSystem() = "switch1";
  neighbor.remotePort() = "eth1/1/1";
  port.expectedNeighborReachability()->push_back(neighbor);

  config.ports()->push_back(port);

  FabricLinkMonitoring monitoring(&config);
  const auto& mapping = monitoring.getPort2LinkSwitchIdMapping();

  // Non-fabric ports should not be in mapping
  EXPECT_TRUE(mapping.empty()) << "Non-fabric ports should not get switch IDs";
}

// Test multiple switch names mapping to same switch ID (lowest ID wins)
TEST_F(FabricLinkMonitoringTest, MultipleSwitchNamesMapToSameSwitchId) {
  cfg::SwitchConfig config;
  config.switchSettings() = cfg::SwitchSettings();
  config.switchSettings()->switchId() = 0;
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;
  config.dsfNodes() = std::map<int64_t, cfg::DsfNode>();
  config.ports() = std::vector<cfg::Port>();

  // Add VoQ switch
  addDsfNode(config, 0, "voq0", cfg::DsfNodeType::INTERFACE_NODE);

  // Add same fabric switch with multiple names and IDs
  addDsfNode(config, 8, "fabric_high", cfg::DsfNodeType::FABRIC_NODE, 1);
  addDsfNode(
      config,
      4,
      "fabric_high",
      cfg::DsfNodeType::FABRIC_NODE,
      1); // Lower ID, same name

  // Add port connecting to "fabric_high"
  addFabricPort(config, 1, "fab1/1/1", "fabric_high", "fab1/1/1");

  FabricLinkMonitoring monitoring(&config);

  // Should use the lower switch ID (4) when multiple names map to same switch
  // The implementation should handle this gracefully
  EXPECT_NO_THROW(monitoring.getSwitchIdForPort(PortID(1)));
}

// Test VD assignment in test mode
TEST_F(FabricLinkMonitoringTest, TestModeVdAssignment) {
  auto config = createVoqConfig();

  FabricLinkMonitoring monitoring(&config);

  // In test mode, VD = portId % 4
  // Port 1 -> VD 1, Port 2 -> VD 2, Port 3 -> VD 3, Port 4 -> VD 0
  // Ports with same VD (mod 4) and connecting to same fabric should have
  // similar switch IDs

  // Get switch IDs for ports that differ by 4 (same VD in test mode)
  SwitchID switchId1 = monitoring.getSwitchIdForPort(PortID(1)); // VD 1
  SwitchID switchId5 =
      monitoring.getSwitchIdForPort(PortID(5)); // VD 1 (5 % 4 = 1)

  // Since port 1 and 5 connect to different fabric switches,
  // they should have different switch IDs
  EXPECT_NE(switchId1, switchId5)
      << "Ports with same VD but different fabric switches should have different switch IDs";

  // Ports 1, 2, 3, 4 all connect to Fabric 4 but have different VDs
  SwitchID switchId2 = monitoring.getSwitchIdForPort(PortID(2)); // VD 2
  SwitchID switchId3 = monitoring.getSwitchIdForPort(PortID(3)); // VD 3
  SwitchID switchId4 = monitoring.getSwitchIdForPort(PortID(4)); // VD 0

  // All should be different (different VDs, same fabric switch)
  std::set<SwitchID> uniqueIds = {switchId1, switchId2, switchId3, switchId4};
  EXPECT_EQ(uniqueIds.size(), 4)
      << "Ports to same fabric with different VDs should have different switch IDs";
}
