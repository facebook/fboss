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

} // namespace facebook::fboss
