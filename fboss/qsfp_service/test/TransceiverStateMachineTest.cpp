/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/test/TransceiverManagerTestHelper.h"

#include "fboss/qsfp_service/TransceiverStateMachine.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"

namespace facebook::fboss {

/*
 * The recommended way to use TransceiverStateMachineTest to verify the event:
 * 1) Call getAllStates() to get all the states of state machine
 * 2) Call verifyStateMachine() with only the supported states to check the
 *    logic of processing a specified event. NOTE: this function will also
 *    erase the supported states from the input `states`
 * 3) Finally call verifyStateUnchanged() to check the rest of states still in
 *    the original state set to make sure even the code tries to process the
 *    specified event, these states will still stay the same.
 */
class TransceiverStateMachineTest : public TransceiverManagerTestHelper {
 public:
  CmisModule* overrideCmisModule() {
    auto xcvrImpl = std::make_unique<Cmis200GTransceiver>(id_);
    // This override function use ids starting from 1
    transceiverManager_->overrideMgmtInterface(
        static_cast<int>(id_) + 1,
        uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS));
    return static_cast<CmisModule*>(
        transceiverManager_->overrideTransceiverForTesting(
            id_,
            std::make_unique<CmisModule>(
                transceiverManager_.get(), std::move(xcvrImpl), 1)));
  }

  CmisModule* setState(TransceiverStateMachineState state) {
    // Always create a new transceiver so that we can make sure the state can
    // go back to the beginning state
    CmisModule* xcvr = overrideCmisModule();
    xcvr->detectPresence();
    switch (state) {
      case TransceiverStateMachineState::NOT_PRESENT:
        // default state is always NOT_PRESENT.
        break;
      case TransceiverStateMachineState::PRESENT:
        xcvr->stateUpdate(TransceiverStateMachineEvent::DETECT_TRANSCEIVER);
        break;
      // TODO(joseph5wu) Will support the reset states later
      default:
        break;
    }

    return xcvr;
  }

  std::set<TransceiverStateMachineState> getAllStates() const {
    return {
        TransceiverStateMachineState::NOT_PRESENT,
        TransceiverStateMachineState::PRESENT,
        // TODO(joseph5wu) Will support the reset states later
        // TransceiverStateMachineState::DISCOVERED,
        // TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
        // TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
        // TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
        // TransceiverStateMachineState::ACTIVE,
        // TransceiverStateMachineState::INACTIVE,
        // TransceiverStateMachineState::UPGRADING,
    };
  }

  template <typename VERIFY_FN>
  void verifyStateMachine(
      TransceiverStateMachineState curState,
      TransceiverStateMachineEvent event,
      std::set<TransceiverStateMachineState>& states,
      VERIFY_FN verify) {
    if (states.find(curState) == states.end()) {
      // Current state is no longer in the state set, skip checking it
      return;
    }
    auto xcvr = setState(curState);
    // Trigger state update with `event`
    xcvr->stateUpdate(event);
    // Verify the result after update finishes
    verify();
    // Remove from the state set
    states.erase(curState);
  }

  void verifyStateUnchanged(
      TransceiverStateMachineEvent event,
      std::set<TransceiverStateMachineState>& states) {
    for (auto state : states) {
      auto xcvr = setState(state);
      // Trigger state update with `event`
      xcvr->stateUpdate(event);
      auto newState = transceiverManager_->getCurrentState(id_);
      EXPECT_EQ(newState, state)
          << "Transceiver=0 state doesn't match after Event="
          << apache::thrift::util::enumNameSafe(event)
          << ", preState=" << apache::thrift::util::enumNameSafe(state)
          << ", newState=" << apache::thrift::util::enumNameSafe(newState);
    }
  }

  const TransceiverID id_ = TransceiverID(0);
};

TEST_F(TransceiverStateMachineTest, defaultState) {
  overrideCmisModule();
  EXPECT_EQ(
      transceiverManager_->getCurrentState(id_),
      TransceiverStateMachineState::NOT_PRESENT);
  // Check state machine attributes should be reset to default values
  const auto& stateMachine =
      transceiverManager_->getStateMachineForTesting(id_);
  EXPECT_EQ(
      stateMachine.get_attribute(transceiverMgrPtr), transceiverManager_.get());
  EXPECT_EQ(stateMachine.get_attribute(transceiverID), id_);
  EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
  EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
  EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
  EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
  EXPECT_FALSE(stateMachine.get_attribute(needResetDataPath));
}

TEST_F(TransceiverStateMachineTest, detectTransceiver) {
  auto allStates = getAllStates();
  // Only NOT_PRESENT can accept DETECT_TRANSCEIVER event
  verifyStateMachine(
      TransceiverStateMachineState::NOT_PRESENT,
      TransceiverStateMachineEvent::DETECT_TRANSCEIVER,
      allStates,
      [this]() {
        // New state should be PRESENT now
        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::PRESENT);
      });
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      TransceiverStateMachineEvent::DETECT_TRANSCEIVER, allStates);
}
} // namespace facebook::fboss
