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
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

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
    XLOG(INFO) << "Making CMIS QSFP for " << id_;
    return static_cast<CmisModule*>(
        transceiverManager_->overrideTransceiverForTesting(
            id_,
            std::make_unique<CmisModule>(
                transceiverManager_.get(), std::move(xcvrImpl), 1)));
  }

  void setState(TransceiverStateMachineState state) {
    // Always create a new transceiver so that we can make sure the state can
    // go back to the beginning state
    xcvr_ = overrideCmisModule();
    xcvr_->detectPresence();
    switch (state) {
      case TransceiverStateMachineState::NOT_PRESENT:
        // default state is always NOT_PRESENT.
        break;
      case TransceiverStateMachineState::PRESENT:
        // Because we want to verify two events: DETECT_TRANSCEIVER and
        // READ_EEPROM seperately, we have to make sure we updateQsfpData with
        // allPages=True after `DETECT_TRANSCEIVER` but before `READ_EEPROM`
        // to match the same behavior as QsfpModule::refreshLocked()
        xcvr_->stateUpdate(TransceiverStateMachineEvent::DETECT_TRANSCEIVER);
        xcvr_->updateQsfpData(true);
        break;
      case TransceiverStateMachineState::DISCOVERED:
        transceiverManager_->refreshStateMachines();
        break;
      // TODO(joseph5wu) Will support the reset states later
      default:
        break;
    }
  }

  std::set<TransceiverStateMachineState> getAllStates() const {
    return {
        TransceiverStateMachineState::NOT_PRESENT,
        TransceiverStateMachineState::PRESENT,
        TransceiverStateMachineState::DISCOVERED,
        // TODO(joseph5wu) Will support the reset states later
        // TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
        // TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
        // TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
        // TransceiverStateMachineState::ACTIVE,
        // TransceiverStateMachineState::INACTIVE,
        // TransceiverStateMachineState::UPGRADING,
    };
  }

  template <typename PRE_UPDATE_FN, typename VERIFY_FN>
  void verifyStateMachine(
      TransceiverStateMachineState curState,
      TransceiverStateMachineEvent event,
      std::set<TransceiverStateMachineState>& states,
      PRE_UPDATE_FN preUpdate,
      VERIFY_FN verify) {
    if (states.find(curState) == states.end()) {
      // Current state is no longer in the state set, skip checking it
      return;
    }
    setState(curState);
    // Call preUpdate() before actual stateUpdate()
    preUpdate();
    // Trigger state update with `event`
    xcvr_->stateUpdate(event);
    // Verify the result after update finishes
    verify();
    // Remove from the state set
    states.erase(curState);
    ::testing::Mock::VerifyAndClearExpectations(transceiverManager_.get());
  }

  void verifyStateUnchanged(
      TransceiverStateMachineEvent event,
      std::set<TransceiverStateMachineState>& states) {
    for (auto state : states) {
      setState(state);
      // Trigger state update with `event`
      xcvr_->stateUpdate(event);
      auto newState = transceiverManager_->getCurrentState(id_);
      EXPECT_EQ(newState, state)
          << "Transceiver=0 state doesn't match after Event="
          << apache::thrift::util::enumNameSafe(event)
          << ", preState=" << apache::thrift::util::enumNameSafe(state)
          << ", newState=" << apache::thrift::util::enumNameSafe(newState);
    }
  }

  CmisModule* xcvr_;
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
      []() {} /* empty preUpdateFn */,
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

TEST_F(TransceiverStateMachineTest, readEeprom) {
  auto allStates = getAllStates();
  // Only PRESENT can accept READ_EEPROM event
  verifyStateMachine(
      TransceiverStateMachineState::PRESENT,
      TransceiverStateMachineEvent::READ_EEPROM,
      allStates,
      [this]() {
        // Make sure `discoverTransceiver` has been called
        EXPECT_CALL(*transceiverManager_, verifyEepromChecksums(id_)).Times(1);
      },
      [this]() {
        // New state should be DISCOVERED now
        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::DISCOVERED);

        // Enter DISCOVERED will also call `resetProgrammingAttributes`
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));

        // Before fetching TransceiverInfo, make sure we call refresh()
        // to update the cached TransceiverInfo
        xcvr_->refresh();
        const auto& info = transceiverManager_->getTransceiverInfo(id_);
        utility::HwTransceiverUtils::verifyDiagsCapability(
            info,
            transceiverManager_->getDiagsCapability(id_),
            false /* skipCheckingIndividualCapability */);
      });
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(TransceiverStateMachineEvent::READ_EEPROM, allStates);
}
} // namespace facebook::fboss
