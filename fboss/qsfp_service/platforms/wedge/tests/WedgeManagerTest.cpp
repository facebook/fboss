/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/test/TransceiverManagerTestHelper.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

class WedgeManagerTest : public TransceiverManagerTestHelper {};

TEST_F(WedgeManagerTest, getTransceiverInfoBasic) {
  // If no ids are passed in, info for all should be returned
  std::map<int32_t, TransceiverInfo> transInfo;
  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  {
    auto synchronizedTransceivers =
        transceiverManager_->getSynchronizedTransceivers().rlock();
    EXPECT_EQ((*synchronizedTransceivers).size(), transInfo.size());
  }
  for (const auto& info : transInfo) {
    EXPECT_EQ(info.second.present(), true);
  }
  auto cachedTransInfo = transInfo;

  // Refresh transceivers again and make sure the collected time increments
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  transceiverManager_->refreshStateMachines();
  transInfo.clear();
  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());

  for (const auto& info : transInfo) {
    EXPECT_GT(
        *info.second.timeCollected(),
        *cachedTransInfo[info.first].timeCollected());
  }

  // Otherwise, just return the ids requested
  std::vector<int32_t> data = {1, 3, 7};
  transInfo.clear();
  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>(data));
  {
    auto synchronizedTransceivers =
        transceiverManager_->getSynchronizedTransceivers().rlock();
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
  transceiverManager_->overridePresence(5, false);
  transceiverManager_->refreshStateMachines();
  transInfo.clear();
  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  for (auto i = 0; i < transceiverManager_->getNumQsfpModules(); i++) {
    EXPECT_EQ(*transInfo[i].present_ref(), i != 4); // ID 5 was marked as absent
  }
}

TEST_F(WedgeManagerTest, getTransceiverInfoWithReadExceptions) {
  // Cause read exceptions while refreshing transceivers and confirm that
  // transceiverInfo still has the old data (this is verified by comparing
  // timestamps)
  std::map<int32_t, TransceiverInfo> transInfo;
  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  auto cachedTransInfo = transInfo;
  transceiverManager_->setReadException(
      false, true); // Read exception only while doing DOM reads
  transceiverManager_->refreshStateMachines();
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  transInfo.clear();

  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  for (const auto& info : transInfo) {
    EXPECT_EQ(
        *info.second.timeCollected(),
        *cachedTransInfo[info.first].timeCollected());
    EXPECT_EQ(*info.second.present(), true);
  }
  transceiverManager_->setReadException(false, false);

  // Cause read exceptions while reading the management interface. In this case,
  // the qsfp_service should handle the exception gracefully and still mark
  // presence for the transceivers but not adding any other data like the
  // timeCollected timestamp to the transceiverInfo
  transceiverManager_->setReadException(
      true, false); // Read exception only while reading mgmt interface
  transceiverManager_->refreshStateMachines();
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  transInfo.clear();
  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  for (const auto& info : transInfo) {
    EXPECT_EQ(*info.second.present(), true);
    EXPECT_EQ(info.second.timeCollected().has_value(), false);
  }
}

TEST_F(WedgeManagerTest, readTransceiver) {
  std::map<int32_t, ReadResponse> response;
  std::unique_ptr<ReadRequest> request(new ReadRequest);
  TransceiverIOParameters param;
  std::vector<int32_t> data = {1, 3, 7};
  request->ids() = data;
  param.offset() = 0x10;
  param.length() = 10;
  request->parameter() = param;

  transceiverManager_->readTransceiverRegister(response, std::move(request));
  for (const auto& i : data) {
    EXPECT_NE(response.find(i), response.end());
  }
}

TEST_F(WedgeManagerTest, writeTransceiver) {
  std::map<int32_t, WriteResponse> response;
  std::unique_ptr<WriteRequest> request(new WriteRequest);
  TransceiverIOParameters param;
  std::vector<int32_t> data = {1, 3, 7};
  request->ids() = data;
  param.offset() = 0x10;
  request->parameter() = param;
  request->data() = 0xab;

  transceiverManager_->writeTransceiverRegister(response, std::move(request));
  for (const auto& i : data) {
    EXPECT_NE(response.find(i), response.end());
  }
}

TEST_F(WedgeManagerTest, modulePresenceTest) {
  // Tests that the module insertion is handled smoothly
  auto currentModules = transceiverManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    EXPECT_EQ(module.second, TransceiverManagementInterface::SFF);
  }
  EXPECT_EQ(
      transceiverManager_->scanTransceiverPresence(
          std::make_unique<std::vector<int32_t>>()),
      16);
  {
    auto synchronizedTransceivers =
        transceiverManager_->getSynchronizedTransceivers().rlock();
    for (const auto& trans : *synchronizedTransceivers) {
      EXPECT_EQ(
          transceiverManager_->getCurrentState(TransceiverID(trans.first)),
          TransceiverStateMachineState::DISCOVERED);
    }
  }
}

TEST_F(WedgeManagerTest, moduleNotPresentTest) {
  // Tests that the module removal is handled smoothly
  transceiverManager_->overridePresence(1, false);
  transceiverManager_->refreshStateMachines();
  auto currentModules = transceiverManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 15);
  EXPECT_EQ(
      transceiverManager_->scanTransceiverPresence(
          std::make_unique<std::vector<int32_t>>()),
      15);
  {
    auto synchronizedTransceivers =
        transceiverManager_->getSynchronizedTransceivers().rlock();
    for (const auto& trans : *synchronizedTransceivers) {
      QsfpModule* qsfp = dynamic_cast<QsfpModule*>(trans.second.get());
      // id is 0 based here
      EXPECT_EQ(
          transceiverManager_->getCurrentState(TransceiverID(trans.first)),
          trans.first == 0 ? TransceiverStateMachineState::NOT_PRESENT
                           : TransceiverStateMachineState::DISCOVERED);
    }
  }
}

TEST_F(WedgeManagerTest, mgmtInterfaceChangedTest) {
  // Simulate the case where a SFF module is swapped with a CMIS module
  transceiverManager_->overridePresence(1, false);
  transceiverManager_->refreshStateMachines();
  transceiverManager_->overrideMgmtInterface(
      1, uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS));
  transceiverManager_->overridePresence(1, true);
  transceiverManager_->refreshStateMachines();
  auto currentModules = transceiverManager_->mgmtInterfaces();
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
  transceiverManager_->overrideMgmtInterface(
      1, uint8_t(TransceiverModuleIdentifier::MINIPHOTON_OBO));
  transceiverManager_->overridePresence(1, true);
  transceiverManager_->refreshStateMachines();
  auto currentModules = transceiverManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    EXPECT_EQ(module.second, TransceiverManagementInterface::SFF);
  }
}

// We default to SFF for unknown IDs
TEST_F(WedgeManagerTest, unknownMgmtInterfaceDetectionTest) {
  // some unused mgmt interface (not in TransceiverModuleIdentifier)
  transceiverManager_->overrideMgmtInterface(1, 0xFF);
  transceiverManager_->overridePresence(1, true);
  transceiverManager_->refreshStateMachines();
  auto currentModules = transceiverManager_->mgmtInterfaces();
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
    idx.transceiverId() = sync.first;
    dummyPortStatus.transceiverIdx() = idx;
    for (const auto& port : sync.second) {
      portMap[port] = dummyPortStatus;
    }
  }

  std::unique_ptr<std::map<int32_t, PortStatus>> ports =
      std::make_unique<std::map<int32_t, PortStatus>>(portMap);
  transceiverManager_->syncPorts(info, std::move(ports));

  EXPECT_EQ(info.size(), transceiverAndPortsToSync.size());
  for (const auto& sync : transceiverAndPortsToSync) {
    EXPECT_NE(info.find(sync.first), info.end());
  }
}

TEST_F(WedgeManagerTest, getAndClearTransceiversSignalFlagsTest) {
  std::map<int32_t, SignalFlags> signalFlags;

  // When no ports are passed, signalFlags should have info about all 16 modules
  transceiverManager_->getAndClearTransceiversSignalFlags(
      signalFlags, std::make_unique<std::vector<int32_t>>());
  EXPECT_EQ(signalFlags.size(), 16);

  signalFlags.clear();
  std::vector<int32_t> ports{1, 5, 9};
  transceiverManager_->getAndClearTransceiversSignalFlags(
      signalFlags, std::make_unique<std::vector<int32_t>>(ports));
  EXPECT_EQ(signalFlags.size(), 3);
}

TEST_F(WedgeManagerTest, getAndClearTransceiversMediaSignalsTest) {
  std::map<int32_t, std::map<int, MediaLaneSignals>> mediaSignalsMap;

  // When no ports are passed, signalFlags should have info about all 16 modules
  transceiverManager_->getAndClearTransceiversMediaSignals(
      mediaSignalsMap, std::make_unique<std::vector<int32_t>>());
  EXPECT_EQ(mediaSignalsMap.size(), 16);

  mediaSignalsMap.clear();
  std::vector<int32_t> ports{1, 5, 9};
  transceiverManager_->getAndClearTransceiversMediaSignals(
      mediaSignalsMap, std::make_unique<std::vector<int32_t>>(ports));
  EXPECT_EQ(mediaSignalsMap.size(), 3);
}

TEST_F(WedgeManagerTest, sfpMgmtInterfaceDetectionTest) {
  for (int id = 1; id <= 16; id++) {
    transceiverManager_->overrideMgmtInterface(
        id, uint8_t(TransceiverModuleIdentifier::SFP_PLUS));
    transceiverManager_->overridePresence(id, true);
  }
  transceiverManager_->refreshStateMachines();
  auto currentModules = transceiverManager_->mgmtInterfaces();
  EXPECT_EQ(currentModules.size(), 16);
  for (auto module : currentModules) {
    EXPECT_EQ(module.second, TransceiverManagementInterface::SFF8472);
  }
}
} // namespace facebook::fboss
