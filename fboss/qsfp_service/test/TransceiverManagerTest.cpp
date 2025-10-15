// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/test/TransceiverManagerTestHelper.h"

#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss {

class TransceiverManagerTest : public TransceiverManagerTestHelper {
 public:
  std::string warmBootFlagFile = qsfpSvcVolatileDir + "/can_warm_boot";
  std::string coldBootFileName =
      qsfpSvcVolatileDir + "/cold_boot_once_qsfp_service";
};

TEST_F(TransceiverManagerTest, getPortNameToModuleMap) {
  EXPECT_EQ(transceiverManager_->getPortNameToModuleMap().at("eth1/1/1"), 0);
  EXPECT_EQ(transceiverManager_->getPortNameToModuleMap().at("eth1/1/2"), 0);
  EXPECT_EQ(transceiverManager_->getPortNameToModuleMap().at("eth1/2/1"), 1);
  EXPECT_THROW(
      transceiverManager_->getPortNameToModuleMap().at("no_such_port"),
      std::out_of_range);
}

TEST_F(TransceiverManagerTest, coldBootTest) {
  auto verifyColdBootLogic = [this]() {
    // Delete the existing wedge manager and create a new one
    resetTransceiverManager();
    // Force cold boot is set
    EXPECT_FALSE(transceiverManager_->canWarmBoot());
    // We expect a cold boot in this case and that should trigger hard resets of
    // QSFP modules
    MockTransceiverPlatformApi* xcvrApi =
        static_cast<MockTransceiverPlatformApi*>(
            transceiverManager_->getQsfpPlatformApi());
    for (int i = 0; i < transceiverManager_->getNumQsfpModules(); i++) {
      EXPECT_CALL(*xcvrApi, triggerQsfpHardReset(i + 1)).Times(1);
    }
    transceiverManager_->init();

    // Confirm that the cold boot file and warm boot flag file were deleted
    EXPECT_FALSE(checkFileExists(coldBootFileName));
    EXPECT_FALSE(checkFileExists(warmBootFlagFile));
  };
  auto gracefulExit = [this]() {
    // Trigger a graceful exit
    transceiverManager_->gracefulExit();
    // Check warm boot flag file is created
    EXPECT_TRUE(checkFileExists(warmBootFlagFile));
  };

  // Create the cold boot file
  auto fd = createFile(coldBootFileName);
  close(fd);
  verifyColdBootLogic();

  // Try cold boot again if last time was using graceful exit
  gracefulExit();
  fd = createFile(coldBootFileName);
  close(fd);
  verifyColdBootLogic();

  // Sepcifically set can_qsfp_service_warm_boot to false to mimic
  // Elbert8DD pim case which doesn't support warm boot
  gracefulExit();
  gflags::SetCommandLineOptionWithMode(
      "can_qsfp_service_warm_boot", "0", gflags::SET_FLAGS_DEFAULT);
  verifyColdBootLogic();
}

TEST_F(TransceiverManagerTest, warmBootTest) {
  // Trigger a graceful exit
  transceiverManager_->gracefulExit();
  gflags::SetCommandLineOptionWithMode(
      "can_qsfp_service_warm_boot", "1", gflags::SET_FLAGS_DEFAULT);
  // Check warm boot flag file is created
  EXPECT_TRUE(checkFileExists(warmBootFlagFile));

  resetTransceiverManager();

  // We expect a warm boot in this case and that should NOT trigger hard resets
  // of QSFP modules
  MockTransceiverPlatformApi* xcvrApi =
      static_cast<MockTransceiverPlatformApi*>(
          transceiverManager_->getQsfpPlatformApi());
  for (int i = 0; i < transceiverManager_->getNumQsfpModules(); i++) {
    EXPECT_CALL(*xcvrApi, triggerQsfpHardReset(i + 1)).Times(0);
  }
  transceiverManager_->init();

  // Confirm that the warm boot falg was still there
  EXPECT_TRUE(checkFileExists(warmBootFlagFile));
  EXPECT_TRUE(transceiverManager_->canWarmBoot());
}

TEST_F(TransceiverManagerTest, runState) {
  resetTransceiverManager();
  EXPECT_FALSE(transceiverManager_->isSystemInitialized());
  EXPECT_FALSE(transceiverManager_->isFullyInitialized());
  EXPECT_FALSE(transceiverManager_->isExiting());
  EXPECT_EQ(
      transceiverManager_->getRunState(), QsfpServiceRunState::UNINITIALIZED);

  transceiverManager_->init();
  EXPECT_TRUE(transceiverManager_->isSystemInitialized());
  EXPECT_FALSE(transceiverManager_->isFullyInitialized());
  EXPECT_FALSE(transceiverManager_->isExiting());
  EXPECT_EQ(
      transceiverManager_->getRunState(), QsfpServiceRunState::INITIALIZED);

  transceiverManager_->refreshStateMachines();
  EXPECT_TRUE(transceiverManager_->isSystemInitialized());
  EXPECT_TRUE(transceiverManager_->isFullyInitialized());
  EXPECT_FALSE(transceiverManager_->isExiting());
  EXPECT_EQ(transceiverManager_->getRunState(), QsfpServiceRunState::ACTIVE);

  transceiverManager_->gracefulExit();
  EXPECT_TRUE(transceiverManager_->isSystemInitialized());
  EXPECT_TRUE(transceiverManager_->isFullyInitialized());
  EXPECT_TRUE(transceiverManager_->isExiting());
  EXPECT_EQ(transceiverManager_->getRunState(), QsfpServiceRunState::EXITING);
}

ACTION(ThrowFbossError) {
  throw FbossError("Mock FbossError");
}
TEST_F(TransceiverManagerTest, getInterfacePhyInfo) {
  std::map<std::string, phy::PhyInfo> phyInfos;
  // Test a valid interface
  transceiverManager_->getInterfacePhyInfo(phyInfos, "eth1/1/1");
  EXPECT_TRUE(phyInfos.find("eth1/1/1") != phyInfos.end());
  // Simulate an exception from xphyInfo and confirm that the map doesn't
  // include that interface's key
  EXPECT_CALL(*transceiverManager_, getXphyInfo(PortID(9)))
      .Times(1)
      .WillRepeatedly(ThrowFbossError());
  transceiverManager_->getInterfacePhyInfo(phyInfos, "eth1/2/1");
  EXPECT_EQ(phyInfos.size(), 1);
  // We still expect the previous interface to exist in the map
  EXPECT_TRUE(phyInfos.find("eth1/1/1") != phyInfos.end());

  // Test an invalid interface
  EXPECT_THROW(
      transceiverManager_->getInterfacePhyInfo(phyInfos, "no_such_interface"),
      FbossError);
}

TEST_F(TransceiverManagerTest, opticalOrActiveCableTest) {
  TcvrState state;
  Cable cable;

  // Set up cable with OPTICAL transmitter tech
  cable.transmitterTech() = TransmitterTechnology::OPTICAL;
  state.cable() = cable;

  // Test with OPTICAL transmitter - should return true
  EXPECT_TRUE(TransceiverManager::opticalOrActiveCable(state));

  // Change to COPPER transmitter - should return false
  state.cable()->transmitterTech() = TransmitterTechnology::COPPER;
  EXPECT_FALSE(TransceiverManager::opticalOrActiveCable(state));
}

TEST_F(TransceiverManagerTest, activeCableTest) {
  TcvrState state;
  Cable cable;

  // Set up cable with ACTIVE_CABLES media type
  cable.mediaTypeEncoding() = MediaTypeEncodings::ACTIVE_CABLES;
  state.cable() = cable;

  // Test with ACTIVE_CABLES media type - should return true
  EXPECT_TRUE(TransceiverManager::activeCable(state));

  // Change to different media type - should return false
  state.cable()->mediaTypeEncoding() = MediaTypeEncodings::OPTICAL_SMF;
  EXPECT_FALSE(TransceiverManager::activeCable(state));
}

TEST_F(TransceiverManagerTest, opticalOrActiveCmisCableTest) {
  TcvrState state;
  Cable cable;

  // Set up cable with OPTICAL transmitter tech
  cable.transmitterTech() = TransmitterTechnology::OPTICAL;
  state.cable() = cable;
  state.transceiverManagementInterface() = TransceiverManagementInterface::CMIS;

  // Test with CMIS interface and optical cable - should return true
  EXPECT_TRUE(TransceiverManager::opticalOrActiveCmisCable(state));

  // Change interface to non-CMIS - should return false
  state.transceiverManagementInterface() = TransceiverManagementInterface::SFF;
  EXPECT_FALSE(TransceiverManager::opticalOrActiveCmisCable(state));
}

TEST_F(TransceiverManagerTest, getTransceiverInfoOptionalNonExistent) {
  // Test with a non-existent transceiver ID
  auto result =
      transceiverManager_->getTransceiverInfoOptional(TransceiverID(999));

  // Verify that the result is std::nullopt
  EXPECT_FALSE(result.has_value());
}

TEST_F(TransceiverManagerTest, resetProgrammedIPhyPortToPortInfoForPorts) {
  // Set up test data using the override mechanism
  TransceiverManager::OverrideTcvrToPortAndProfile testOverride = {
      {TransceiverID(0),
       {{PortID(1), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL},
        {PortID(2), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL}}},
      {TransceiverID(1),
       {{PortID(9), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL},
        {PortID(10), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL}}}};

  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(testOverride);

  // Program internal PHY ports to populate tcvrToPortInfo_
  transceiverManager_->programInternalPhyPorts(TransceiverID(0));
  transceiverManager_->programInternalPhyPorts(TransceiverID(1));

  // Verify initial state - both transceivers should have populated port info
  auto tcvr0PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(0));
  auto tcvr1PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(1));

  EXPECT_EQ(tcvr0PortInfo.size(), 2);
  EXPECT_TRUE(tcvr0PortInfo.count(PortID(1)));
  EXPECT_TRUE(tcvr0PortInfo.count(PortID(2)));

  EXPECT_EQ(tcvr1PortInfo.size(), 2);
  EXPECT_TRUE(tcvr1PortInfo.count(PortID(9)));
  EXPECT_TRUE(tcvr1PortInfo.count(PortID(10)));

  // Test Case 1: Reset specific ports across multiple transceivers
  std::unordered_set<PortID> portsToReset = {PortID(1), PortID(9)};
  transceiverManager_->resetProgrammedIphyPortToPortInfoForPorts(portsToReset);

  // Verify that only specified ports were removed
  tcvr0PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(0));
  tcvr1PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(1));

  // TransceiverID(0) should have only PortID(2), PortID(1) should be removed
  EXPECT_EQ(tcvr0PortInfo.size(), 1);
  EXPECT_FALSE(tcvr0PortInfo.count(PortID(1)));
  EXPECT_TRUE(tcvr0PortInfo.count(PortID(2)));

  // TransceiverID(1) should have only PortID(10), PortID(9) should be removed
  EXPECT_EQ(tcvr1PortInfo.size(), 1);
  EXPECT_FALSE(tcvr1PortInfo.count(PortID(9)));
  EXPECT_TRUE(tcvr1PortInfo.count(PortID(10)));

  // Test Case 2: Reset non-existent ports (should be no-op)
  std::unordered_set<PortID> nonExistentPorts = {PortID(999), PortID(1000)};
  transceiverManager_->resetProgrammedIphyPortToPortInfoForPorts(
      nonExistentPorts);

  // Verify state remains the same
  auto tcvr0PortInfoAfter =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(0));
  auto tcvr1PortInfoAfter =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(1));

  EXPECT_EQ(tcvr0PortInfoAfter.size(), 1);
  EXPECT_TRUE(tcvr0PortInfoAfter.count(PortID(2)));

  EXPECT_EQ(tcvr1PortInfoAfter.size(), 1);
  EXPECT_TRUE(tcvr1PortInfoAfter.count(PortID(10)));

  // Test Case 3: Reset remaining ports
  std::unordered_set<PortID> remainingPorts = {PortID(2), PortID(10)};
  transceiverManager_->resetProgrammedIphyPortToPortInfoForPorts(
      remainingPorts);

  // Verify all specified ports are removed
  tcvr0PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(0));
  tcvr1PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(1));

  EXPECT_EQ(tcvr0PortInfo.size(), 0);
  EXPECT_EQ(tcvr1PortInfo.size(), 0);

  // Test Case 4: Reset empty set (should be no-op)
  std::unordered_set<PortID> emptySet;
  transceiverManager_->resetProgrammedIphyPortToPortInfoForPorts(emptySet);

  // Verify state remains empty
  tcvr0PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(0));
  tcvr1PortInfo =
      transceiverManager_->getProgrammedIphyPortToPortInfo(TransceiverID(1));

  EXPECT_EQ(tcvr0PortInfo.size(), 0);
  EXPECT_EQ(tcvr1PortInfo.size(), 0);

  // Clean up override
  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(std::nullopt);
}

TEST_F(TransceiverManagerTest, programTransceiverWithNullQsfpConfig) {
  // Set up test data using the override mechanism first
  TransceiverManager::OverrideTcvrToPortAndProfile testOverride = {
      {TransceiverID(0),
       {{PortID(1), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL}}}};

  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(testOverride);

  // Program internal PHY ports to populate tcvrToPortInfo_
  transceiverManager_->programInternalPhyPorts(TransceiverID(0));

  // Reset the transceiver manager to create one without a qsfp config
  // This will make qsfpConfig_ NULL, which will test the untested lines in
  // getOpticalChannelConfig
  resetTransceiverManager();

  // Set up the override again since resetTransceiverManager creates a new
  // instance
  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(testOverride);
  transceiverManager_->programInternalPhyPorts(TransceiverID(0));

  // Call programTransceiver which internally calls getOpticalChannelConfig
  // This should handle the NULL qsfpConfig_ case gracefully without crashing
  // The getOpticalChannelConfig method will return std::nullopt when
  // qsfpConfig_ is NULL
  EXPECT_NO_THROW(
      transceiverManager_->programTransceiver(TransceiverID(0), false));

  // Clean up override
  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(std::nullopt);
}

} // namespace facebook::fboss
