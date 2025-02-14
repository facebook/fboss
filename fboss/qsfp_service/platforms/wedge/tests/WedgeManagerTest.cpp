/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/module/tests/MockTransceiverImpl.h"
#include "fboss/qsfp_service/test/TransceiverManagerTestHelper.h"

#include <gmock/gmock.h>
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
    EXPECT_EQ(info.second.tcvrState()->present(), true);
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
        *info.second.tcvrState()->timeCollected(),
        *cachedTransInfo[info.first].tcvrState()->timeCollected());
    EXPECT_GT(
        *info.second.tcvrStats()->timeCollected(),
        *cachedTransInfo[info.first].tcvrStats()->timeCollected());
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
    EXPECT_EQ(
        *transInfo[i].tcvrState()->present(),
        i != 4); // ID 5 was marked as absent
    std::string expectedTcvrName = fmt::format("eth1/{}", (i + 1));
    EXPECT_EQ(*transInfo[i].tcvrState()->tcvrName(), expectedTcvrName);
    EXPECT_EQ(*transInfo[i].tcvrStats()->tcvrName(), expectedTcvrName);
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
        *info.second.tcvrState()->timeCollected(),
        *cachedTransInfo[info.first].tcvrState()->timeCollected());
    EXPECT_EQ(
        *info.second.tcvrStats()->timeCollected(),
        *cachedTransInfo[info.first].tcvrStats()->timeCollected());
    EXPECT_EQ(*info.second.tcvrState()->present(), true);
  }

  // Cause read exceptions while reading the management interface. In this case,
  // the qsfp_service should handle the exception gracefully. Since the
  // management interface is not identified correctly so it will mark it not
  // present but have the timestamp info there in transceiverInfo
  transceiverManager_->setReadException(
      true, false); // Read exception only while reading mgmt interface
  transceiverManager_->refreshStateMachines();
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  transInfo.clear();
  transceiverManager_->getTransceiversInfo(
      transInfo, std::make_unique<std::vector<int32_t>>());
  for (const auto& info : transInfo) {
    EXPECT_EQ(*info.second.tcvrState()->present(), false);
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

TEST_F(WedgeManagerTest, writeTransceiverMultiByte) {
  std::map<int32_t, WriteResponse> response;
  std::unique_ptr<WriteRequest> request(new WriteRequest);
  TransceiverIOParameters param;
  std::vector<int32_t> data = {1, 3, 7};
  request->ids() = data;
  param.offset() = 0x10;
  request->parameter() = param;
  request->bytes() = {0, 16, 41};

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
      // id is 0 based here
      EXPECT_EQ(
          transceiverManager_->getCurrentState(TransceiverID(trans.first)),
          trans.first == uint16_t(0)
              ? TransceiverStateMachineState::NOT_PRESENT
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
    if (module.first == uint16_t(0)) {
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

TEST_F(WedgeManagerTest, hardResetUnknownMgmtInterfaceTransceiverDuringInit) {
  transceiverManager_->setReadException(
      true /* throwReadExceptionForMgmtInterface */,
      false /* throwReadExceptionForDomQuery */);
  auto xcvrImpl = static_cast<MockTransceiverPlatformApi*>(
      transceiverManager_->getQsfpPlatformApi());
  // Because we didn't cache the software port yet, areAllPortsDown() will
  // return false to avoid hard resetting a transceiver when we don't know
  // whether the ports are up or not.
  EXPECT_CALL(*xcvrImpl, triggerQsfpHardReset(1)).Times(0);
  transceiverManager_->refreshStateMachines();
}

TEST_F(
    WedgeManagerTest,
    hardResetUnknownMgmtInterfaceTransceiverAfterProgrammed) {
  auto xcvrID = TransceiverID(0);
  // Set override iphy info so that we can pass PROGRAM_IPHY event
  TransceiverManager::OverrideTcvrToPortAndProfile oneTcvrTo4X25G = {
      {xcvrID,
       {
           {PortID(1), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL},
           {PortID(2), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL},
           {PortID(3), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL},
           {PortID(4), cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL},
       }}};
  auto verify = [this, &xcvrID, &oneTcvrTo4X25G](bool isPortUp) {
    transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
        oneTcvrTo4X25G);
    // First state machine refresh to trigger PROGRAM_IPHY
    transceiverManager_->refreshStateMachines();

    // Making mgmt read throw will make readyTransceiver return true so it
    // can transition to transceiver_ready on refresh()
    transceiverManager_->setReadException(
        true /* throwReadExceptionForMgmtInterface */,
        false /* throwReadExceptionForDomQuery */);
    // Second state machine refresh to trigger PREPARE_TRANSCEIVER
    transceiverManager_->refreshStateMachines();

    // Third state machine refresh to trigger PROGRAM_TRANSCEIVER
    transceiverManager_->refreshStateMachines();
    // Override port status
    transceiverManager_->setOverrideAgentPortStatusForTesting(
        isPortUp /* up */, true /* enabled */, false /* clearOnly */);
    // Fourth state machine refresh to trigger PORT_UP or ALL_PORTS_DOWN
    transceiverManager_->refreshStateMachines();
    auto expectedState = isPortUp ? TransceiverStateMachineState::ACTIVE
                                  : TransceiverStateMachineState::INACTIVE;
    auto curState = transceiverManager_->getCurrentState(xcvrID);
    EXPECT_EQ(curState, expectedState)
        << "Transceiver=0 state doesn't match state expected="
        << apache::thrift::util::enumNameSafe(expectedState)
        << ", actual=" << apache::thrift::util::enumNameSafe(curState);

    // Now set module read exception to mimic the case that we can't read the
    // management interface type for the module
    transceiverManager_->setReadException(
        true /* throwReadExceptionForMgmtInterface */,
        false /* throwReadExceptionForDomQuery */);
    auto xcvrImpl = static_cast<MockTransceiverPlatformApi*>(
        transceiverManager_->getQsfpPlatformApi());
    // Because areAllPortsDown() will return false due to the previous port
    // programming and status updating, we should expect there won't be any hard
    // resetting on this transceiver.
    EXPECT_CALL(*xcvrImpl, triggerQsfpHardReset(1)).Times(!isPortUp);
    transceiverManager_->refreshStateMachines();
    curState = transceiverManager_->getCurrentState(xcvrID);
    // If some port is up, the state machine will stay ACTIVE; otherwise, the
    // WedgeManager::updateTransceiverMap() will remove the old QsfpModule,
    // and the refreshStateMachines() will trigger PROGRAM_IPHY again
    expectedState = isPortUp ? TransceiverStateMachineState::ACTIVE
                             : TransceiverStateMachineState::INACTIVE;
    EXPECT_EQ(curState, expectedState)
        << "Transceiver=0 state doesn't match state expected="
        << apache::thrift::util::enumNameSafe(expectedState)
        << ", actual=" << apache::thrift::util::enumNameSafe(curState);
    transceiverManager_->setReadException(
        false /* throwReadExceptionForMgmtInterface */,
        false /* throwReadExceptionForDomQuery */);
  };

  // First verify there WILL NOT be a hard reset if such transceiver is
  // programmed and some ports are up
  verify(true /* isPortUp */);
  // Delete the existing wedge manager and create a new one
  resetTransceiverManager();
  transceiverManager_->init();
  XLOG(INFO) << "[DEBUG] Now verifying when all ports are down";
  // Then verify there WILL be a hard reset if such transceiver is programmed
  // and all ports are down
  verify(false /* isPortUp */);
}

/*
 * This test sets the module level and the global level pause remediation timer
 * and then verifieis these timer values
 */
TEST_F(WedgeManagerTest, pauseRemediationSetGetTest) {
  auto portNameMap = transceiverManager_->getPortNameToModuleMap();

  // List of transceiver names
  std::vector<std::string> portNames{"eth1/1/1", "eth1/2/1"};
  for (auto port : portNames) {
    XLOG(INFO) << "Performing pause remediation set and get test on ports "
               << port << " module id " << portNameMap.at(port);
  }

  // Set the global pause remediation time for 5 seconds
  // And also set module pause remediation time as 10 seconds
  transceiverManager_->setPauseRemediation(5, nullptr);
  transceiverManager_->setPauseRemediation(
      10, std::make_unique<std::vector<std::string>>(portNames));

  // Get the global pause remediation time and verify
  std::map<std::string, int32_t> info;
  transceiverManager_->getPauseRemediationUntil(info, nullptr);
  auto now = std::time(nullptr);
  EXPECT_LE(now, info["all"]);
  EXPECT_LE(info["all"], now + 5);

  // Get the module level pause remediation time and verify
  info.clear();
  transceiverManager_->getPauseRemediationUntil(
      info, std::make_unique<std::vector<std::string>>(portNames));
  for (auto& portName : portNames) {
    EXPECT_LE(now, info[portName]);
    EXPECT_LE(info[portName], now + 10);
  }
}

/*
 * This test sets the pause remediate timers for 2 set of ports. It attempt
 * the remediation on these ports and verifies that they adhere to the pause
 * timers
 */
TEST_F(WedgeManagerTest, pauseRemediationTimerTest) {
  auto portNameMap = transceiverManager_->getPortNameToModuleMap();

  // List of transceiver names
  std::vector<std::string> portNames1{"eth1/1/1", "eth1/2/1"};
  std::vector<std::string> portNames2{"eth1/3/1", "eth1/4/1"};

  // Set module pause remediation time as 125 seconds and 130 seconds for two
  // set of ports respectively
  transceiverManager_->setPauseRemediation(
      125, std::make_unique<std::vector<std::string>>(portNames1));
  transceiverManager_->setPauseRemediation(
      130, std::make_unique<std::vector<std::string>>(portNames2));

  // This Lambda attempts the remediation on the module and check if there was
  // a rediation performed on the module
  auto attemptAndCheckRemediate = [this](TransceiverID xcvr) {
    transceiverManager_->tryRemediateTransceiver(xcvr);

    // A successful remediation will increment module's remediationCount_,
    // this needs to be updated in cache
    transceiverManager_->refreshStateMachines();

    std::map<int32_t, TransceiverInfo> transInfo;
    std::vector<int32_t> xcvrList{xcvr};
    transceiverManager_->getTransceiversInfo(
        transInfo, std::make_unique<std::vector<int32_t>>(xcvrList));
    if (transInfo.find(xcvr) != transInfo.end()) {
      auto remediateCount =
          transInfo[xcvr].tcvrStats()->remediationCounter().value();
      XLOG(INFO) << "Module " << xcvr
                 << " remediate count = " << remediateCount;
      if (remediateCount > 0) {
        return true;
      }
    }
    return false;
  };

  // For first 120 seconds, the remediation is disallowed so wait till that
  // much time
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(120));

  // Check that remediation could not be done on all 4 modules
  EXPECT_FALSE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/1/1"))));
  EXPECT_FALSE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/2/1"))));
  EXPECT_FALSE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/3/1"))));
  EXPECT_FALSE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/4/1"))));

  // Wait for 6 seconds and try again. This time the remediation should be
  // performed on first set of module but not on second set of modules
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(6));
  EXPECT_TRUE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/1/1"))));
  EXPECT_TRUE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/2/1"))));
  EXPECT_FALSE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/3/1"))));
  EXPECT_FALSE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/4/1"))));

  // Wait for 5 seconds and try again. This time the remediation should be
  // allowed on second set of modules also
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::seconds(5));
  EXPECT_TRUE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/3/1"))));
  EXPECT_TRUE(
      attemptAndCheckRemediate(TransceiverID(portNameMap.at("eth1/4/1"))));
}

// This test suite focuses solely on validation logic.
TEST_F(WedgeManagerTest, validateTransceiverConfigTest) {
  auto makeTcvrInfo = [](int id,
                         std::string vendorName,
                         std::string vendorPartNumber,
                         std::string firmwareVersion,
                         std::string dspFirmwareVersion,
                         std::vector<cfg::PortProfileID> portProfileIds,
                         bool validEepromChecksum,
                         std::pair<bool, std::string> requiredFields) {
    TransceiverValidationInfo tcvrInfo;
    tcvrInfo.id = id;
    tcvrInfo.vendorName = vendorName;
    tcvrInfo.vendorPartNumber = vendorPartNumber;
    tcvrInfo.firmwareVersion = firmwareVersion;
    tcvrInfo.dspFirmwareVersion = dspFirmwareVersion;
    tcvrInfo.portProfileIds = portProfileIds;
    tcvrInfo.validEepromChecksums = validEepromChecksum;
    tcvrInfo.requiredFields = requiredFields;
    return tcvrInfo;
  };
  auto verifyTransceiverValidationAndReturnValue =
      [&](TransceiverValidationInfo& tcvrInfo,
          bool isValid,
          std::string notValidatedReason) {
        std::string reason;
        EXPECT_EQ(
            transceiverManager_->validateTransceiverConfiguration(
                tcvrInfo, reason),
            isValid);
        EXPECT_EQ(reason, notValidatedReason);
      };

  TransceiverValidationInfo tcvrInfo;

  // Non-Validated Vendor, PartNumber, Firmware, PortProfileID
  tcvrInfo = makeTcvrInfo(
      0,
      "fakeVendor",
      "0",
      "0",
      "0",
      {cfg::PortProfileID::PROFILE_DEFAULT},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(
      tcvrInfo, false, "nonValidatedVendorName");

  // Validated Vendor, Firmware, PortProfileID / Non-Validated Part Number
  tcvrInfo = makeTcvrInfo(
      0,
      "FBOSSONE",
      "TR-FC13H-HF",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(
      tcvrInfo, false, "nonValidatedVendorPartNumber");

  // Validated Vendor, Firmware, PortProfileID
  tcvrInfo = makeTcvrInfo(
      0,
      "FBOSSONE",
      "LUX426C4AD",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(tcvrInfo, true, "");

  // Validated Vendor, Firmware, PortProfileID / Non-Validated Firmware
  tcvrInfo = makeTcvrInfo(
      0,
      "FBOSSTWO",
      "TR-FC13H-HFZ",
      "0",
      "0",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(tcvrInfo, true, "");

  // Validated Vendor, Firmware / Non-Validated PortProfileID
  tcvrInfo = makeTcvrInfo(
      0,
      "FBOSSONE",
      "SPTSMP3CLCK10",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(
      tcvrInfo, false, "nonValidatedPortProfileId");

  // Validated Vendor, Firmware, Multiple PortProfileIDs
  tcvrInfo = makeTcvrInfo(
      0,
      "FBOSSONE",
      "SPTSMP3CLCK10",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC,
       cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(tcvrInfo, true, "");

  // Validated Configuration / Validated and Not-Validated PortProfileIDs
  tcvrInfo = makeTcvrInfo(
      0,
      "FBOSSONE",
      "SPTSMP3CLCK10",
      "1",
      "2",
      {
          cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC,
          cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
          cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
          cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N,
      },
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(
      tcvrInfo, false, "nonValidatedPortProfileId");

  // Validated Configuration / Multiple Non-Validated PortProfileIDs
  tcvrInfo = makeTcvrInfo(
      0,
      "FBOSSONE",
      "SPTSMP3CLCK10",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
       cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(
      tcvrInfo, false, "nonValidatedPortProfileId");

  // Validated Vendor (lowercase), Firmware, PortProfileID
  tcvrInfo = makeTcvrInfo(
      0,
      "fbossONE",
      "LUX426C4AD",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
      true,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(tcvrInfo, true, "");

  // Validated Configuration / Non-Validated Eeprom Checksums
  tcvrInfo = makeTcvrInfo(
      0,
      "fbossONE",
      "LUX426C4AD",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
      false,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(
      tcvrInfo, false, "invalidEepromChecksums");

  // Non-Validated Configuration, Eeprom Checksums
  tcvrInfo = makeTcvrInfo(
      0,
      "fbossTwo",
      "LUX426C4AD",
      "1",
      "2",
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
      false,
      std::make_pair(true, ""));
  verifyTransceiverValidationAndReturnValue(
      tcvrInfo, false, "invalidEepromChecksums");

  // Missing Information
  tcvrInfo = makeTcvrInfo(
      0, "", "", "", "", {}, true, std::make_pair(false, "missingVendor"));
  verifyTransceiverValidationAndReturnValue(tcvrInfo, false, "missingVendor");
}

// This suite focuses on information retrieval and validation logic.
TEST_F(WedgeManagerTest, validateTransceiverConfigByIdTest) {
  // Initialize Transceiver and Return Values
  auto tcvrOneID = TransceiverID(1);
  auto tcvrTwoID = TransceiverID(2);
  auto tcvrThreeID = TransceiverID(3);
  auto tcvrFourID = TransceiverID(4);

  auto transceiverImpl =
      std::make_unique<::testing::NiceMock<MockTransceiverImpl>>();
  auto transImpl_ = transceiverImpl.get();
  EXPECT_CALL(*transImpl_, detectTransceiver())
      .WillRepeatedly(::testing::Return(true));
  qsfpImpls_.push_back(std::move(transceiverImpl));
  std::string defaultSerialNumber = "123456";
  MediaInterfaceCode defaultMediaInterfaceCode = MediaInterfaceCode::FR4_200G;
  std::string defaultMediaInterfaceCodeString = "FR4_200G";
  auto makeDefaultTcvr = [&](TransceiverID tcvrID) {
    auto tcvr = static_cast<MockSffModule*>(
        transceiverManager_->overrideTransceiverForTesting(
            tcvrID,
            std::make_unique<MockSffModule>(
                transceiverManager_->getPortNames(tcvrID),
                qsfpImpls_.back().get(),
                tcvrConfig_,
                transceiverManager_->getTransceiverName(tcvrID))));
    tcvr->detectPresence();
    tcvr->overrideVendorInfo("fbossTwo", "TR-FC13H-HFZ", defaultSerialNumber);
    tcvr->setFwVersion("1", "2");
    tcvr->overrideValidChecksums(true);
    tcvr->overrideMediaInterfaceCode(defaultMediaInterfaceCode);
    return tcvr;
  };

  // Initialize Transceiver Default Settings (validated / nonValidated /
  // missing)
  TransceiverManager::OverrideTcvrToPortAndProfile portProfiles = {
      {tcvrOneID,
       {
           {PortID(1), cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
           {PortID(2), cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
       }},
      {tcvrTwoID,
       {
           {PortID(1), cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER},
           {PortID(2), cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER},
           {PortID(4), cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC},
       }}};
  transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(portProfiles);

  // Validation Checker Helper Function (checks validity and config string)
  auto validateBasic = [&](MockSffModule* tcvr,
                           TransceiverID tcvrID,
                           bool isValid,
                           bool validatePortProfile,
                           std::string configString) {
    std::string reason;
    transceiverManager_->refreshStateMachines();
    tcvr->useActualGetTransceiverInfo();
    EXPECT_EQ(
        transceiverManager_->validateTransceiverById(
            tcvrID, reason, validatePortProfile),
        isValid);
    if (validatePortProfile) {
      EXPECT_EQ(
          transceiverManager_->getTransceiverValidationConfigString(tcvrID),
          configString);
    }
  };
  auto validateWithReason = [&](MockSffModule* tcvr,
                                TransceiverID tcvrID,
                                bool isValid,
                                std::string configString) {
    validateBasic(tcvr, tcvrID, isValid, true, configString);
  };
  // Verifies the QSFP Test version of validation – where portProfileIds are not
  // used in validation.
  auto validateQsfpTestWithReason = [&](MockSffModule* tcvr,
                                        TransceiverID tcvrID,
                                        bool isValid,
                                        std::string configString) {
    validateBasic(tcvr, tcvrID, isValid, false, configString);
  };
  auto configString = [&](TransceiverID tcvrID,
                          std::string vendorName,
                          std::string vendorSerialNumber,
                          std::string vendorPartNumber,
                          std::string mediaInterfaceCode,
                          std::string appFwVer,
                          std::string dspFwVer,
                          std::vector<std::string> portProfileIds,
                          std::string nonValidatedAttribute) {
    return folly::sformat(
        "{{\n  \"Non-Validated Attribute\": \"{}\",\n  \"Transceiver Application Firmware Version\": \"{}\""
        ",\n  \"Transceiver DSP Firmware Version\": \"{}\",\n  \"Transceiver ID\": {},\n  \"Transceiver Media Interface Code\": \"{}\",\n  "
        "\"Transceiver Part Number\": \"{}\",\n  \"Transceiver Port Profile Ids\": \"{}\",\n  \"Transceiver Serial Number\": \"{}\",\n  "
        "\"Transceiver Vendor\": \"{}\"\n}}",
        nonValidatedAttribute,
        appFwVer,
        dspFwVer,
        static_cast<int>(tcvrID),
        mediaInterfaceCode,
        vendorPartNumber,
        folly::join(",", portProfileIds),
        vendorSerialNumber,
        vendorName);
  };

  auto tcvrOne = makeDefaultTcvr(tcvrOneID);
  auto tcvrTwo = makeDefaultTcvr(tcvrTwoID);
  auto tcvrThree = makeDefaultTcvr(tcvrThreeID);
  auto tcvrFour = makeDefaultTcvr(tcvrFourID);

  // Test Validated Config
  validateWithReason(tcvrOne, tcvrOneID, true, "");
  // Test Validated Config w/o Validating PortProfiles
  validateQsfpTestWithReason(tcvrOne, tcvrOneID, true, "");

  // Test Missing Vendor
  tcvrOne->overrideVendorInfo("", "", "");
  validateWithReason(
      tcvrOne,
      tcvrOneID,
      false,
      configString(tcvrOneID, "", "", "", "", "", "", {}, "missingVendor"));

  // Test Missing Vendor Part Number
  tcvrOne->overrideVendorInfo("fbossOne", "", defaultSerialNumber);
  validateWithReason(
      tcvrOne,
      tcvrOneID,
      false,
      configString(
          tcvrOneID,
          "fbossOne",
          defaultSerialNumber,
          "",
          "",
          "",
          "",
          {},
          "missingVendorPartNumber"));

  // Test Missing Firmware (should yield a Validated result)
  tcvrOne->overrideVendorInfo("fbossTwo", "TR-FC13H-HFZ", defaultSerialNumber);
  tcvrOne->setFwVersion("", "");
  validateWithReason(tcvrOne, tcvrOneID, true, "");

  // Test Missing PortProfileIDs
  validateWithReason(
      tcvrThree,
      tcvrThreeID,
      false,
      configString(
          tcvrThreeID,
          "fbossTwo",
          defaultSerialNumber,
          "TR-FC13H-HFZ",
          defaultMediaInterfaceCodeString,
          "1",
          "2",
          {},
          "missingPortProfileIds"));
  // Test Missing PortProfileIDs w/o Validating PortProfiles
  validateQsfpTestWithReason(tcvrThree, tcvrThreeID, true, "");

  // Test Non-Validated Config –– Incorrect Vendor
  tcvrOne->overrideVendorInfo(
      "fbossThree", "TR-FC13H-HFZ", defaultSerialNumber);
  tcvrOne->setFwVersion("1", "2");
  validateWithReason(
      tcvrOne,
      tcvrOneID,
      false,
      configString(
          tcvrOneID,
          "fbossThree",
          defaultSerialNumber,
          "TR-FC13H-HFZ",
          defaultMediaInterfaceCodeString,
          "1",
          "2",
          {"PROFILE_100G_4_NRZ_NOFEC", "PROFILE_100G_4_NRZ_NOFEC"},
          "nonValidatedVendorName"));

  // Test Non-Validated Config –– Incorrect Vendor Part Number
  tcvrOne->overrideVendorInfo("fbossTwo", "TR-FC13H-HF", defaultSerialNumber);
  validateWithReason(
      tcvrOne,
      tcvrOneID,
      false,
      configString(
          tcvrOneID,
          "fbossTwo",
          defaultSerialNumber,
          "TR-FC13H-HF",
          defaultMediaInterfaceCodeString,
          "1",
          "2",
          {"PROFILE_100G_4_NRZ_NOFEC", "PROFILE_100G_4_NRZ_NOFEC"},
          "nonValidatedVendorPartNumber"));

  // Test Non-Validated Config –– Incorrect Port Profile ID
  validateWithReason(
      tcvrTwo,
      tcvrTwoID,
      false,
      configString(
          tcvrTwoID,
          "fbossTwo",
          defaultSerialNumber,
          "TR-FC13H-HFZ",
          defaultMediaInterfaceCodeString,
          "1",
          "2",
          {"PROFILE_100G_4_NRZ_NOFEC",
           "PROFILE_100G_4_NRZ_CL91_COPPER",
           "PROFILE_25G_1_NRZ_CL74_COPPER"},
          "nonValidatedPortProfileId"));

  // Test Non-Validated Eeprom Checksums
  tcvrFour->overrideValidChecksums(false);
  validateWithReason(
      tcvrFour,
      tcvrFourID,
      false,
      configString(
          tcvrFourID,
          "fbossTwo",
          defaultSerialNumber,
          "TR-FC13H-HFZ",
          defaultMediaInterfaceCodeString,
          "1",
          "2",
          {},
          "invalidEepromChecksums"));
}

// These test cases validate the logic used to calculate Transceiver Validator's
// exported fb303 counter.
TEST_F(WedgeManagerTest, testGetNumNonValidatedTransceiverConfigs) {
  int numTransceivers = 5;
  auto genTcvrInfoMap = [&](std::unordered_set<int> upTransceivers) {
    std::map<int32_t, TransceiverInfo> infoMap;
    for (int i = 0; i < numTransceivers; i++) {
      TransceiverInfo info;
      info.tcvrState()->present() =
          upTransceivers.find(i) != upTransceivers.end();

      infoMap[TransceiverID(i)] = std::move(info);
    }

    return infoMap;
  };
  auto setNonValidatedCache = [&](std::unordered_set<int> nonValidTcvrIds) {
    for (int i = 0; i < numTransceivers; i++) {
      transceiverManager_->updateValidationCache(
          TransceiverID(i), nonValidTcvrIds.find(i) == nonValidTcvrIds.end());
    }
  };
  auto expectNumNonValidatedConfigs =
      [&](std::map<int32_t, TransceiverInfo>& infoMap, int expectedVal) {
        EXPECT_EQ(
            transceiverManager_->getNumNonValidatedTransceiverConfigs(infoMap),
            expectedVal);
      };

  std::map<int32_t, TransceiverInfo> infoMap;
  // Empty nonValidTransceiversCache
  setNonValidatedCache({});

  infoMap = genTcvrInfoMap({});
  expectNumNonValidatedConfigs(infoMap, 0);

  infoMap = genTcvrInfoMap({2, 3});
  expectNumNonValidatedConfigs(infoMap, 0);

  infoMap = genTcvrInfoMap({0, 1, 2, 3, 4});
  expectNumNonValidatedConfigs(infoMap, 0);

  // Full nonValidTransceiversCache
  setNonValidatedCache({0, 1, 2, 3, 4});

  infoMap = genTcvrInfoMap({});
  expectNumNonValidatedConfigs(infoMap, 0);

  infoMap = genTcvrInfoMap({2, 3});
  expectNumNonValidatedConfigs(infoMap, 2);

  infoMap = genTcvrInfoMap({0, 1, 2, 3, 4});
  expectNumNonValidatedConfigs(infoMap, 5);

  // Partially full nonValidTransceiversCache
  setNonValidatedCache({2, 3});

  infoMap = genTcvrInfoMap({});
  expectNumNonValidatedConfigs(infoMap, 0);

  infoMap = genTcvrInfoMap({0, 1});
  expectNumNonValidatedConfigs(infoMap, 0);

  infoMap = genTcvrInfoMap({2, 3, 4});
  expectNumNonValidatedConfigs(infoMap, 2);

  infoMap = genTcvrInfoMap({0, 1, 2, 3, 4});
  expectNumNonValidatedConfigs(infoMap, 2);
}

} // namespace facebook::fboss
