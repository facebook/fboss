/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

namespace facebook::fboss {

class HwStateMachineTest : public HwTest {
 public:
  HwStateMachineTest() : HwTest(true) {}

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
        unpresentTransceivers_.push_back(TransceiverID(id));
      }
    }
    XLOG(DBG2) << "Transceivers num: [present:" << presentTransceivers_.size()
               << ", unpresent:" << unpresentTransceivers_.size() << "]";
  }

  const std::vector<TransceiverID> getPresentTransceivers() const {
    return presentTransceivers_;
  }
  const std::vector<TransceiverID> getUnpresentTransceivers() const {
    return unpresentTransceivers_;
  }

 private:
  // Forbidden copy constructor and assignment operator
  HwStateMachineTest(HwStateMachineTest const&) = delete;
  HwStateMachineTest& operator=(HwStateMachineTest const&) = delete;

  std::vector<TransceiverID> presentTransceivers_;
  std::vector<TransceiverID> unpresentTransceivers_;
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
    for (auto id : getUnpresentTransceivers()) {
      auto curState = wedgeMgr->getCurrentState(id);
      EXPECT_EQ(curState, TransceiverStateMachineState::NOT_PRESENT)
          << "Transceiver:" << id
          << " Actual: " << getTransceiverStateMachineStateName(curState)
          << ", Expected: NOT_PRESENT";
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
