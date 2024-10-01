/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

namespace facebook::fboss {

class HwStateMachineTest : public HwTest {
 public:
  HwStateMachineTest(bool setupOverrideTcvrToPortAndProfile = true)
      : HwTest(setupOverrideTcvrToPortAndProfile) {}

  void SetUp() override {
    HwTest::SetUp();
    if (!IsSkipped()) {
      std::map<int32_t, TransceiverInfo> presentTcvrs;
      getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
          presentTcvrs, std::make_unique<std::vector<int32_t>>());

      auto cabledTransceivers = utility::legacyTransceiverIds(
          utility::getCabledPortTranceivers(getHwQsfpEnsemble()));

      // Get all transceivers from platform mapping
      const auto& chips = getHwQsfpEnsemble()->getPlatformMapping()->getChips();
      for (const auto& chip : chips) {
        if (*chip.second.type() != phy::DataPlanePhyChipType::TRANSCEIVER) {
          continue;
        }
        auto id = *chip.second.physicalID();
        if (auto tcvrIt = presentTcvrs.find(id); tcvrIt != presentTcvrs.end() &&
            *tcvrIt->second.tcvrState()->present()) {
          if (std::find(
                  cabledTransceivers.begin(), cabledTransceivers.end(), id) !=
              cabledTransceivers.end()) {
            presentTransceivers_.push_back(TransceiverID(id));
          }
        } else {
          absentTransceivers_.push_back(TransceiverID(id));
        }
      }
      XLOG(DBG2) << "Transceivers num: [present:" << presentTransceivers_.size()
                 << ", absent:" << absentTransceivers_.size() << "]";

      // Set pause remdiation so it won't trigger remediation
      setPauseRemediation(true);
    }
  }

  const std::vector<TransceiverID>& getPresentTransceivers() const {
    return presentTransceivers_;
  }
  const std::vector<TransceiverID>& getAbsentTransceivers() const {
    return absentTransceivers_;
  }
  void setPauseRemediation(bool paused) {
    getHwQsfpEnsemble()->getWedgeManager()->setPauseRemediation(
        paused ? 600 : 0, nullptr);
  }

  bool meetAllExpectedState(
      TransceiverID id,
      TransceiverStateMachineState curState,
      std::queue<TransceiverStateMachineState>& tcvrExpectedStates,
      bool isRemediated,
      bool isAgentColdboot) {
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    // Check whether current state matches the head of the expected state
    // queue
    if (tcvrExpectedStates.empty()) {
      // Already meet all expected states.
      return true;
    } else if (curState == tcvrExpectedStates.front()) {
      tcvrExpectedStates.pop();
      const auto programmedPortToPortInfo =
          wedgeMgr->getProgrammedIphyPortToPortInfo(id);
      std::vector<PortID> xphyPorts;
      if (auto phyManager = wedgeMgr->getPhyManager()) {
        xphyPorts = phyManager->getXphyPorts();
      }

      if (curState == TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED) {
        EXPECT_EQ(wedgeMgr->getNeedResetDataPath(id), isAgentColdboot);
      } else if (
          curState == TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED) {
        if (isRemediated) {
          // TODO(joseph5wu) Add check to ensure the module remediation did
          // happen
          const auto& transceiver = wedgeMgr->getTransceiverInfo(id);
          auto mgmtInterface = apache::thrift::can_throw(
              *transceiver.tcvrState()->transceiverManagementInterface());
          if (mgmtInterface == TransceiverManagementInterface::CMIS) {
            // CMIS will hard reset the module in
            // remediateFlakyTransceiver() Without clear hard reset, we
            // won't be able to detect such transceiver
          } else if (mgmtInterface == TransceiverManagementInterface::SFF) {
            // SFF will trigger tx enable and then reset low power mode.
          }
        }
        // Check xphy programmed correctly
        const auto& transceiver = wedgeMgr->getTransceiverInfo(id);
        for (const auto& [portID, portInfo] : programmedPortToPortInfo) {
          if (std::find(xphyPorts.begin(), xphyPorts.end(), portID) !=
              xphyPorts.end()) {
            utility::verifyXphyPort(
                portID, portInfo.profile, transceiver, getHwQsfpEnsemble());
          }
        }
        EXPECT_EQ(wedgeMgr->getNeedResetDataPath(id), isAgentColdboot);
      } else if (
          curState == TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED) {
        // Just finished transceiver programming
        // Only care enabled ports
        if (!programmedPortToPortInfo.empty()) {
          for (const auto& [portID, portInfo] : programmedPortToPortInfo) {
            const auto tcvrInfo = wedgeMgr->getTransceiverInfo(id);
            auto portName = wedgeMgr->getPortNameByPortId(portID);
            CHECK(portName.has_value());
            utility::HwTransceiverUtils::verifyTransceiverSettings(
                *tcvrInfo.tcvrState(), *portName, portInfo.profile);
            utility::HwTransceiverUtils::verifyDiagsCapability(
                *tcvrInfo.tcvrState(), wedgeMgr->getDiagsCapability(id));
          }
        }
        // After transceiver is programmed, needResetDataPath should be false
        EXPECT_FALSE(wedgeMgr->getNeedResetDataPath(id));
      }
      return tcvrExpectedStates.empty();
    }
    XLOG(WARN) << "Transceiver:" << id << " doesn't have expected state="
               << apache::thrift::util::enumNameSafe(tcvrExpectedStates.front())
               << " but actual state="
               << apache::thrift::util::enumNameSafe(curState);
    return false;
  }

  bool refreshStateMachinesTillMeetAllStates(
      std::unordered_map<
          TransceiverID,
          std::queue<TransceiverStateMachineState>>& expectedStates,
      bool isRemediated,
      bool isAgentColdboot) {
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    wedgeMgr->refreshStateMachines();
    int numFailedTransceivers = 0;
    for (auto& idToExpectStates : expectedStates) {
      auto id = idToExpectStates.first;
      auto curState = wedgeMgr->getCurrentState(id);
      if (!meetAllExpectedState(
              id,
              curState,
              idToExpectStates.second,
              isRemediated,
              isAgentColdboot)) {
        ++numFailedTransceivers;
      }
    }
    XLOG_IF(WARN, numFailedTransceivers)
        << numFailedTransceivers
        << " transceivers don't meet the expected state";
    return numFailedTransceivers == 0;
  }

 private:
  // Forbidden copy constructor and assignment operator
  HwStateMachineTest(HwStateMachineTest const&) = delete;
  HwStateMachineTest& operator=(HwStateMachineTest const&) = delete;

  std::vector<TransceiverID> presentTransceivers_;
  std::vector<TransceiverID> absentTransceivers_;
};

// With HwTest::waitTillCabledTcvrProgrammed() being called in HwTest::SetUp(),
// all transceivers' state machine will reach programmed state eventually.
// But we also want to verify the early state if qsfp_service can't call
// iphy programming from wedge_agent.
class HwStateMachineTestWithoutIphyProgramming : public HwStateMachineTest {
 public:
  HwStateMachineTestWithoutIphyProgramming() : HwStateMachineTest(false) {}

 private:
  // Forbidden copy constructor and assignment operator
  HwStateMachineTestWithoutIphyProgramming(
      HwStateMachineTestWithoutIphyProgramming const&) = delete;
  HwStateMachineTestWithoutIphyProgramming& operator=(
      HwStateMachineTestWithoutIphyProgramming const&) = delete;
};

TEST_F(HwStateMachineTestWithoutIphyProgramming, CheckOpticsDetection) {
  auto verify = [&]() {
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    // Default HwTest::Setup() already has a refresh, so all present
    // transceivers should be in DISCOVERED state; while
    // not present front panel ports should still be NOT_PRESENT
    for (auto id : getPresentTransceivers()) {
      auto curState = wedgeMgr->getCurrentState(id);
      EXPECT_EQ(curState, TransceiverStateMachineState::DISCOVERED)
          << "Transceiver:" << id
          << " Actual: " << apache::thrift::util::enumNameSafe(curState)
          << ", Expected: DISCOVERED";
    }
    for (auto id : getAbsentTransceivers()) {
      auto curState = wedgeMgr->getCurrentState(id);
      EXPECT_EQ(curState, TransceiverStateMachineState::NOT_PRESENT)
          << "Transceiver:" << id
          << " Actual: " << apache::thrift::util::enumNameSafe(curState)
          << ", Expected: NOT_PRESENT";
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(HwStateMachineTest, CheckPortsProgrammed) {
  auto verify = [this]() {
    auto checkTransceiverProgrammed = [this](const std::vector<TransceiverID>&
                                                 tcvrs) {
      auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
      std::vector<PortID> xphyPorts;
      if (auto phyManager = wedgeMgr->getPhyManager()) {
        xphyPorts = phyManager->getXphyPorts();
      }

      for (auto id : tcvrs) {
        // Verify IPHY/ XPHY/ TCVR programmed as expected
        const auto& portAndProfile =
            wedgeMgr->getOverrideProgrammedIphyPortAndProfileForTest(id);
        // Check programmed iphy port and profile
        const auto programmedPortToPortInfo =
            wedgeMgr->getProgrammedIphyPortToPortInfo(id);
        const auto& transceiver = wedgeMgr->getTransceiverInfo(id);
        if (portAndProfile.empty()) {
          // If iphy port and profile is empty, it means the ports are
          // disabled. We don't need to program such transceivers
          EXPECT_TRUE(programmedPortToPortInfo.empty());
        } else {
          EXPECT_EQ(programmedPortToPortInfo.size(), portAndProfile.size());
          for (const auto& [portID, portInfo] : programmedPortToPortInfo) {
            auto expectedPortAndProfileIt = portAndProfile.find(portID);
            EXPECT_TRUE(expectedPortAndProfileIt != portAndProfile.end());
            EXPECT_EQ(portInfo.profile, expectedPortAndProfileIt->second);
            if (std::find(xphyPorts.begin(), xphyPorts.end(), portID) !=
                xphyPorts.end()) {
              utility::verifyXphyPort(
                  portID, portInfo.profile, transceiver, getHwQsfpEnsemble());
            }
          }
          if (!programmedPortToPortInfo.empty()) {
            for (const auto& [portID, portInfo] : programmedPortToPortInfo) {
              auto portName = wedgeMgr->getPortNameByPortId(portID);
              CHECK(portName.has_value());
              utility::HwTransceiverUtils::verifyTransceiverSettings(
                  *transceiver.tcvrState(), *portName, portInfo.profile);
              utility::HwTransceiverUtils::verifyDiagsCapability(
                  *transceiver.tcvrState(), wedgeMgr->getDiagsCapability(id));
            }
          }
        }
      }
    };

    checkTransceiverProgrammed(getPresentTransceivers());
    checkTransceiverProgrammed(getAbsentTransceivers());
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(HwStateMachineTest, CheckPortStatusUpdated) {
  auto verify = [this]() {
    std::unordered_map<TransceiverID, time_t> lastDownTimes;
    auto checkTransceiverActiveState =
        [this, &lastDownTimes](
            bool up, TransceiverStateMachineState expectedState) {
          auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
          wedgeMgr->setOverrideAgentPortStatusForTesting(
              up, true /* enabled */);
          wedgeMgr->refreshStateMachines();
          for (auto id : getPresentTransceivers()) {
            auto curState = wedgeMgr->getCurrentState(id);
            if (wedgeMgr->getProgrammedIphyPortToPortInfo(id).empty()) {
              // If iphy port and profile is empty, it means the ports are
              // disabled. We treat such port to be always INACTIVE
              EXPECT_EQ(curState, TransceiverStateMachineState::INACTIVE)
                  << "Transceiver:" << id
                  << " doesn't have expected state=INACTIVE but actual state="
                  << apache::thrift::util::enumNameSafe(curState);
            } else {
              EXPECT_EQ(curState, expectedState)
                  << "Transceiver:" << id << " doesn't have expected state="
                  << apache::thrift::util::enumNameSafe(expectedState)
                  << " but actual state="
                  << apache::thrift::util::enumNameSafe(curState);
              // Make sure if lastDownTimes has such transceiver, we compare the
              // new lastDownTime for such Transceiver with the value in the map
              // to make sure we'll get new lastDownTime
              if (curState == TransceiverStateMachineState::INACTIVE) {
                auto newDownTime = wedgeMgr->getLastDownTime(id);
                if (lastDownTimes.find(id) != lastDownTimes.end()) {
                  EXPECT_GT(newDownTime, lastDownTimes[id]);
                }
                lastDownTimes[id] = newDownTime;
              }
            }
          }
        };
    // First set all ports up
    checkTransceiverActiveState(true, TransceiverStateMachineState::ACTIVE);
    // Then set all ports down
    checkTransceiverActiveState(false, TransceiverStateMachineState::INACTIVE);
    // Set all ports up again
    checkTransceiverActiveState(true, TransceiverStateMachineState::ACTIVE);
    // Finally set all ports down to see whether lastDownTime got updated
    checkTransceiverActiveState(false, TransceiverStateMachineState::INACTIVE);
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(HwStateMachineTest, CheckTransceiverRemoved) {
  auto verify = [this]() {
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    wedgeMgr->setOverrideAgentPortStatusForTesting(
        false /* up */, true /* enabled */);
    wedgeMgr->refreshStateMachines();
    // Reset all present transceivers
    for (auto tcvrID : getPresentTransceivers()) {
      wedgeMgr->triggerQsfpHardReset(tcvrID);
      auto curState = wedgeMgr->getCurrentState(tcvrID);
      EXPECT_EQ(curState, TransceiverStateMachineState::NOT_PRESENT)
          << "Transceiver:" << tcvrID
          << " doesn't have expected state=NOT_PRESENT but actual state="
          << apache::thrift::util::enumNameSafe(curState);
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(HwStateMachineTest, CheckTransceiverRemediated) {
  auto verify = [this]() {
    std::set<TransceiverID> enabledTcvrs;
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    wedgeMgr->setOverrideAgentPortStatusForTesting(
        true /* up */, true /* enabled */);
    // Remove pause remediation
    setPauseRemediation(false);
    wedgeMgr->refreshStateMachines();
    for (auto id : getPresentTransceivers()) {
      auto curState = wedgeMgr->getCurrentState(id);
      bool isEnabled = !wedgeMgr->getProgrammedIphyPortToPortInfo(id).empty();
      auto expectedState = isEnabled ? TransceiverStateMachineState::ACTIVE
                                     : TransceiverStateMachineState::INACTIVE;
      if (isEnabled) {
        enabledTcvrs.insert(id);
      }
      EXPECT_EQ(curState, expectedState)
          << "Transceiver:" << id << " doesn't have expected state="
          << apache::thrift::util::enumNameSafe(expectedState)
          << " but actual state="
          << apache::thrift::util::enumNameSafe(curState);
    }

    // Now set all ports down to trigger remediation
    wedgeMgr->setOverrideAgentPortStatusForTesting(
        false /* up */, true /* enabled */);
    // Make sure all enabled transceiver should go through:
    // XPHY_PORTS_PROGRAMMED -> TRANSCEIVER_READY -> TRANSCEIVER_PROGRAMMED
    std::unordered_map<TransceiverID, std::queue<TransceiverStateMachineState>>
        expectedStates;
    for (auto id : getPresentTransceivers()) {
      std::queue<TransceiverStateMachineState> tcvrExpectedStates;
      // Only care enabled ports
      if (enabledTcvrs.find(id) != enabledTcvrs.end()) {
        if (!wedgeMgr->supportRemediateTransceiver(id)) {
          continue;
        }
        tcvrExpectedStates.push(
            TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED);
        tcvrExpectedStates.push(
            TransceiverStateMachineState::TRANSCEIVER_READY);
        tcvrExpectedStates.push(
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
      }
      expectedStates.emplace(id, std::move(tcvrExpectedStates));
    }

    // Due to some platforms are easy to have i2c issue which causes the current
    // refresh not work as expected. Adding enough retries to make sure that we
    // at least can meet all `expectedStates` after 10 times.
    WITH_RETRIES_N_TIMED(
        10 /* retries */,
        std::chrono::milliseconds(10000) /* msBetweenRetry */,
        EXPECT_EVENTUALLY_TRUE(refreshStateMachinesTillMeetAllStates(
            expectedStates,
            true /* isRemediated */,
            false /* isAgentColdboot */)));
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(HwStateMachineTest, CheckAgentConfigChanged) {
  auto verify = [this]() {
    auto verifyConfigChanged = [this](bool isAgentColdboot) {
      std::time_t testStartTime = std::time(nullptr);
      auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
      // Prepare expected states
      std::
          unordered_map<TransceiverID, std::queue<TransceiverStateMachineState>>
              expectedStates;
      bool hasXphy = (wedgeMgr->getPhyManager() != nullptr);
      // All tcvrs should go through no matter present or absent
      // IPHY_PORTS_PROGRAMMED -> XPHY_PORTS_PROGRAMMED ->
      // -> TRANSCEIVER_READY -> TRANSCEIVER_PROGRAMMED
      auto prepareExpectedStates = [hasXphy]() {
        std::queue<TransceiverStateMachineState> tcvrExpectedStates;
        tcvrExpectedStates.push(
            TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED);
        if (hasXphy) {
          tcvrExpectedStates.push(
              TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED);
        }
        tcvrExpectedStates.push(
            TransceiverStateMachineState::TRANSCEIVER_READY);
        tcvrExpectedStates.push(
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
        return tcvrExpectedStates;
      };

      for (auto id : getPresentTransceivers()) {
        expectedStates.emplace(id, prepareExpectedStates());
      }
      for (auto id : getAbsentTransceivers()) {
        expectedStates.emplace(id, prepareExpectedStates());
      }

      // Override ConfigAppliedInfo
      ConfigAppliedInfo configAppliedInfo;
      auto currentInMs = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch());
      configAppliedInfo.lastAppliedInMs() = currentInMs.count();
      if (isAgentColdboot) {
        configAppliedInfo.lastColdbootAppliedInMs() = currentInMs.count();
      }
      wedgeMgr->setOverrideAgentConfigAppliedInfoForTesting(configAppliedInfo);

      // Due to some platforms are easy to have i2c issue which causes the
      // current refresh not work as expected. Adding enough retries to make
      // sure that we at least can meet all `expectedStates` after 10 times.
      WITH_RETRIES_N_TIMED(
          10 /* retries */,
          std::chrono::milliseconds(10000) /* msBetweenRetry */,
          EXPECT_EVENTUALLY_TRUE(refreshStateMachinesTillMeetAllStates(
              expectedStates, false /* isRemediated */, isAgentColdboot)));

      // Verify datapath reset happened on a config change that involved agent
      // cold boot and didn't happen for warmboot
      for (auto id : getPresentTransceivers()) {
        auto tcvrInfo = wedgeMgr->getTransceiverInfo(id);
        auto& tcvrState = tcvrInfo.tcvrState().value();
        auto& tcvrStats = tcvrInfo.tcvrStats().value();
        for (auto& [portName, _] : tcvrStats.get_portNameToHostLanes()) {
          XLOG(INFO) << "Verify datapath reset timestamp for port " << portName;
          utility::HwTransceiverUtils::verifyDatapathResetTimestamp(
              portName, tcvrState, tcvrStats, testStartTime, isAgentColdboot);
        }
      }
    };

    // First verify warmboot config change
    verifyConfigChanged(false /* isAgentColdboot */);
    // And then coldboot
    verifyConfigChanged(true /* isAgentColdboot */);
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
