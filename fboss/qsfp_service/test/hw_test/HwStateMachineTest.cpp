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
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

namespace facebook::fboss {

class HwStateMachineTest : public HwTest {
 public:
  HwStateMachineTest(bool setupOverrideTcvrToPortAndProfile = false)
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
  }

  const std::vector<TransceiverID>& getPresentTransceivers() const {
    return presentTransceivers_;
  }
  const std::vector<TransceiverID>& getAbsentTransceivers() const {
    return absentTransceivers_;
  }

 private:
  // Forbidden copy constructor and assignment operator
  HwStateMachineTest(HwStateMachineTest const&) = delete;
  HwStateMachineTest& operator=(HwStateMachineTest const&) = delete;

  std::vector<TransceiverID> presentTransceivers_;
  std::vector<TransceiverID> absentTransceivers_;
};

TEST_F(HwStateMachineTest, CheckOpticsDetection) {
  auto verify = [&]() {
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    // Default HwTest::Setup() already has a refresh, so all present
    // transceivers should be in DISCOVERED state; while
    // not present front panel ports should still be NOT_PRESENT
    for (auto id : getPresentTransceivers()) {
      auto curState = wedgeMgr->getCurrentState(id);
      EXPECT_EQ(curState, TransceiverStateMachineState::DISCOVERED)
          << "Transceiver:" << id
          << " Actual: " << getTransceiverStateMachineStateName(curState)
          << ", Expected: DISCOVERED";
    }
    for (auto id : getAbsentTransceivers()) {
      auto curState = wedgeMgr->getCurrentState(id);
      EXPECT_EQ(curState, TransceiverStateMachineState::NOT_PRESENT)
          << "Transceiver:" << id
          << " Actual: " << getTransceiverStateMachineStateName(curState)
          << ", Expected: NOT_PRESENT";
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

class HwStateMachineTestWithOverrideTcvrToPortAndProfile
    : public HwStateMachineTest {
 public:
  HwStateMachineTestWithOverrideTcvrToPortAndProfile()
      : HwStateMachineTest(true /* setupOverrideTcvrToPortAndProfile */) {}
};

TEST_F(
    HwStateMachineTestWithOverrideTcvrToPortAndProfile,
    CheckPortsProgrammed) {
  auto setup = [this]() {
    // Due to some platforms are easy to have i2c issue that cause the current
    // refresh work perfectly. Adding enough retries to make sure that we at
    // least can secure TRANSCEIVER_PROGRAMMED after 10 times.
    auto refreshStateMachinesTillTcvrProgrammed = [this]() {
      auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
      // Later will change it to refreshStateMachines()
      wedgeMgr->refreshTransceivers();
      for (auto id : getPresentTransceivers()) {
        auto curState = wedgeMgr->getCurrentState(id);
        if (curState != TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED) {
          return false;
        }
      }
      return true;
    };
    // Retry 10 times until all state machine reach TRANSCEIVER_PROGRAMMED
    checkWithRetry(refreshStateMachinesTillTcvrProgrammed);
  };
  auto verify = [this]() {
    auto checkTransceiverProgrammed =
        [this](const std::vector<TransceiverID>& tcvrs) {
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
            const auto programmedIphyPortAndProfile =
                wedgeMgr->getProgrammedIphyPortAndProfile(id);
            const auto& transceiver = wedgeMgr->getTransceiverInfo(id);
            if (portAndProfile.empty()) {
              // If iphy port and profile is empty, it means the ports are
              // disabled. We don't need to program such transceivers
              EXPECT_TRUE(programmedIphyPortAndProfile.empty());
            } else {
              EXPECT_EQ(
                  programmedIphyPortAndProfile.size(), portAndProfile.size());
              for (auto [portID, profileID] : programmedIphyPortAndProfile) {
                auto expectedPortAndProfileIt = portAndProfile.find(portID);
                EXPECT_TRUE(expectedPortAndProfileIt != portAndProfile.end());
                EXPECT_EQ(profileID, expectedPortAndProfileIt->second);
                if (std::find(xphyPorts.begin(), xphyPorts.end(), portID) !=
                    xphyPorts.end()) {
                  utility::verifyXphyPort(
                      portID, profileID, transceiver, getHwQsfpEnsemble());
                }
              }
              // TODO(joseph5wu) Usually we only need to program optical
              // Transceiver which doesn't need to support split-out copper
              // cable for flex ports. Which means for the optical transceiver,
              // it usually has one programmed iphy port and profile. If in the
              // future, we need to support flex port copper transceiver
              // programming, we might need to combine the speeds of all flex
              // port to program such transceiver.
              if (programmedIphyPortAndProfile.size() == 1) {
                utility::verifyTransceiverSettings(
                    transceiver, programmedIphyPortAndProfile.begin()->second);
              }
            }
          }
        };

    checkTransceiverProgrammed(getPresentTransceivers());
    checkTransceiverProgrammed(getAbsentTransceivers());
  };
  // Because state machine is not warmboot-able, which means we have to rebuild
  // the state machine from init state for each transceiver every time
  // qsfp_service restarts, no matter coldboot or warmboot.
  // So we only need to very non-warmboot case here.
  // For verifying warmboot ability of each component(iphy/xphy/tcvr)
  // programming, we already have other tests to cover, like HwPortProfileTest.
  setup();
  verify();
}
} // namespace facebook::fboss
