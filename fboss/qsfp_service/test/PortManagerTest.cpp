// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/PortManager.h"
#include <folly/testing/TestUtil.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"
#include "fboss/qsfp_service/test/FakeConfigsHelper.h"
#include "fboss/qsfp_service/test/MockManagerConstructorArgs.h"
#include "fboss/qsfp_service/test/MockPhyManager.h"
#include "fboss/qsfp_service/test/MockPortManager.h"

namespace facebook::fboss {

class PortManagerTest : public ::testing::Test {
 public:
  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
  std::string qsfpSvcVolatileDir = tmpDir.path().string();
  std::string qsfpCfgPath = qsfpSvcVolatileDir + "/fakeQsfpConfig";

  void SetUp() override {
    gflags::SetCommandLineOptionWithMode(
        "port_manager_mode", "t", gflags::SET_FLAGS_DEFAULT);

    gflags::SetCommandLineOptionWithMode(
        "qsfp_service_volatile_dir",
        qsfpSvcVolatileDir.c_str(),
        gflags::SET_FLAGS_DEFAULT);

    setupFakeQsfpConfig(qsfpCfgPath);

    initManagers();
  }

 protected:
  void initManagers(
      int numModules = 1,
      int numPortsPerModule = 4,
      bool setPhyManager = false) {
    const auto platformMapping =
        makeFakePlatformMapping(numModules, numPortsPerModule);
    const std::shared_ptr<const PlatformMapping> castedPlatformMapping =
        platformMapping;

    // Create Threads Object
    const auto threadsMap = makeSlotThreadHelper(platformMapping);

    std::unique_ptr<MockPhyManager> phyManager =
        std::make_unique<MockPhyManager>(platformMapping.get());
    phyManager_ = phyManager.get();
    transceiverManager_ = std::make_shared<MockWedgeManager>(
        numModules, 4, platformMapping, threadsMap);
    portManager_ = std::make_unique<MockPortManager>(
        transceiverManager_.get(),
        setPhyManager ? std::move(phyManager) : nullptr,
        platformMapping,
        threadsMap);

    transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
        overrideMultiPortTcvrToPortAndProfile_);
  }
  void validatePortStatusNotInTestingOverride(PortID portId) {
    const auto& overrideStatusMap =
        portManager_->getOverrideAgentPortStatusForTesting();
    auto it = overrideStatusMap.find(portId);
    ASSERT_EQ(it, overrideStatusMap.end())
        << "Port ID " << portId << " found in override status map";
  }
  void validatePortStatusInTestingOverride(
      PortID portId,
      bool expectedUp,
      bool expectedEnabled) {
    const auto& overrideStatusMap =
        portManager_->getOverrideAgentPortStatusForTesting();
    auto it = overrideStatusMap.find(portId);
    ASSERT_NE(it, overrideStatusMap.end())
        << "Port ID " << portId << " not found in override status map";
    const auto& portStatus = it->second;

    ASSERT_EQ(portStatus.portEnabled, expectedEnabled)
        << "Port enabled status mismatch for port " << portId;
    ASSERT_EQ(portStatus.operState, expectedUp)
        << "Port operational state mismatch for port " << portId;
  }

  void programInternalPhyPortsForTest() {
    // Set up transceiver override for testing
    transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
        overrideMultiPortTcvrToPortAndProfile_);

    // Test that programInternalPhyPorts uses the override for testing
    EXPECT_NO_THROW(portManager_->programInternalPhyPorts(TransceiverID(0)));
  }

  const TransceiverID tcvrId_ = TransceiverID(0);
  const PortID portId1_ = PortID(1);
  const PortID portId3_ = PortID(3);

  const cfg::PortProfileID multiPortProfile_ =
      cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL;
  const TransceiverManager::OverrideTcvrToPortAndProfile
      overrideMultiPortTcvrToPortAndProfile_ = {
          {tcvrId_,
           {
               {portId1_, multiPortProfile_},
               {portId3_, multiPortProfile_},
           }}};

  // Keeping as raw pointer only for testing mocks.
  MockPhyManager* phyManager_{};
  std::shared_ptr<MockWedgeManager> transceiverManager_;
  std::unique_ptr<MockPortManager> portManager_;
};

TEST_F(PortManagerTest, getLowestIndexedPortForTransceiverPortGroup) {
  // Single Tcvr - Single Port
  // Initialized
  initManagers(1, 1);
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      PortID(1));

  // Uninitialized
  initManagers(1, 1);
  ASSERT_THROW(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      FbossError);

  // Single Tcvr – Multi Port
  initManagers(1, 4);
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  portManager_->setPortEnabledStatusInCache(PortID(3), true);
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      PortID(1));
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(3)),
      PortID(1));

  // Uninitialized
  initManagers(1, 4);
  ASSERT_THROW(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      FbossError);
  ASSERT_THROW(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(3)),
      FbossError);

  initManagers(1, 4);
  portManager_->setPortEnabledStatusInCache(PortID(3), true);
  // Not enabled, but this is the expected behavior anyways.
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      PortID(3));
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(3)),
      PortID(3));

  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

TEST_F(PortManagerTest, getLowestIndexedTransceiverForPort) {
  // Single Tcvr – Single Port
  initManagers(1, 1);
  ASSERT_EQ(
      portManager_->getLowestIndexedTransceiverForPort(PortID(1)),
      TransceiverID(0));

  // Single Tcvr – Multi Port
  initManagers(1, 4);
  ASSERT_EQ(
      portManager_->getLowestIndexedTransceiverForPort(PortID(1)),
      TransceiverID(0));
  ASSERT_EQ(
      portManager_->getLowestIndexedTransceiverForPort(PortID(3)),
      TransceiverID(0));

  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

TEST_F(PortManagerTest, isLowestIndexedPortForTransceiverPortGroup) {
  // Single Tcvr - Single Port
  // Initialized
  initManagers(1, 1);
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  ASSERT_TRUE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)));
  // Unitialized
  initManagers(1, 1);
  ASSERT_THROW(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      FbossError);

  // Single Tcvr – Multi Port
  // Initialized
  initManagers(1, 4);
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  portManager_->setPortEnabledStatusInCache(PortID(3), true);
  ASSERT_TRUE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)));
  ASSERT_FALSE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(3)));

  // Unitialized
  initManagers(1, 4);
  ASSERT_THROW(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      FbossError);
  ASSERT_THROW(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(3)),
      FbossError);

  initManagers(1, 4);
  portManager_->setPortEnabledStatusInCache(PortID(3), true);
  // Not enabled, but this is the expected behavior anyways.
  ASSERT_FALSE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)));
  ASSERT_TRUE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(3)));

  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

TEST_F(PortManagerTest, getTransceiverIdsForPort) {
  // Single Tcvr – Single Port
  initManagers(1, 1);
  ASSERT_THAT(
      portManager_->getTransceiverIdsForPort(PortID(1)),
      ::testing::ElementsAre(TransceiverID(0)));
  // Single Tcvr – Multi Port
  initManagers(1, 4);
  ASSERT_THAT(
      portManager_->getTransceiverIdsForPort(PortID(1)),
      ::testing::ElementsAre(TransceiverID(0)));
  ASSERT_THAT(
      portManager_->getTransceiverIdsForPort(PortID(3)),
      ::testing::ElementsAre(TransceiverID(0)));
  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

// need to add more testing for tcvroverride too, any combinations that mihgt
// be relevant here
TEST_F(PortManagerTest, setBothPortsEnabledAndUpForTesting) {
  initManagers(1, 4);
  portManager_->setOverrideAgentPortStatusForTesting(
      {PortID(1), PortID(3)}, {PortID(1), PortID(3)});

  validatePortStatusInTestingOverride(PortID(1), true, true);
  validatePortStatusInTestingOverride(PortID(3), true, true);
}

TEST_F(PortManagerTest, setBothPortsDisabledAndDownForTesting) {
  initManagers(1, 4);
  portManager_->setOverrideAgentPortStatusForTesting({}, {});

  validatePortStatusInTestingOverride(PortID(1), false, false);
  validatePortStatusInTestingOverride(PortID(3), false, false);
}

TEST_F(PortManagerTest, setPortsMixedStatusForTesting) {
  initManagers(1, 4);
  portManager_->setOverrideAgentPortStatusForTesting({PortID(1)}, {PortID(3)});

  validatePortStatusInTestingOverride(PortID(1), true, false);
  validatePortStatusInTestingOverride(PortID(3), false, true);
}

TEST_F(PortManagerTest, clearOverrideAgentPortStatusForTesting) {
  // needs to be modified to account for clear case
  initManagers(1, 4);
  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
      overrideMultiPortTcvrToPortAndProfile_);
  portManager_->setOverrideAgentPortStatusForTesting(
      {PortID(1), PortID(3)}, {PortID(1), PortID(3)});

  validatePortStatusInTestingOverride(PortID(1), true, true);
  validatePortStatusInTestingOverride(PortID(3), true, true);

  portManager_->setOverrideAgentPortStatusForTesting({}, {}, true);

  validatePortStatusNotInTestingOverride(PortID(1));
  validatePortStatusNotInTestingOverride(PortID(3));
}

TEST_F(PortManagerTest, getCachedXphyPorts) {
  initManagers(1, 1, true);
  ASSERT_THAT(
      portManager_->getCachedXphyPortsForTest(),
      ::testing::UnorderedElementsAre(PortID(1)));
}

TEST_F(PortManagerTest, getPortIDByPortName) {
  // Test valid port name returns correct PortID
  auto portId = portManager_->getPortIDByPortName("eth1/1/1");
  ASSERT_TRUE(portId.has_value());
  ASSERT_EQ(*portId, PortID(1));

  // Test invalid port name returns nullopt
  ASSERT_FALSE(portManager_->getPortIDByPortName("invalid_port").has_value());
}

TEST_F(PortManagerTest, getPortIDByPortNameOrThrow) {
  // Test valid port name returns correct PortID
  ASSERT_EQ(portManager_->getPortIDByPortNameOrThrow("eth1/1/1"), PortID(1));

  // Test invalid port name throws FbossError
  ASSERT_THROW(
      portManager_->getPortIDByPortNameOrThrow("invalid_port"), FbossError);
}

TEST_F(PortManagerTest, getPortNameByPortId) {
  // Test valid port ID returns correct port name
  ASSERT_EQ(portManager_->getPortNameByPortId(PortID(1)), "eth1/1/1");
  ASSERT_EQ(portManager_->getPortNameByPortId(PortID(3)), "eth1/1/3");

  // Test invalid port ID returns nullopt
  ASSERT_EQ(portManager_->getPortNameByPortId(PortID(999)), std::nullopt);
  ASSERT_THROW(
      portManager_->getPortNameByPortIdOrThrow(PortID(999)), FbossError);
}

TEST_F(PortManagerTest, portNameIdMappingConsistency) {
  // Test round-trip consistency for multiple ports
  for (const auto& portId : {PortID(1), PortID(3)}) {
    auto portNameStr = portManager_->getPortNameByPortIdOrThrow(portId);
    ASSERT_EQ(portManager_->getPortIDByPortNameOrThrow(portNameStr), portId);

    auto optionalPortId = portManager_->getPortIDByPortName(portNameStr);
    ASSERT_TRUE(optionalPortId.has_value());
    ASSERT_EQ(*optionalPortId, portId);
  }
}

TEST_F(PortManagerTest, testSetupPortNameToPortIDMapWithNullPlatformMapping) {
  // Attempt to create PortManager with null platform mapping
  ASSERT_THROW(
      std::make_unique<PortManager>(
          transceiverManager_.get(),
          nullptr /* phyManager */,
          nullptr /* platformMapping */,
          nullptr /* threads */),
      FbossError);
}

TEST_F(PortManagerTest, testGetPortNameByPortIdOrThrowWithInvalidId) {
  // Initialize with default settings
  initManagers(4);

  // Try to get port name for an invalid port ID
  // This should throw since the port ID doesn't exist
  ASSERT_THROW(
      portManager_->getPortNameByPortIdOrThrow(PortID(999)), FbossError)
      << "Should throw FbossError for invalid port ID";

  // Verify valid port IDs still work
  ASSERT_NO_THROW(portManager_->getPortNameByPortIdOrThrow(PortID(1)));
  ASSERT_NO_THROW(portManager_->getPortNameByPortIdOrThrow(PortID(3)));
}

ACTION(ThrowFbossError) {
  throw FbossError("Mock FbossError");
}
TEST_F(PortManagerTest, getInterfacePhyInfo) {
  initManagers(16, 8);
  std::map<std::string, phy::PhyInfo> phyInfos;
  // Test a valid interface
  portManager_->getInterfacePhyInfo(phyInfos, "eth1/1/1");
  EXPECT_TRUE(phyInfos.find("eth1/1/1") != phyInfos.end());
  // Simulate an exception from xphyInfo and confirm that the map doesn't
  // include that interface's key
  EXPECT_CALL(*portManager_, getXphyInfo(PortID(9)))
      .Times(1)
      .WillRepeatedly(ThrowFbossError());
  portManager_->getInterfacePhyInfo(phyInfos, "eth1/2/1");
  EXPECT_EQ(phyInfos.size(), 1);
  // We still expect the previous interface to exist in the map
  EXPECT_TRUE(phyInfos.find("eth1/1/1") != phyInfos.end());

  // Test an invalid interface
  EXPECT_THROW(
      portManager_->getInterfacePhyInfo(phyInfos, "no_such_interface"),
      FbossError);
}

TEST_F(PortManagerTest, programInternalPhyPorts) {
  initManagers(1, 4, false /* setPhyManager */);
  programInternalPhyPortsForTest();

  // Verify that the programmed port info is updated
  auto portToPortInfoPtr =
      transceiverManager_->getSynchronizedProgrammedIphyPortToPortInfo(
          TransceiverID(0));
  ASSERT_NE(portToPortInfoPtr, nullptr);

  auto lockedPortInfo = portToPortInfoPtr->rlock();
  EXPECT_EQ(lockedPortInfo->size(), 2); // Two ports in override

  auto port1Info = lockedPortInfo->find(PortID(1));
  ASSERT_NE(port1Info, lockedPortInfo->end());
  EXPECT_EQ(
      port1Info->second.profile,
      cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL);

  auto port3Info = lockedPortInfo->find(PortID(3));
  ASSERT_NE(port3Info, lockedPortInfo->end());
  EXPECT_EQ(
      port3Info->second.profile,
      cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL);
}

TEST_F(PortManagerTest, programXphyPort) {
  initManagers(1, 4, true /* setPhyManager */);

  // Test successful programming with valid transceiver info
  EXPECT_CALL(
      *phyManager_,
      programOnePort(
          PortID(1),
          cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
          ::testing::_,
          false))
      .Times(1);

  EXPECT_NO_THROW(portManager_->programXphyPort(
      PortID(1), cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL));
  // We don't throw if port is not an XPHY port.
  EXPECT_NO_THROW(portManager_->programXphyPort(
      PortID(1000), cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL));
}

TEST_F(PortManagerTest, programXphyPortNoPhyManager) {
  initManagers(1, 4, false /* setPhyManager */);

  // Test that FbossError is thrown when PhyManager is not set
  EXPECT_THROW(
      portManager_->programXphyPort(
          PortID(1), cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL),
      FbossError);
}

TEST_F(PortManagerTest, programExternalPhyPorts) {
  initManagers(1, 4, true /* setPhyManager */);
  programInternalPhyPortsForTest();

  // Add ports to the initialized ports cache - this is normally done by the
  // state machine during port initialization
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  portManager_->setPortEnabledStatusInCache(PortID(3), true);

  // programExternalPhyPorts should program all ports for the transceiver
  // Based on overrideMultiPortTcvrToPortAndProfile_, TransceiverID(0) has ports
  // 1 and 3
  EXPECT_CALL(
      *phyManager_,
      programOnePort(
          PortID(1),
          cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
          ::testing::_,
          false))
      .Times(1);
  EXPECT_CALL(
      *phyManager_,
      programOnePort(
          PortID(3),
          cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
          ::testing::_,
          false))
      .Times(1);

  EXPECT_NO_THROW(
      portManager_->programExternalPhyPorts(TransceiverID(0), false));
  // We don't throw if the transceiver doesn't have XPHY ports.
  EXPECT_NO_THROW(
      portManager_->programExternalPhyPorts(TransceiverID(1000), false));
}

TEST_F(PortManagerTest, programExternalPhyPortsNoPhyManager) {
  // Test the early return when phyManager_ is null
  initManagers(1, 4, false /* setPhyManager */);

  // Should return early without throwing when phyManager_ is null
  EXPECT_NO_THROW(
      portManager_->programExternalPhyPorts(TransceiverID(0), false));
}

TEST_F(PortManagerTest, initAndExit) {
  initManagers();
  transceiverManager_->init();
  portManager_->init();
}

TEST_F(PortManagerTest, initAndExitGracefully) {
  initManagers();
  transceiverManager_->init();
  portManager_->init();

  portManager_->gracefulExit();
  transceiverManager_->gracefulExit();
}

TEST_F(PortManagerTest, tcvrToInitializedPortsCacheValidation) {
  initManagers(2, 4); // 2 transceivers, 4 ports each

  // Initially, the cache should be empty for all transceivers
  EXPECT_TRUE(portManager_->hasTransceiverInCache(TransceiverID(0)));
  EXPECT_TRUE(portManager_->hasTransceiverInCache(TransceiverID(1)));
  EXPECT_EQ(
      portManager_->getInitializedPortsForTransceiver(TransceiverID(0)).size(),
      0);
  EXPECT_EQ(
      portManager_->getInitializedPortsForTransceiver(TransceiverID(1)).size(),
      0);

  // Test Case 1: Enable some ports for transceivers
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  portManager_->setPortEnabledStatusInCache(PortID(2), true);
  portManager_->setPortEnabledStatusInCache(PortID(5), true);

  // Verify cache updates
  auto tcvr0Ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  auto tcvr1Ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(1));

  EXPECT_EQ(tcvr0Ports.size(), 2);
  EXPECT_TRUE(tcvr0Ports.count(PortID(1)));
  EXPECT_TRUE(tcvr0Ports.count(PortID(2)));

  EXPECT_EQ(tcvr1Ports.size(), 1);
  EXPECT_TRUE(tcvr1Ports.count(PortID(5)));

  // Test Case 2: Disable some ports
  portManager_->setPortEnabledStatusInCache(PortID(1), false);
  portManager_->setPortEnabledStatusInCache(PortID(6), true);

  // Verify cache updates after disabling
  tcvr0Ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  tcvr1Ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(1));

  EXPECT_EQ(tcvr0Ports.size(), 1);
  EXPECT_FALSE(tcvr0Ports.count(PortID(1))); // Should be removed
  EXPECT_TRUE(tcvr0Ports.count(PortID(2))); // Should still be there

  EXPECT_EQ(tcvr1Ports.size(), 2);
  EXPECT_TRUE(tcvr1Ports.count(PortID(5))); // Should still be there
  EXPECT_TRUE(tcvr1Ports.count(PortID(6))); // Should be added

  // Test Case 3: Test getTransceiversWithAllPortsInSet function
  std::unordered_set<PortID> testSet1 = {PortID(2), PortID(5), PortID(6)};
  auto transceivers1 = portManager_->getTransceiversWithAllPortsInSet(testSet1);

  // TransceiverID(0) has only port 2, which is in testSet1 → should be included
  // TransceiverID(1) has ports 5,6, both are in testSet1 → should be included
  EXPECT_EQ(transceivers1.size(), 2);
  EXPECT_TRUE(transceivers1.count(TransceiverID(0)));
  EXPECT_TRUE(transceivers1.count(TransceiverID(1)));

  // Test Case 4: Test with partial match
  std::unordered_set<PortID> testSet2 = {PortID(2), PortID(5)};
  auto transceivers2 = portManager_->getTransceiversWithAllPortsInSet(testSet2);

  // TransceiverID(0) has only port 2, which is in testSet2 → should be included
  // TransceiverID(1) has ports 5,6, but 6 is not in testSet2 → should NOT be
  // included
  EXPECT_EQ(transceivers2.size(), 1);
  EXPECT_TRUE(transceivers2.count(TransceiverID(0)));
  EXPECT_FALSE(transceivers2.count(TransceiverID(1)));

  // Test Case 5: Test with empty set
  std::unordered_set<PortID> emptySet;
  auto transceivers3 = portManager_->getTransceiversWithAllPortsInSet(emptySet);

  // No transceiver should match an empty set (since they all have some ports)
  EXPECT_EQ(transceivers3.size(), 0);

  // Test Case 6: Test with superset
  std::unordered_set<PortID> superSet = {
      PortID(1),
      PortID(2),
      PortID(3),
      PortID(4),
      PortID(5),
      PortID(6),
      PortID(7),
      PortID(8)};
  auto transceivers4 = portManager_->getTransceiversWithAllPortsInSet(superSet);

  // Both transceivers should match since all their ports are in the superset
  EXPECT_EQ(transceivers4.size(), 2);
  EXPECT_TRUE(transceivers4.count(TransceiverID(0)));
  EXPECT_TRUE(transceivers4.count(TransceiverID(1)));

  // Test Case 7: Clear all ports for one transceiver
  portManager_->setPortEnabledStatusInCache(PortID(2), false);

  tcvr0Ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  EXPECT_EQ(tcvr0Ports.size(), 0);
}

TEST_F(PortManagerTest, tcvrToInitializedPortsCacheEdgeCases) {
  initManagers(1, 4); // 1 transceiver, 4 ports - valid configuration

  // Test Case 1: Enable and disable the same port multiple times
  portManager_->setPortEnabledStatusInCache(PortID(1), true);

  auto ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  EXPECT_EQ(ports.size(), 1);
  EXPECT_TRUE(ports.count(PortID(1)));

  // Disable it
  portManager_->setPortEnabledStatusInCache(PortID(1), false);

  ports = portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  EXPECT_EQ(ports.size(), 0);

  // Enable it again
  portManager_->setPortEnabledStatusInCache(PortID(1), true);

  ports = portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  EXPECT_EQ(ports.size(), 1);
  EXPECT_TRUE(ports.count(PortID(1)));

  // Test Case 2: Test invalid transceiver ID handling
  // This should throw an exception since we only have TransceiverID(0) in our
  // setup
  EXPECT_THROW(
      portManager_->setPortEnabledStatusInCache(PortID(999), true), FbossError);

  // Test Case 3: Batch operations
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  portManager_->setPortEnabledStatusInCache(PortID(2), true);
  portManager_->setPortEnabledStatusInCache(PortID(1), false);

  ports = portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  EXPECT_EQ(ports.size(), 1);
  EXPECT_FALSE(ports.count(PortID(1))); // Should be false due to last operation
  EXPECT_TRUE(ports.count(PortID(2))); // Should be true
}

TEST_F(PortManagerTest, setOverrideAllAgentPortStatusForTesting) {
  initManagers(1, 4);

  // Test various combinations and verify they apply to all ports
  portManager_->setOverrideAllAgentPortStatusForTesting(true, true);
  validatePortStatusInTestingOverride(PortID(1), true, true);
  validatePortStatusInTestingOverride(PortID(3), true, true);

  portManager_->setOverrideAllAgentPortStatusForTesting(false, false);
  validatePortStatusInTestingOverride(PortID(1), false, false);
  validatePortStatusInTestingOverride(PortID(3), false, false);

  portManager_->setOverrideAllAgentPortStatusForTesting(false, true);
  validatePortStatusInTestingOverride(PortID(1), false, true);
  validatePortStatusInTestingOverride(PortID(3), false, true);

  // Test that it clears existing individual overrides
  portManager_->setOverrideAgentPortStatusForTesting({PortID(1)}, {PortID(3)});
  portManager_->setOverrideAllAgentPortStatusForTesting(true, true);
  validatePortStatusInTestingOverride(PortID(1), true, true);
  validatePortStatusInTestingOverride(PortID(3), true, true);
}

TEST_F(PortManagerTest, tcvrToInitializedPortsCacheConsistency) {
  initManagers(2, 4); // 2 transceivers, 4 ports each - valid configuration

  // Test that the cache correctly handles multiple transceivers
  portManager_->setPortEnabledStatusInCache(PortID(1), true);
  portManager_->setPortEnabledStatusInCache(PortID(2), true);
  portManager_->setPortEnabledStatusInCache(PortID(5), true);
  portManager_->setPortEnabledStatusInCache(PortID(6), true);

  // Verify each transceiver's cache independently
  auto tcvr0Ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(0));
  auto tcvr1Ports =
      portManager_->getInitializedPortsForTransceiver(TransceiverID(1));

  EXPECT_EQ(tcvr0Ports.size(), 2);
  EXPECT_TRUE(tcvr0Ports.count(PortID(1)));
  EXPECT_TRUE(tcvr0Ports.count(PortID(2)));

  EXPECT_EQ(tcvr1Ports.size(), 2);
  EXPECT_TRUE(tcvr1Ports.count(PortID(5)));
  EXPECT_TRUE(tcvr1Ports.count(PortID(6)));

  // Test getTransceiversWithAllPortsInSet with various combinations
  std::unordered_set<PortID> testCombination1 = {PortID(1), PortID(2)};
  auto result1 =
      portManager_->getTransceiversWithAllPortsInSet(testCombination1);
  EXPECT_EQ(result1.size(), 1); // Only TransceiverID(0)
  EXPECT_TRUE(result1.count(TransceiverID(0)));
  EXPECT_FALSE(result1.count(TransceiverID(1)));

  std::unordered_set<PortID> testCombination2 = {PortID(5), PortID(6)};
  auto result2 =
      portManager_->getTransceiversWithAllPortsInSet(testCombination2);
  EXPECT_EQ(result2.size(), 1); // Only TransceiverID(1)
  EXPECT_TRUE(result2.count(TransceiverID(1)));
  EXPECT_FALSE(result2.count(TransceiverID(0)));

  // Test with a combination that matches all
  std::unordered_set<PortID> allPorts = {
      PortID(1), PortID(2), PortID(5), PortID(6)};
  auto resultAll = portManager_->getTransceiversWithAllPortsInSet(allPorts);
  EXPECT_EQ(resultAll.size(), 2); // Both transceivers
  EXPECT_TRUE(resultAll.count(TransceiverID(0)));
  EXPECT_TRUE(resultAll.count(TransceiverID(1)));
}

TEST_F(PortManagerTest, syncPorts) {
  initManagers(2, 4); // 2 transceivers, 4 ports each

  // Create a map of PortStatus objects
  auto ports = std::make_unique<std::map<int32_t, PortStatus>>();

  // Create PortStatus for port 1 with transceiver info
  PortStatus port1Status;
  port1Status.enabled() = true;
  port1Status.up() = true;
  port1Status.speedMbps() = 100000;
  port1Status.profileID() = "PROFILE_100G_2_PAM4_RS544X2N_OPTICAL";
  port1Status.drained() = false;

  TransceiverIdxThrift tcvrIdx1;
  tcvrIdx1.transceiverId() = 0;
  port1Status.transceiverIdx() = tcvrIdx1;

  // Create PortStatus for port 5 with different transceiver info
  PortStatus port5Status;
  port5Status.enabled() = true;
  port5Status.up() = false;
  port5Status.speedMbps() = 100000;
  port5Status.profileID() = "PROFILE_100G_2_PAM4_RS544X2N_OPTICAL";
  port5Status.drained() = false;

  TransceiverIdxThrift tcvrIdx2;
  tcvrIdx2.transceiverId() = 1;
  port5Status.transceiverIdx() = tcvrIdx2;

  // Create PortStatus for port without transceiver info (should be skipped)
  PortStatus port9Status;
  port9Status.enabled() = false;
  port9Status.up() = false;
  port9Status.speedMbps() = 0;
  port9Status.profileID() = "";
  port9Status.drained() = false;
  // No transceiverIdx set - this should be skipped in the loop

  (*ports)[1] = port1Status;
  (*ports)[5] = port5Status;
  (*ports)[9] = port9Status;

  // Call syncPorts
  std::map<int32_t, TransceiverInfo> info;
  EXPECT_NO_THROW(portManager_->syncPorts(info, std::move(ports)));

  // The info map should contain entries for transceivers that have valid info
  // The exact content depends on what the mock TransceiverManager returns
  // but we can at least verify the method doesn't crash and processes the input
}

TEST_F(PortManagerTest, getPortStates) {
  initManagers(2, 4); // 2 transceivers, 4 ports each

  // Create a vector of port IDs - mix of valid and invalid
  auto portIds = std::make_unique<std::vector<int32_t>>();
  portIds->push_back(1); // Valid port
  portIds->push_back(3); // Valid port
  portIds->push_back(999); // Invalid port - should be skipped

  // Create states map to be populated
  std::map<int32_t, PortStateMachineState> states;

  // Call getPortStates
  EXPECT_NO_THROW(portManager_->getPortStates(states, std::move(portIds)));

  // Verify that valid ports are in the map
  EXPECT_TRUE(states.find(1) != states.end());
  EXPECT_TRUE(states.find(3) != states.end());

  // Verify that invalid port is NOT in the map (it was caught and skipped)
  EXPECT_TRUE(states.find(999) == states.end());

  // Verify we got the expected number of entries (only valid ports)
  EXPECT_EQ(states.size(), 2);
}

} // namespace facebook::fboss
