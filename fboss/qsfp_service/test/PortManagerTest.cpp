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
    initManagers();
    setupFakeQsfpConfig(qsfpCfgPath);
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
      bool expectedEnabled,
      bool expectedUp) {
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
  std::shared_ptr<TransceiverManager> transceiverManager_;
  std::unique_ptr<MockPortManager> portManager_;
};

TEST_F(PortManagerTest, getLowestIndexedPortForTransceiverPortGroup) {
  // Single Tcvr - Single Port
  initManagers(1, 1);
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      PortID(1));

  // Single Tcvr – Multi Port
  initManagers(1, 4);
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(1)),
      PortID(1));
  ASSERT_EQ(
      portManager_->getLowestIndexedPortForTransceiverPortGroup(PortID(3)),
      PortID(1));

  // Multi Tcvr – Single Port (will be added once we validate other use cases)
}

TEST_F(PortManagerTest, getLowestIndexedTransceiverForPort) {
  // Single Tcvr – Single Port
  initManagers(1, 4);
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
  initManagers(1, 1);
  ASSERT_TRUE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)));

  // Single Tcvr – Multi Port
  initManagers(1, 4);
  ASSERT_TRUE(
      portManager_->isLowestIndexedPortForTransceiverPortGroup(PortID(1)));
  ASSERT_FALSE(
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

  validatePortStatusInTestingOverride(PortID(1), false, true);
  validatePortStatusInTestingOverride(PortID(3), true, false);
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

TEST_F(PortManagerTest, programExternalPhyPort) {
  initManagers(1, 4, true /* setPhyManager */);
  programInternalPhyPortsForTest();

  EXPECT_CALL(
      *phyManager_,
      programOnePort(
          PortID(1),
          cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
          ::testing::_,
          false))
      .Times(1);

  EXPECT_NO_THROW(portManager_->programExternalPhyPort(PortID(1), false));
  // We don't throw if the port isn't an XPHY port.
  EXPECT_NO_THROW(portManager_->programExternalPhyPort(PortID(1000), false));
}

TEST_F(PortManagerTest, programExternalPhyPortNoPhyManager) {
  // Test the early return when phyManager_ is null
  initManagers(1, 4, false /* setPhyManager */);

  // Should return early without throwing when phyManager_ is null
  EXPECT_NO_THROW(portManager_->programExternalPhyPort(PortID(1), false));
}

TEST_F(PortManagerTest, initAndExit) {
  initManagers();
  transceiverManager_->init();
  portManager_->init();

  portManager_->gracefulExit();
}

} // namespace facebook::fboss
