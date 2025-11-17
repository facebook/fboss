// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricLinkMonitoring.h"
#include <gtest/gtest.h>
#include "fboss/agent/FbossError.h"
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
