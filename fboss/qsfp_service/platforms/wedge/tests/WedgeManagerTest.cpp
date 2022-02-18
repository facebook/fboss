/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/tests/MockTransceiverImpl.h"
#include "fboss/qsfp_service/test/FakeConfigsHelper.h"

#include <folly/Memory.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/experimental/TestUtil.h>

using namespace facebook::fboss;
using namespace ::testing;
namespace {

class WedgeManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    setupFakeAgentConfig(agentCfgPath);
    setupFakeQsfpConfig(qsfpCfgPath);

    // Set up fake qsfp_service volatile directory
    gflags::SetCommandLineOptionWithMode(
        "qsfp_service_volatile_dir",
        qsfpSvcVolatileDir.c_str(),
        gflags::SET_FLAGS_DEFAULT);

    // Create a wedge manager for 16 modules and 4 ports per module
    wedgeManager_ = std::make_unique<NiceMock<MockWedgeManager>>(16, 4);
    wedgeManager_->initTransceiverMap();
    gflags::SetCommandLineOptionWithMode(
        "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);
  }

  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
  std::string qsfpSvcVolatileDir = tmpDir.path().string();
  std::string agentCfgPath = qsfpSvcVolatileDir + "/fakeAgentConfig";
  std::string qsfpCfgPath = qsfpSvcVolatileDir + "/fakeQsfpConfig";
  std::string warmBootFlagFile = qsfpSvcVolatileDir + "/can_warm_boot";
  std::string coldBootFileName =
      qsfpSvcVolatileDir + "/cold_boot_once_qsfp_service";
  std::unique_ptr<NiceMock<MockWedgeManager>> wedgeManager_;
};

TEST_F(WedgeManagerTest, getTransceiverInfoBasic) {
  // If no ids are passed in, info for all should be returned
  std::map<int32_t, TransceiverInfo> transInfo;
  wedgeManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  {
    auto synchronizedTransceivers =
        wedgeManager_->getSynchronizedTransceivers().rlock();
    EXPECT_EQ((*synchronizedTransceivers).size(), transInfo.size());
  }
  for (const auto& info : transInfo) {
    EXPECT_EQ(info.second.present_ref(), true);
  }
  auto cachedTransInfo = transInfo;

  // Refresh transceivers again and make sure the collected time increments
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  wedgeManager_->refreshTransceivers();
  transInfo.clear();
  wedgeManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());

  for (const auto& info : transInfo) {
    EXPECT_GT(
        *info.second.timeCollected_ref(),
        *cachedTransInfo[info.first].timeCollected_ref());
  }

  // Otherwise, just return the ids requested
  std::vector<int32_t> data = {1, 3, 7};
  transInfo.clear();
  wedgeManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>(data));
  {
    auto synchronizedTransceivers =
        wedgeManager_->getSynchronizedTransceivers().rlock();
    for (const auto& trans : *synchronizedTransceivers) {
      if (std::find(data.begin(), data.end(), (int)trans.first) == data.end()) {
        EXPECT_EQ(transInfo.find(trans.first), transInfo.end());
      } else {
        EXPECT_NE(transInfo.find(trans.first), transInfo.end());
      }
    }
  }

  // Remove a transceiver and make sure that port's transceiverInfo says
  // transceiver is absent
  wedgeManager_->overridePresence(5, false);
  wedgeManager_->refreshTransceivers();
  transInfo.clear();
  wedgeManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  for (auto i = 0; i < wedgeManager_->getNumQsfpModules(); i++) {
    EXPECT_EQ(*transInfo[i].present_ref(), i != 4); // ID 5 was marked as absent
  }
}

TEST_F(WedgeManagerTest, getTransceiverInfoWithReadExceptions) {
  // Cause read exceptions while refreshing transceivers and confirm that
  // transceiverInfo still has the old data (this is verified by comparing
  // timestamps)
  std::map<int32_t, TransceiverInfo> transInfo;
  wedgeManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  auto cachedTransInfo = transInfo;
  wedgeManager_->setReadException(
      false, true); // Read exception only while doing DOM reads
  wedgeManager_->refreshTransceivers();
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  transInfo.clear();

  wedgeManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  for (const auto& info : transInfo) {
    EXPECT_EQ(
        *info.second.timeCollected_ref(),
        *cachedTransInfo[info.first].timeCollected_ref());
    EXPECT_EQ(*info.second.present_ref(), true);
  }
  wedgeManager_->setReadException(false, false);

  // Cause read exceptions while reading the management interface. In this case,
  // the qsfp_service should handle the exception gracefully and still mark
  // presence for the transceivers but not adding any other data like the
  // timeCollected timestamp to the transceiverInfo
  wedgeManager_->setReadException(
      true, false); // Read exception only while reading mgmt interface
  wedgeManager_->refreshTransceivers();
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  transInfo.clear();
  wedgeManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  for (const auto& info : transInfo) {
    EXPECT_EQ(*info.second.present_ref(), true);
    EXPECT_EQ(info.second.timeCollected_ref().has_value(), false);
  }
}

TEST_F(WedgeManagerTest, readTransceiver) {
  std::map<int32_t, ReadResponse> response;
  std::unique_ptr<ReadRequest> request(new ReadRequest);
  TransceiverIOParameters param;
  std::vector<int32_t> data = {1, 3, 7};
  request->ids_ref() = data;
  param.offset_ref() = 0x10;
  param.length_ref() = 10;
  request->parameter_ref() = param;

  wedgeManager_->readTransceiverRegister(response, std::move(request));
  for (const auto& i : data) {
    EXPECT_NE(response.find(i), response.end());
  }
}

TEST_F(WedgeManagerTest, writeTransceiver) {
  std::map<int32_t, WriteResponse> response;
  std::unique_ptr<WriteRequest> request(new WriteRequest);
  TransceiverIOParameters param;
  std::vector<int32_t> data = {1, 3, 7};
  request->ids_ref() = data;
  param.offset_ref() = 0x10;
  request->parameter_ref() = param;
  request->data_ref() = 0xab;

  wedgeManager_->writeTransceiverRegister(response, std::move(request));
  for (const auto& i : data) {
    EXPECT_NE(response.find(i), response.end());
  }
}

TEST_F(WedgeManagerTest, modulePresenceTest) {
  // Tests that the module insertion is handled smoothly
  auto currentModules = wedgeManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    EXPECT_EQ(module.second, TransceiverManagementInterface::SFF);
  }
  EXPECT_EQ(
      wedgeManager_->scanTransceiverPresence(
          std::make_unique<std::vector<int32_t>>()),
      16);
  {
    auto synchronizedTransceivers =
        wedgeManager_->getSynchronizedTransceivers().rlock();
    for (const auto& trans : *synchronizedTransceivers) {
      QsfpModule* qsfp = dynamic_cast<QsfpModule*>(trans.second.get());
      // Expected state is MODULE_STATE_DISCOVERED
      EXPECT_EQ(qsfp->getLegacyModuleStateMachineCurrentState(), 2);
    }
  }
}

TEST_F(WedgeManagerTest, moduleNotPresentTest) {
  // Tests that the module removal is handled smoothly
  wedgeManager_->overridePresence(1, false);
  wedgeManager_->refreshTransceivers();
  auto currentModules = wedgeManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 15);
  EXPECT_EQ(
      wedgeManager_->scanTransceiverPresence(
          std::make_unique<std::vector<int32_t>>()),
      15);
  {
    auto synchronizedTransceivers =
        wedgeManager_->getSynchronizedTransceivers().rlock();
    for (const auto& trans : *synchronizedTransceivers) {
      QsfpModule* qsfp = dynamic_cast<QsfpModule*>(trans.second.get());
      if (trans.first == 0) { // id is 0 based here
        // Expected state is MODULE_STATE_NOT_PRESENT
        EXPECT_EQ(qsfp->getLegacyModuleStateMachineCurrentState(), 0);
      } else {
        // Expected state is MODULE_STATE_DISCOVERED
        EXPECT_EQ(qsfp->getLegacyModuleStateMachineCurrentState(), 2);
      }
    }
  }
}

TEST_F(WedgeManagerTest, mgmtInterfaceChangedTest) {
  // Simulate the case where a SFF module is swapped with a CMIS module
  wedgeManager_->overridePresence(1, false);
  wedgeManager_->refreshTransceivers();
  wedgeManager_->overrideMgmtInterface(
      1, uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS));
  wedgeManager_->overridePresence(1, true);
  wedgeManager_->refreshTransceivers();
  auto currentModules = wedgeManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    if (module.first == 0) {
      EXPECT_EQ(module.second, TransceiverManagementInterface::CMIS);
    } else {
      EXPECT_EQ(module.second, TransceiverManagementInterface::SFF);
    }
  }
}

TEST_F(WedgeManagerTest, miniphotonMgmtInterfaceDetectionTest) {
  wedgeManager_->overrideMgmtInterface(
      1, uint8_t(TransceiverModuleIdentifier::MINIPHOTON_OBO));
  wedgeManager_->overridePresence(1, true);
  wedgeManager_->refreshTransceivers();
  auto currentModules = wedgeManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    EXPECT_EQ(module.second, TransceiverManagementInterface::SFF);
  }
}

// We default to SFF for unknown IDs
TEST_F(WedgeManagerTest, unknownMgmtInterfaceDetectionTest) {
  // some unused mgmt interface (not in TransceiverModuleIdentifier)
  wedgeManager_->overrideMgmtInterface(1, 0xFF);
  wedgeManager_->overridePresence(1, true);
  wedgeManager_->refreshTransceivers();
  auto currentModules = wedgeManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    EXPECT_EQ(module.second, TransceiverManagementInterface::SFF);
  }
}

TEST_F(WedgeManagerTest, syncPorts) {
  // Sync the port status for a few ports and confirm that the returned
  // response is expected
  std::map<int32_t, TransceiverInfo> info;
  std::map<int32_t, std::vector<int32_t>> transceiverAndPortsToSync = {
      {1, std::vector{1, 2}},
      {2, std::vector{5}},
      {6, std::vector{9, 10, 11, 12}}};
  std::map<int32_t, PortStatus> portMap = {};

  for (const auto& sync : transceiverAndPortsToSync) {
    PortStatus dummyPortStatus;
    TransceiverIdxThrift idx;
    idx.transceiverId_ref() = sync.first;
    dummyPortStatus.transceiverIdx_ref() = idx;
    for (const auto& port : sync.second) {
      portMap[port] = dummyPortStatus;
    }
  }

  std::unique_ptr<std::map<int32_t, PortStatus>> ports =
      std::make_unique<std::map<int32_t, PortStatus>>(portMap);
  wedgeManager_->syncPorts(info, std::move(ports));

  EXPECT_EQ(info.size(), transceiverAndPortsToSync.size());
  for (const auto& sync : transceiverAndPortsToSync) {
    EXPECT_NE(info.find(sync.first), info.end());
  }
}

TEST_F(WedgeManagerTest, coldBootTest) {
  auto verifyColdBootLogic = [this]() {
    // Delete the existing wedge manager and create a new one
    wedgeManager_.reset();
    wedgeManager_ = std::make_unique<NiceMock<MockWedgeManager>>(16, 4);
    // Force cold boot is set
    EXPECT_FALSE(wedgeManager_->canWarmBoot());
    // We expect a cold boot in this case and that should trigger hard resets of
    // QSFP modules
    for (int i = 0; i < wedgeManager_->getNumQsfpModules(); i++) {
      EXPECT_CALL(*wedgeManager_, triggerQsfpHardReset(i)).Times(1);
    }
    wedgeManager_->initTransceiverMap();

    // Confirm that the cold boot file and warm boot flag file were deleted
    EXPECT_FALSE(checkFileExists(coldBootFileName));
    EXPECT_FALSE(checkFileExists(warmBootFlagFile));
  };
  auto gracefulExit = [this]() {
    // Trigger a graceful exit
    wedgeManager_->gracefulExit();
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

TEST_F(WedgeManagerTest, warmBootTest) {
  // Trigger a graceful exit
  wedgeManager_->gracefulExit();
  // Check warm boot flag file is created
  EXPECT_TRUE(checkFileExists(warmBootFlagFile));

  wedgeManager_.reset();
  wedgeManager_ = std::make_unique<NiceMock<MockWedgeManager>>(16, 4);
  // Confirm that the warm boot falg was still there
  EXPECT_TRUE(checkFileExists(warmBootFlagFile));
  EXPECT_TRUE(wedgeManager_->canWarmBoot());

  // We expect a warm boot in this case and that should NOT trigger hard resets
  // of QSFP modules
  for (int i = 0; i < wedgeManager_->getNumQsfpModules(); i++) {
    EXPECT_CALL(*wedgeManager_, triggerQsfpHardReset(i)).Times(0);
  }
  wedgeManager_->initTransceiverMap();
}

TEST_F(WedgeManagerTest, getAndClearTransceiversSignalFlagsTest) {
  std::map<int32_t, SignalFlags> signalFlags;

  // When no ports are passed, signalFlags should have info about all 16 modules
  wedgeManager_->getAndClearTransceiversSignalFlags(
      signalFlags, std::make_unique<std::vector<int32_t>>());
  EXPECT_EQ(signalFlags.size(), 16);

  signalFlags.clear();
  std::vector<int32_t> ports{1, 5, 9};
  wedgeManager_->getAndClearTransceiversSignalFlags(
      signalFlags, std::make_unique<std::vector<int32_t>>(ports));
  EXPECT_EQ(signalFlags.size(), 3);
}

TEST_F(WedgeManagerTest, getAndClearTransceiversMediaSignalsTest) {
  std::map<int32_t, std::map<int, MediaLaneSignals>> mediaSignalsMap;

  // When no ports are passed, signalFlags should have info about all 16 modules
  wedgeManager_->getAndClearTransceiversMediaSignals(
      mediaSignalsMap, std::make_unique<std::vector<int32_t>>());
  EXPECT_EQ(mediaSignalsMap.size(), 16);

  mediaSignalsMap.clear();
  std::vector<int32_t> ports{1, 5, 9};
  wedgeManager_->getAndClearTransceiversMediaSignals(
      mediaSignalsMap, std::make_unique<std::vector<int32_t>>(ports));
  EXPECT_EQ(mediaSignalsMap.size(), 3);
}

TEST_F(WedgeManagerTest, sfpMgmtInterfaceDetectionTest) {
  for (int id = 1; id <= 16; id++) {
    wedgeManager_->overrideMgmtInterface(
        id, uint8_t(TransceiverModuleIdentifier::SFP_PLUS));
    wedgeManager_->overridePresence(id, true);
  }
  wedgeManager_->refreshTransceivers();
  auto currentModules = wedgeManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    EXPECT_EQ(module.second, TransceiverManagementInterface::SFF8472);
  }
}
} // namespace
