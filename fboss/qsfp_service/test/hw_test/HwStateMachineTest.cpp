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
      : HwTest(true, setupOverrideTcvrToPortAndProfile) {}

  void SetUp() override {
    HwTest::SetUp();

    std::map<int32_t, TransceiverInfo> presentTcvrs;
    getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
        presentTcvrs, std::make_unique<std::vector<int32_t>>());

    // Get all transceivers from platform mapping
    const auto& chips = getHwQsfpEnsemble()->getPlatformMapping()->getChips();
    for (const auto& chip : chips) {
      if (*chip.second.type_ref() != phy::DataPlanePhyChipType::TRANSCEIVER) {
        continue;
      }
      auto id = *chip.second.physicalID_ref();
      if (auto tcvrIt = presentTcvrs.find(id);
          tcvrIt != presentTcvrs.end() && *tcvrIt->second.present_ref()) {
        presentTransceivers_.push_back(TransceiverID(id));
      } else {
        absentTransceivers_.push_back(TransceiverID(id));
      }
    }
    XLOG(DBG2) << "Transceivers num: [present:" << presentTransceivers_.size()
               << ", absent:" << absentTransceivers_.size() << "]";

    // Set pause remdiation so it won't trigger remediation
    setPauseRemediation(true);
  }

  const std::vector<TransceiverID>& getPresentTransceivers() const {
    return presentTransceivers_;
  }
  const std::vector<TransceiverID>& getAbsentTransceivers() const {
    return absentTransceivers_;
  }
  void setPauseRemediation(bool paused) {
    getHwQsfpEnsemble()->getWedgeManager()->setPauseRemediation(
        paused ? 600 : 0);
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
          // TODO(joseph5wu) Usually we only need to program optical
          // Transceiver which doesn't need to support split-out copper
          // cable for flex ports. Which means for the optical transceiver,
          // it usually has one programmed iphy port and profile. If in the
          // future, we need to support flex port copper transceiver
          // programming, we might need to combine the speeds of all flex
          // port to program such transceiver.
          if (programmedPortToPortInfo.size() == 1) {
            utility::HwTransceiverUtils::verifyTransceiverSettings(
                transceiver, programmedPortToPortInfo.begin()->second.profile);
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
    // XPHY_PORTS_PROGRAMMED -> TRANSCEIVER_PROGRAMMED
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
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
      }
      expectedStates.emplace(id, std::move(tcvrExpectedStates));
    }

    auto meetAllExpectedState =
        [](WedgeManager* wedgeMgr,
           TransceiverID id,
           TransceiverStateMachineState curState,
           std::queue<TransceiverStateMachineState>& tcvrExpectedStates) {
          // Check whether current state matches the head of the expected state
          // queue
          if (tcvrExpectedStates.empty()) {
            // Already meet all expected states.
            return true;
          } else if (curState == tcvrExpectedStates.front()) {
            tcvrExpectedStates.pop();
            if (curState ==
                TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED) {
              // TODO(joseph5wu) Add check to ensure the module remediation did
              // happen
              const auto& transceiver = wedgeMgr->getTransceiverInfo(id);
              auto mgmtInterface = apache::thrift::can_throw(
                  *transceiver.transceiverManagementInterface_ref());
              if (mgmtInterface == TransceiverManagementInterface::CMIS) {
                // CMIS will hard reset the module in
                // remediateFlakyTransceiver() Without clear hard reset, we
                // won't be able to detect such transceiver
              } else if (mgmtInterface == TransceiverManagementInterface::SFF) {
                // SFF will trigger tx enable and then reset low power mode.
              }
            } else if (
                curState ==
                TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED) {
              // Just finished transceiver programming
              // Only care enabled ports
              const auto programmedPortToPortInfo =
                  wedgeMgr->getProgrammedIphyPortToPortInfo(id);
              if (programmedPortToPortInfo.size() == 1) {
                utility::HwTransceiverUtils::verifyTransceiverSettings(
                    wedgeMgr->getTransceiverInfo(id),
                    programmedPortToPortInfo.begin()->second.profile);
              }
            }
            return tcvrExpectedStates.empty();
          }
          XLOG(WARN) << "Transceiver:" << id << " doesn't have expected state="
                     << apache::thrift::util::enumNameSafe(
                            tcvrExpectedStates.front())
                     << " but actual state="
                     << apache::thrift::util::enumNameSafe(curState);
          return false;
        };

    // Due to some platforms are easy to have i2c issue which causes the current
    // refresh not work as expected. Adding enough retries to make sure that we
    // at least can meet all `expectedStates` after 10 times.
    auto refreshStateMachinesTillMeetAllStates = [this,
                                                  wedgeMgr,
                                                  &expectedStates,
                                                  &meetAllExpectedState]() {
      wedgeMgr->refreshStateMachines();
      int numFailedTransceivers = 0;
      for (auto id : getPresentTransceivers()) {
        auto curState = wedgeMgr->getCurrentState(id);
        auto tcvrExpectedStates = expectedStates.find(id);
        // Only enabled transceivers are in expectedStates
        // Disabled ports should stay INACTIVE without remediation
        if (tcvrExpectedStates == expectedStates.end()) {
          EXPECT_EQ(curState, TransceiverStateMachineState::INACTIVE)
              << "Transceiver:" << id << " doesn't have expected state=INACTIVE"
              << " but actual state="
              << apache::thrift::util::enumNameSafe(curState);
        } else if (!meetAllExpectedState(
                       wedgeMgr, id, curState, tcvrExpectedStates->second)) {
          ++numFailedTransceivers;
        }
      }
      XLOG_IF(WARN, numFailedTransceivers)
          << numFailedTransceivers
          << " transceivers don't meet the expected state";
      return numFailedTransceivers == 0;
    };
    // Retry 10 times until all state machines reach expected states
    checkWithRetry(
        refreshStateMachinesTillMeetAllStates,
        10 /* retries */,
        std::chrono::milliseconds(10000) /* msBetweenRetry */);
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
