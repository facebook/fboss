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
#include "fboss/qsfp_service/module/sff/Sff8472Module.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/qsfp_service/module/tests/MockSffModule.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

namespace facebook::fboss {

namespace {
const auto kWarmBootAgentConfigChangedFnStr =
    "Triggering Agent config changed w/ warm boot";
const auto kColdBootAgentConfigChangedFnStr =
    "Triggering Agent config changed w/ cold boot";
const auto kRemediateTransceiverFnStr = "Remediating transceiver";
} // namespace

class MockSff8472TransceiverImpl : public Sfp10GTransceiver {
 public:
  explicit MockSff8472TransceiverImpl(int module) : Sfp10GTransceiver(module) {}

  MOCK_METHOD0(detectTransceiver, bool());
};

class MockSff8472Module : public Sff8472Module {
 public:
  explicit MockSff8472Module(
      TransceiverManager* transceiverManager,
      std::unique_ptr<MockSff8472TransceiverImpl> qsfpImpl)
      : Sff8472Module(transceiverManager, std::move(qsfpImpl)) {
    ON_CALL(*this, updateQsfpData(testing::_))
        .WillByDefault(testing::Assign(&dirty_, false));
  }

  MockSff8472TransceiverImpl* getTransceiverImpl() {
    return static_cast<MockSff8472TransceiverImpl*>(qsfpImpl_.get());
  }

  MOCK_METHOD0(configureModule, void());
  MOCK_METHOD1(customizeTransceiverLocked, void(cfg::PortSpeed));

  MOCK_METHOD1(
      ensureRxOutputSquelchEnabled,
      void(const std::vector<HostLaneSettings>&));
  MOCK_METHOD0(resetDataPath, void());
  MOCK_METHOD1(updateQsfpData, void(bool));
  MOCK_METHOD1(updateCachedTransceiverInfoLocked, void(ModuleStatus));
};

/*
 * Because this is a TransceiverStateMachine test, we only care the functions
 * we need to call in the process of New Port Programming using the new state
 * machine logic. We don't really care about whether the programming
 * iphy/xphy/xcvr actually changes the hardware in a unit test.
 * Therefore, we just need to mock the CmisModule and then check whether these
 * functions have been called correctly.
 */
class MockCmisTransceiverImpl : public Cmis200GTransceiver {
 public:
  explicit MockCmisTransceiverImpl(int module) : Cmis200GTransceiver(module) {}

  MOCK_METHOD0(detectTransceiver, bool());
};

class MockCmisModule : public CmisModule {
 public:
  explicit MockCmisModule(
      TransceiverManager* transceiverManager,
      std::unique_ptr<MockCmisTransceiverImpl> qsfpImpl)
      : CmisModule(transceiverManager, std::move(qsfpImpl)) {
    ON_CALL(*this, updateQsfpData(testing::_))
        .WillByDefault(testing::Assign(&dirty_, false));
  }

  MockCmisTransceiverImpl* getTransceiverImpl() {
    return static_cast<MockCmisTransceiverImpl*>(qsfpImpl_.get());
  }

  MOCK_METHOD0(configureModule, void());
  MOCK_METHOD1(customizeTransceiverLocked, void(cfg::PortSpeed));

  MOCK_METHOD1(
      ensureRxOutputSquelchEnabled,
      void(const std::vector<HostLaneSettings>&));
  MOCK_METHOD0(resetDataPath, void());
  MOCK_METHOD1(updateQsfpData, void(bool));
  MOCK_METHOD1(updateCachedTransceiverInfoLocked, void(ModuleStatus));
  MOCK_CONST_METHOD0(ensureOutOfReset, void());
};

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
  enum class TransceiverType {
    CMIS,
    MOCK_CMIS,
    MOCK_SFF,
    MOCK_SFF8472,
  };

  QsfpModule* overrideTransceiver(
      TransceiverType type = TransceiverType::CMIS) {
    // Set port status to DOWN so that we can remove the transceiver correctly
    updateTransceiverActiveState(false /* up */, true /* enabled */);

    auto overrideCmisModule = [this](bool isMock) {
      // This override function use ids starting from 1
      transceiverManager_->overrideMgmtInterface(
          static_cast<int>(id_) + 1,
          uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS));
      if (isMock) {
        XLOG(INFO) << "Making Mock CMIS QSFP for " << id_;
        auto xcvrImpl = std::make_unique<MockCmisTransceiverImpl>(id_);
        EXPECT_CALL(*xcvrImpl.get(), detectTransceiver())
            .WillRepeatedly(::testing::Return(true));
        return transceiverManager_->overrideTransceiverForTesting(
            id_,
            std::make_unique<MockCmisModule>(
                transceiverManager_.get(), std::move(xcvrImpl)));
      } else {
        XLOG(INFO) << "Making CMIS QSFP for " << id_;
        auto xcvrImpl = std::make_unique<Cmis200GTransceiver>(id_);
        return transceiverManager_->overrideTransceiverForTesting(
            id_,
            std::make_unique<CmisModule>(
                transceiverManager_.get(), std::move(xcvrImpl)));
      }
    };

    auto overrideSffModule = [this]() {
      auto xcvrImpl = std::make_unique<SffCwdm4Transceiver>(id_);
      // This override function use ids starting from 1
      transceiverManager_->overrideMgmtInterface(
          static_cast<int>(id_) + 1,
          uint8_t(TransceiverModuleIdentifier::QSFP));
      XLOG(INFO) << "Making Mock SFF QSFP for " << id_;
      auto xcvr = transceiverManager_->overrideTransceiverForTesting(
          id_,
          std::make_unique<MockSffModule>(
              transceiverManager_.get(), std::move(xcvrImpl)));
      return xcvr;
    };

    auto overrideSff8472Module = [this]() {
      transceiverManager_->overrideMgmtInterface(
          static_cast<int>(id_) + 1,
          uint8_t(TransceiverModuleIdentifier::SFP_PLUS));
      XLOG(INFO) << "Making Mock SFF8472 SFP for " << id_;
      auto xcvrImpl = std::make_unique<MockSff8472TransceiverImpl>(id_);
      EXPECT_CALL(*xcvrImpl.get(), detectTransceiver())
          .WillRepeatedly(::testing::Return(true));
      return transceiverManager_->overrideTransceiverForTesting(
          id_,
          std::make_unique<MockSff8472Module>(
              transceiverManager_.get(), std::move(xcvrImpl)));
    };

    Transceiver* xcvr;
    switch (type) {
      case TransceiverType::CMIS:
        xcvr = overrideCmisModule(false);
        break;
      case TransceiverType::MOCK_CMIS:
        xcvr = overrideCmisModule(true);
        break;
      case TransceiverType::MOCK_SFF:
        xcvr = overrideSffModule();
        break;
      case TransceiverType::MOCK_SFF8472:
        xcvr = overrideSff8472Module();
        break;
    }
    EXPECT_TRUE(xcvr->getLastDownTime() != 0);

    // Remove the override config we set before
    transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
        emptyOverrideTcvrToPortAndProfile_);

    auto qsfpModuleXcvr = static_cast<QsfpModule*>(xcvr);
    // Assign a default transceiver info so that we can set expectations based
    // on it even when transceiverInfo hasn't been updated for the first time
    *(qsfpModuleXcvr->info_.wlock()) = TransceiverInfo();
    return qsfpModuleXcvr;
  }

  void setState(TransceiverStateMachineState state) {
    // Pause remediation
    transceiverManager_->setPauseRemediation(60, nullptr);

    auto discoverTransceiver = [this]() {
      // One refresh can finish discoverring xcvr after detectPresence
      transceiverManager_->refreshStateMachines();
    };
    auto programIphy = [this]() {
      transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
          overrideTcvrToPortAndProfile_);
      // refreshStateMachines() will first refresh all transceivers to bring
      // them to DISCOVERED state, and then program the iphy ports
      transceiverManager_->refreshStateMachines();
    };
    auto programIphyAndXcvr = [this]() {
      transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
          overrideTcvrToPortAndProfile_);
      transceiverManager_->refreshStateMachines();
      // Because MockWedgeManager doesn't have PhyManager, so this second
      // refreshStateMachines() will actually trigger programming xcvr
      transceiverManager_->refreshStateMachines();
    };
    auto setXcvtActiveState = [this, &programIphyAndXcvr](bool portUp) {
      programIphyAndXcvr();
      transceiverManager_->setOverrideAgentPortStatusForTesting(
          portUp /* up */, true /* enabled */, false /* clearOnly */);
      transceiverManager_->refreshStateMachines();
    };

    switch (state) {
      case TransceiverStateMachineState::NOT_PRESENT:
        // default state is always NOT_PRESENT.
        break;
      case TransceiverStateMachineState::PRESENT:
        // Because we want to verify two events: DETECT_TRANSCEIVER and
        // READ_EEPROM seperately, we have to make sure we updateQsfpData with
        // allPages=True after `DETECT_TRANSCEIVER` but before `READ_EEPROM`
        // to match the same behavior as QsfpModule::refreshLocked()
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::DETECT_TRANSCEIVER);
        xcvr_->updateQsfpData(true);
        break;
      case TransceiverStateMachineState::DISCOVERED:
        discoverTransceiver();
        break;
      case TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED:
        programIphy();
        break;
      case TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED:
        programIphy();
        // Use updateStateBlocking() to skip PhyManager check
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::PROGRAM_XPHY);
        break;
      case TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED:
        programIphyAndXcvr();
        break;
      case TransceiverStateMachineState::ACTIVE:
        setXcvtActiveState(true);
        break;
      case TransceiverStateMachineState::INACTIVE:
        setXcvtActiveState(false);
        break;
      case TransceiverStateMachineState::UPGRADING:
        throw FbossError("Doesn't support UPGRADING state yet");
    }
    auto curState = transceiverManager_->getCurrentState(id_);
    EXPECT_EQ(curState, state)
        << "Transceiver=0 state doesn't match state expected="
        << apache::thrift::util::enumNameSafe(state)
        << ", actual=" << apache::thrift::util::enumNameSafe(curState);
  }

  std::set<TransceiverStateMachineState> getAllStates() const {
    return {
        TransceiverStateMachineState::NOT_PRESENT,
        TransceiverStateMachineState::PRESENT,
        TransceiverStateMachineState::DISCOVERED,
        TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
        TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
        TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
        TransceiverStateMachineState::ACTIVE,
        TransceiverStateMachineState::INACTIVE,
        // TODO(joseph5wu) Will support the reset states later
        // TransceiverStateMachineState::UPGRADING,
    };
  }

  template <
      typename PRE_UPDATE_FN,
      typename STATE_UPDATE_FN,
      typename VERIFY_FN>
  void verifyStateMachine(
      const std::set<TransceiverStateMachineState>& supportedStates,
      TransceiverStateMachineState expectedState,
      std::set<TransceiverStateMachineState>& states,
      PRE_UPDATE_FN preUpdate,
      STATE_UPDATE_FN stateUpdate,
      VERIFY_FN verify,
      TransceiverType type,
      const std::string& updateStr) {
    for (auto preState : supportedStates) {
      if (states.find(preState) == states.end()) {
        // Current state is no longer in the state set, skip checking it
        continue;
      }
      XLOG(INFO) << "Verifying Transceiver=0 state CHANGED by " << updateStr
                 << " from preState="
                 << apache::thrift::util::enumNameSafe(preState)
                 << " to newState="
                 << apache::thrift::util::enumNameSafe(expectedState);
      // Always create a new transceiver so that we can make sure the state
      // can go back to the beginning state
      xcvr_ = overrideTransceiver(type);
      setState(preState);

      // Call preUpdate() before actual stateUpdate()
      preUpdate();

      // Trigger state update
      stateUpdate();

      // Check current state matches expected state
      auto curState = transceiverManager_->getCurrentState(id_);
      EXPECT_EQ(curState, expectedState)
          << "Transceiver=0 state doesn't match after " << updateStr
          << ", preState=" << apache::thrift::util::enumNameSafe(preState)
          << ", expected new state="
          << apache::thrift::util::enumNameSafe(expectedState)
          << ", actual=" << apache::thrift::util::enumNameSafe(curState);

      // Verify the result after update finishes
      verify();

      // Remove from the state set
      states.erase(preState);
      ::testing::Mock::VerifyAndClearExpectations(transceiverManager_.get());
      ::testing::Mock::VerifyAndClearExpectations(xcvr_);
    }
  }

  template <typename PRE_UPDATE_FN, typename VERIFY_FN>
  void verifyStateMachine(
      const std::set<TransceiverStateMachineState>& supportedStates,
      TransceiverStateMachineEvent event,
      TransceiverStateMachineState expectedState,
      std::set<TransceiverStateMachineState>& states,
      PRE_UPDATE_FN preUpdate,
      VERIFY_FN verify,
      TransceiverType type = TransceiverType::CMIS) {
    auto stateUpdateFn = [this, event]() {
      transceiverManager_->updateStateBlocking(id_, event);
    };
    auto stateUpdateFnStr = folly::to<std::string>(
        "Event=", apache::thrift::util::enumNameSafe(event));
    verifyStateMachine(
        supportedStates,
        expectedState,
        states,
        preUpdate,
        stateUpdateFn,
        verify,
        type,
        stateUpdateFnStr);
  }

  template <
      typename PRE_UPDATE_FN,
      typename STATE_UPDATE_FN,
      typename VERIFY_FN>
  void verifyStateUnchanged(
      std::set<TransceiverStateMachineState>& states,
      PRE_UPDATE_FN preUpdate,
      STATE_UPDATE_FN stateUpdate,
      VERIFY_FN verify,
      TransceiverType type,
      const std::string& updateStr) {
    for (auto state : states) {
      XLOG(INFO) << "Verifying Transceiver=0 State="
                 << apache::thrift::util::enumNameSafe(state)
                 << " UNCHANGED by " << updateStr;
      // Always create a new transceiver so that we can make sure the state
      // can go back to the beginning state
      xcvr_ = overrideTransceiver(type);
      setState(state);

      // Call preUpdate() before actual stateUpdate()
      preUpdate();

      // Trigger state update
      stateUpdate();

      // Check new state doesn't change
      auto newState = transceiverManager_->getCurrentState(id_);
      EXPECT_EQ(newState, state)
          << "Transceiver=0 unchanged state doesn't match after " << updateStr
          << ", preState=" << apache::thrift::util::enumNameSafe(state)
          << ", newState=" << apache::thrift::util::enumNameSafe(newState);

      // Verify the result after update finishes
      verify();

      ::testing::Mock::VerifyAndClearExpectations(transceiverManager_.get());
      ::testing::Mock::VerifyAndClearExpectations(xcvr_);
    }
  }

  template <typename PRE_UPDATE_FN, typename VERIFY_FN>
  void verifyStateUnchanged(
      TransceiverStateMachineEvent event,
      std::set<TransceiverStateMachineState>& states,
      PRE_UPDATE_FN preUpdate,
      VERIFY_FN verify,
      TransceiverType type = TransceiverType::CMIS) {
    auto stateUpdateFn = [this, event]() {
      transceiverManager_->updateStateBlocking(id_, event);
    };
    auto stateUpdateFnStr = folly::to<std::string>(
        "Event=", apache::thrift::util::enumNameSafe(event));
    verifyStateUnchanged(
        states, preUpdate, stateUpdateFn, verify, type, stateUpdateFnStr);
  }

  void verifyResetProgrammingAttributes() {
    // Check state machine attributes should be reset to default values
    const auto& stateMachine =
        transceiverManager_->getStateMachineForTesting(id_);
    EXPECT_EQ(
        stateMachine.get_attribute(transceiverMgrPtr),
        transceiverManager_.get());
    EXPECT_EQ(stateMachine.get_attribute(transceiverID), id_);
    EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
    EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
    EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
    EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
    EXPECT_TRUE(
        transceiverManager_->getProgrammedIphyPortToPortInfo(id_).empty());
  }

  void updateTransceiverActiveState(bool up, bool enabled) {
    PortStatus status;
    status.enabled() = enabled;
    status.up() = up;
    transceiverManager_->updateTransceiverActiveState(
        {id_}, {{portId_, status}});
    // Sleep 1s to avoid the state machine handling the event too fast
    /* sleep override */
    sleep(1);
  }

  void setProgramCmisModuleExpectation(bool isProgrammed) {
    ::testing::Sequence s;
    setProgramCmisModuleExpectation(isProgrammed, s);
  }

  void setProgramCmisModuleExpectation(
      bool isProgrammed,
      ::testing::Sequence& s) {
    int callTimes = isProgrammed ? 1 : 0;
    MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
    EXPECT_CALL(*mockXcvr, customizeTransceiverLocked(cfg::PortSpeed::HUNDREDG))
        .Times(callTimes)
        .InSequence(s);
    EXPECT_CALL(*mockXcvr, updateQsfpData(true)).Times(callTimes).InSequence(s);
    EXPECT_CALL(*mockXcvr, configureModule()).Times(callTimes).InSequence(s);

    const auto& info = transceiverManager_->getTransceiverInfo(id_);
    if (auto settings = info.settings()) {
      if (auto hostLaneSettings = settings->hostLaneSettings()) {
        EXPECT_CALL(*mockXcvr, ensureRxOutputSquelchEnabled(*hostLaneSettings))
            .Times(callTimes)
            .InSequence(s);
      }
    }

    // Normal transceiver programming shouldn't trigger resetDataPath()
    EXPECT_CALL(*mockXcvr, resetDataPath()).Times(0).InSequence(s);

    EXPECT_CALL(*mockXcvr, updateQsfpData(false))
        .Times(callTimes)
        .InSequence(s);
    ModuleStatus moduleStatus;
    EXPECT_CALL(*mockXcvr, updateCachedTransceiverInfoLocked(moduleStatus))
        .Times(callTimes)
        .InSequence(s);
  }

  void enableRemediationTesting() {
    gflags::SetCommandLineOptionWithMode(
        "initial_remediate_interval", "0", gflags::SET_FLAGS_DEFAULT);
    gflags::SetCommandLineOptionWithMode(
        "remediate_interval", "0", gflags::SET_FLAGS_DEFAULT);
  }

  void triggerAgentConfigChanged(bool isAgentColdBoot) {
    // Override ConfigAppliedInfo
    ConfigAppliedInfo configAppliedInfo;
    auto currentInMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    configAppliedInfo.lastAppliedInMs() = currentInMs.count();
    if (isAgentColdBoot) {
      configAppliedInfo.lastColdbootAppliedInMs() = currentInMs.count();
    }
    transceiverManager_->setOverrideAgentConfigAppliedInfoForTesting(
        configAppliedInfo);

    transceiverManager_->triggerAgentConfigChangeEvent();
  }

  void triggerRemediateEvents() {
    transceiverManager_->triggerRemediateEvents(stableXcvrIds_);
  }

  void setMockCmisPresence(bool isPresent) {
    MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
    auto xcvrImpl = mockXcvr->getTransceiverImpl();
    EXPECT_CALL(*xcvrImpl, detectTransceiver())
        .WillRepeatedly(::testing::Return(isPresent));
  }

  QsfpModule* xcvr_;
  const TransceiverID id_ = TransceiverID(0);
  const std::vector<TransceiverID> stableXcvrIds_ = {id_};
  const PortID portId_ = PortID(1);
  const cfg::PortProfileID profile_ =
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL;
  const TransceiverManager::OverrideTcvrToPortAndProfile
      overrideTcvrToPortAndProfile_ = {
          {id_,
           {
               {portId_, profile_},
           }}};
  const TransceiverManager::OverrideTcvrToPortAndProfile
      emptyOverrideTcvrToPortAndProfile_ = {};
};

TEST_F(TransceiverStateMachineTest, defaultState) {
  overrideTransceiver();
  EXPECT_EQ(
      transceiverManager_->getCurrentState(id_),
      TransceiverStateMachineState::NOT_PRESENT);
  verifyResetProgrammingAttributes();
}

TEST_F(TransceiverStateMachineTest, detectTransceiver) {
  auto allStates = getAllStates();
  // Only NOT_PRESENT can accept DETECT_TRANSCEIVER event
  verifyStateMachine(
      {TransceiverStateMachineState::NOT_PRESENT,
       TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::INACTIVE},
      TransceiverStateMachineEvent::DETECT_TRANSCEIVER,
      TransceiverStateMachineState::PRESENT /* expected state */,
      allStates,
      []() {} /* preUpdate */,
      []() {} /* verify */);
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      TransceiverStateMachineEvent::DETECT_TRANSCEIVER,
      allStates,
      []() {} /* preUpdate */,
      []() {} /* verify */);
}

TEST_F(TransceiverStateMachineTest, readEeprom) {
  auto allStates = getAllStates();
  // Only PRESENT can accept READ_EEPROM event
  verifyStateMachine(
      {TransceiverStateMachineState::PRESENT},
      TransceiverStateMachineEvent::READ_EEPROM,
      TransceiverStateMachineState::DISCOVERED /* expected state */,
      allStates,
      [this]() {
        // Make sure `discoverTransceiver` has been called
        EXPECT_CALL(*transceiverManager_, verifyEepromChecksums(id_)).Times(1);
      },
      [this]() {
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
  verifyStateUnchanged(
      TransceiverStateMachineEvent::READ_EEPROM,
      allStates,
      []() {} /* preUpdate */,
      []() {} /* verify */);
}

TEST_F(TransceiverStateMachineTest, programIphy) {
  auto allStates = getAllStates();
  // Both NOT_PRESENT and DISCOVERED can accept PROGRAM_IPHY event
  verifyStateMachine(
      {TransceiverStateMachineState::NOT_PRESENT,
       TransceiverStateMachineState::DISCOVERED},
      TransceiverStateMachineEvent::PROGRAM_IPHY,
      TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED /* expected state */,
      allStates,
      [this]() {
        transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
            overrideTcvrToPortAndProfile_);
      },
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isIphyProgrammed should be true
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));

        // checked programmed iphy ports match overrideTcvrToPortAndProfile_
        for (const auto& xcvrToPortAndProfile : overrideTcvrToPortAndProfile_) {
          const auto& programmedIphyPorts =
              transceiverManager_->getProgrammedIphyPortToPortInfo(
                  xcvrToPortAndProfile.first);
          EXPECT_EQ(
              xcvrToPortAndProfile.second.size(), programmedIphyPorts.size());
          for (auto portToProfile : xcvrToPortAndProfile.second) {
            const auto& it = programmedIphyPorts.find(portToProfile.first);
            EXPECT_TRUE(it != programmedIphyPorts.end());
            EXPECT_EQ(portToProfile.second, it->second.profile);
          }
        }
      });
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      TransceiverStateMachineEvent::PROGRAM_IPHY,
      allStates,
      []() {} /* preUpdate */,
      []() {} /* verify */);
}

TEST_F(TransceiverStateMachineTest, programIphyFailed) {
  std::set<TransceiverStateMachineState> stateSet = {
      TransceiverStateMachineState::NOT_PRESENT,
      TransceiverStateMachineState::DISCOVERED};
  // If we never set overrideTcvrToPortAndProfile_, programming iphy won't work
  verifyStateUnchanged(
      TransceiverStateMachineEvent::PROGRAM_IPHY,
      stateSet,
      []() {} /* preUpdate */,
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isIphyProgrammed should still be false
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));

        // checked programmed iphy ports
        const auto& programmedIphyPorts =
            transceiverManager_->getProgrammedIphyPortToPortInfo(id_);
        EXPECT_TRUE(programmedIphyPorts.empty());

        // Now set the override transceiver to port and profile to make
        // programming iphy ports work
        transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
            overrideTcvrToPortAndProfile_);
        // Then try again, it should succeed
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::PROGRAM_IPHY);

        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED);
        const auto& newStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isIphyProgrammed should be true
        EXPECT_TRUE(newStateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(newStateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(newStateMachine.get_attribute(isTransceiverProgrammed));
        // checked programmed iphy ports
        const auto& newProgrammedIphyPorts =
            transceiverManager_->getProgrammedIphyPortToPortInfo(id_);
        EXPECT_EQ(newProgrammedIphyPorts.size(), 1);
      });
}

TEST_F(TransceiverStateMachineTest, programXphy) {
  auto allStates = getAllStates();
  // Only IPHY_PORTS_PROGRAMMED can accept PROGRAM_XPHY event
  verifyStateMachine(
      {TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED},
      TransceiverStateMachineEvent::PROGRAM_XPHY,
      TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED /* expected state */,
      allStates,
      [this]() {
        // Make sure `programExternalPhyPorts` has been called
        EXPECT_CALL(*transceiverManager_, programExternalPhyPorts(id_, false))
            .Times(1);
      },
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isXphyProgrammed should be true
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
      });
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      TransceiverStateMachineEvent::PROGRAM_XPHY,
      allStates,
      [this]() {
        // Make sure `programExternalPhyPorts` has never been called
        EXPECT_CALL(*transceiverManager_, programExternalPhyPorts(id_, false))
            .Times(0);
      } /* preUpdate */,
      []() {} /* verify */);
}

ACTION(ThrowFbossError) {
  throw FbossError("Mock FbossError");
}
TEST_F(TransceiverStateMachineTest, programXphyFailed) {
  std::set<TransceiverStateMachineState> stateSet = {
      TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED};
  // If programExternalPhyPorts() failed, state shouldn't change
  verifyStateUnchanged(
      TransceiverStateMachineEvent::PROGRAM_XPHY,
      stateSet,
      [this]() {
        EXPECT_CALL(*transceiverManager_, programExternalPhyPorts(id_, false))
            .Times(2)
            .WillOnce(ThrowFbossError());
      },
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        // Now isXphyProgrammed should still be false
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));

        // Then try again, it should succeed
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::PROGRAM_XPHY);

        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED);
        const auto& newStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(newStateMachine.get_attribute(isIphyProgrammed));
        // Now isXphyProgrammed should be true
        EXPECT_TRUE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(newStateMachine.get_attribute(isTransceiverProgrammed));
      });
}

TEST_F(TransceiverStateMachineTest, programTransceiver) {
  auto allStates = getAllStates();
  // Both IPHY_PORTS_PROGRAMMED and XPHY_PORTS_PROGRAMMED
  // can accept PROGRAM_TRANSCEIVER event
  verifyStateMachine(
      {TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED},
      TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER,
      TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED /* expected state */,
      allStates,
      [this]() { setProgramCmisModuleExpectation(true); },
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isTransceiverProgrammed should be true
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
        EXPECT_FALSE(stateMachine.get_attribute(needResetDataPath));
      },
      TransceiverType::MOCK_CMIS);
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER,
      allStates,
      [this]() { setProgramCmisModuleExpectation(false); },
      []() {} /* verify */,
      TransceiverType::MOCK_CMIS);
}

TEST_F(TransceiverStateMachineTest, programTransceiverFailed) {
  std::set<TransceiverStateMachineState> stateSet = {
      TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
      TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED};
  // If any function inside QsfpModule::programTransceiver() failed,
  // state shouldn't change
  verifyStateUnchanged(
      TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER,
      stateSet,
      [this]() {
        MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);

        // Mock throw exception on one of the functions in
        // QsfpModule::programTransceiver()
        EXPECT_CALL(
            *mockXcvr, customizeTransceiverLocked(cfg::PortSpeed::HUNDREDG))
            .Times(2)
            .WillOnce(ThrowFbossError());
        EXPECT_CALL(*mockXcvr, configureModule()).Times(1);

        // The rest functions are after customizeTransceiverLocked() and they
        // should only be called once at the second time
        const auto& info = transceiverManager_->getTransceiverInfo(id_);
        if (auto settings = info.settings()) {
          if (auto hostLaneSettings = settings->hostLaneSettings()) {
            EXPECT_CALL(
                *mockXcvr, ensureRxOutputSquelchEnabled(*hostLaneSettings))
                .Times(1);
          }
        }
        // 2 update qsfp data: one immediately after the successful
        // customizeTransceiverLocked(), the second one is before
        // updateCachedTransceiverInfoLocked() at the end
        EXPECT_CALL(*mockXcvr, updateQsfpData(true)).Times(1);
        ModuleStatus moduleStatus;
        EXPECT_CALL(*mockXcvr, updateCachedTransceiverInfoLocked(moduleStatus))
            .Times(1);
        EXPECT_CALL(*mockXcvr, updateQsfpData(false)).Times(1);
        // Normal transceiver programming shouldn't trigger resetDataPath()
        EXPECT_CALL(*mockXcvr, resetDataPath()).Times(0);
      },
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        // Now isTransceiverProgrammed should still be false
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));

        // Then try again, it should succeed
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER);

        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
        const auto& newStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(newStateMachine.get_attribute(isIphyProgrammed));
        // Now isTransceiverProgrammed should be true
        EXPECT_TRUE(newStateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(needResetDataPath));
      },
      TransceiverType::MOCK_CMIS);
}

TEST_F(TransceiverStateMachineTest, portUp) {
  auto allStates = getAllStates();
  // Only TRANSCEIVER_PROGRAMMED and INACTIVE can accept PORT_UP event
  verifyStateMachine(
      {TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::INACTIVE},
      TransceiverStateMachineEvent::PORT_UP,
      TransceiverStateMachineState::ACTIVE /* expected state */,
      allStates,
      []() {},
      [this]() {
        // Enter ACTIVE will also set `needMarkLastDownTime` to true
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
      });
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      TransceiverStateMachineEvent::PORT_UP,
      allStates,
      []() {} /* preUpdate */,
      []() {} /* verify */);
}

TEST_F(TransceiverStateMachineTest, portDown) {
  auto allStates = getAllStates();
  time_t beforeStateChangedTime{0};
  // Only TRANSCEIVER_PROGRAMMED and ACTIVE can accept ALL_PORTS_DOWN event
  verifyStateMachine(
      {TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::ACTIVE},
      TransceiverStateMachineEvent::ALL_PORTS_DOWN,
      TransceiverStateMachineState::INACTIVE /* expected state */,
      allStates,
      [&beforeStateChangedTime]() {
        beforeStateChangedTime = std::time(nullptr);
        // Sleep 1s to avoid the state machine handling the event too fast
        /* sleep override */
        sleep(1);
      },
      [this, &beforeStateChangedTime]() {
        // Enter INACTIVE will also mark last down time
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(needMarkLastDownTime));
        // Check the lastDownTime should be updated
        EXPECT_GT(
            transceiverManager_->getLastDownTime(id_), beforeStateChangedTime);
      });
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      TransceiverStateMachineEvent::ALL_PORTS_DOWN,
      allStates,
      []() {} /* preUpdate */,
      []() {} /* verify */);
}

TEST_F(TransceiverStateMachineTest, removeTransceiver) {
  auto allStates = getAllStates();
  // All states besides NOT_PRESENT can accept REMOVE_TRANSCEIVER event
  // Only PRESENT and DISCOVERED don't need to check all ports down, since
  // nothing is actually programmed yet.
  // And INACTIVE also means ports are down so it should pass
  verifyStateMachine(
      {TransceiverStateMachineState::PRESENT,
       TransceiverStateMachineState::DISCOVERED,
       TransceiverStateMachineState::INACTIVE},
      TransceiverStateMachineEvent::REMOVE_TRANSCEIVER,
      TransceiverStateMachineState::NOT_PRESENT /* expected state */,
      allStates,
      []() {} /* preUpdate */,
      [this]() { verifyResetProgrammingAttributes(); });

  // Other after programming states need to make sure all ports down before
  // removing such transceiver in case some transient i2c issue causes we
  // can't detect a transceiver and accidentally remove it
  verifyStateMachine(
      {TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::ACTIVE},
      TransceiverStateMachineEvent::REMOVE_TRANSCEIVER,
      TransceiverStateMachineState::NOT_PRESENT /* expected state */,
      allStates,
      [this]() {
        // Set port status to DOWN so that we can remove the transceiver
        // correctly
        updateTransceiverActiveState(false /* up */, true /* enabled */);
      },
      [this]() { verifyResetProgrammingAttributes(); });

  // Only NOT_PRESENT left
  EXPECT_EQ(allStates.size(), 1);
  EXPECT_TRUE(
      allStates.find(TransceiverStateMachineState::NOT_PRESENT) !=
      allStates.end());
  verifyStateUnchanged(
      TransceiverStateMachineEvent::REMOVE_TRANSCEIVER,
      allStates,
      []() {} /* preUpdate */,
      []() {} /* verify */);
}

TEST_F(TransceiverStateMachineTest, removeTransceiverFailed) {
  std::set<TransceiverStateMachineState> stateSet = {
      TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED,
      TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
      TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
      TransceiverStateMachineState::ACTIVE};
  // If never set port state to down, we can't change the state to NOT_PRESENT
  verifyStateUnchanged(
      TransceiverStateMachineEvent::REMOVE_TRANSCEIVER,
      stateSet,
      []() {} /* preUpdate */,
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isIphyProgrammed should still be true
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        const auto& programmedIphyPorts =
            transceiverManager_->getProgrammedIphyPortToPortInfo(id_);
        EXPECT_EQ(programmedIphyPorts.size(), 1);

        // Set port status to DOWN so that we can remove the transceiver
        // correctly
        updateTransceiverActiveState(false /* up */, true /* enabled */);
        // Then try again, it should succeed
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::REMOVE_TRANSCEIVER);

        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::NOT_PRESENT);
        verifyResetProgrammingAttributes();
      });
}

TEST_F(TransceiverStateMachineTest, remediateCmisTransceiver) {
  enableRemediationTesting();
  auto allStates = getAllStates();
  // Only INACTIVE can accept REMEDIATE_TRANSCEIVER event
  verifyStateMachine(
      {TransceiverStateMachineState::INACTIVE},
      TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED /* expected state */,
      allStates,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        // We trigger a hard reset as remediation
        MockTransceiverPlatformApi* xcvrApi =
            static_cast<MockTransceiverPlatformApi*>(
                transceiverManager_->getQsfpPlatformApi());
        EXPECT_CALL(*xcvrApi, triggerQsfpHardReset(id_ + 1)).Times(1);
      },
      [this]() { triggerRemediateEvents(); },
      [this]() {
        // Just finished remediation, so the dirty flag should be set
        EXPECT_TRUE(xcvr_->getDirty_());
        // state machine goes back to XPHY_PORTS_PROGRAMMED so that we can
        // reprogram the xcvr again
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        // Trigger a program transceiver which should fail because the cache is
        // still dirty
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER);
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        // Refresh the state machine again which will first do a successful
        // refresh and clear the dirty_ flag and then trigger
        // PROGRAM_TRANSCEIVER
        transceiverManager_->refreshStateMachines();
        EXPECT_FALSE(xcvr_->getDirty_());
        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
        const auto& newStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isTransceiverProgrammed should be true
        EXPECT_TRUE(newStateMachine.get_attribute(isTransceiverProgrammed));
      },
      TransceiverType::MOCK_CMIS,
      kRemediateTransceiverFnStr);
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      allStates,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        // Make sure no triggerQsfpHardReset() has been called
        MockTransceiverPlatformApi* xcvrApi =
            static_cast<MockTransceiverPlatformApi*>(
                transceiverManager_->getQsfpPlatformApi());
        EXPECT_CALL(*xcvrApi, triggerQsfpHardReset(id_ + 1)).Times(0);
      } /* preUpdate */,
      [this]() { triggerRemediateEvents(); },
      []() {} /* verify */,
      TransceiverType::MOCK_CMIS,
      kRemediateTransceiverFnStr);
}

TEST_F(TransceiverStateMachineTest, remediateSff8472Transceiver) {
  enableRemediationTesting();
  auto allStates = getAllStates();
  // Only INACTIVE can accept REMEDIATE_TRANSCEIVER event
  verifyStateMachine(
      {TransceiverStateMachineState::INACTIVE},
      TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED /* expected state */,
      allStates,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        // We trigger a hard reset as remediation
        MockTransceiverPlatformApi* xcvrApi =
            static_cast<MockTransceiverPlatformApi*>(
                transceiverManager_->getQsfpPlatformApi());
        EXPECT_CALL(*xcvrApi, triggerQsfpHardReset(id_ + 1)).Times(1);
      },
      [this]() { triggerRemediateEvents(); },
      [this]() {
        // Just finished remediation, so the dirty flag should be set
        EXPECT_TRUE(xcvr_->getDirty_());
        // state machine goes back to XPHY_PORTS_PROGRAMMED so that we can
        // reprogram the xcvr again
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        // Trigger a program transceiver which should fail because the cache is
        // still dirty
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER);
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        // Refresh the state machine again which will first do a successful
        // refresh and clear the dirty_ flag and then trigger
        // PROGRAM_TRANSCEIVER
        transceiverManager_->refreshStateMachines();
        EXPECT_FALSE(xcvr_->getDirty_());
        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
        const auto& newStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isTransceiverProgrammed should be true
        EXPECT_TRUE(newStateMachine.get_attribute(isTransceiverProgrammed));
      },
      TransceiverType::MOCK_SFF8472,
      kRemediateTransceiverFnStr);
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      allStates,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        // Make sure no triggerQsfpHardReset() has been called
        MockTransceiverPlatformApi* xcvrApi =
            static_cast<MockTransceiverPlatformApi*>(
                transceiverManager_->getQsfpPlatformApi());
        EXPECT_CALL(*xcvrApi, triggerQsfpHardReset(id_ + 1)).Times(0);
      } /* preUpdate */,
      [this]() { triggerRemediateEvents(); },
      []() {} /* verify */,
      TransceiverType::MOCK_SFF8472,
      kRemediateTransceiverFnStr);
}

TEST_F(TransceiverStateMachineTest, remediateCmisTransceiverFailed) {
  enableRemediationTesting();
  std::set<TransceiverStateMachineState> stateSet = {
      TransceiverStateMachineState::INACTIVE};
  // If triggerQsfpHardReset() failed, state shouldn't change
  verifyStateUnchanged(
      stateSet,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        MockTransceiverPlatformApi* xcvrApi =
            static_cast<MockTransceiverPlatformApi*>(
                transceiverManager_->getQsfpPlatformApi());
        EXPECT_CALL(*xcvrApi, triggerQsfpHardReset(id_ + 1))
            .Times(2)
            .WillOnce(ThrowFbossError());

        MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
        ::testing::Sequence s;
        // Expect updateQsfpData and updateCachedTransceiverInfoLocked to be
        // called from refreshStateMachines() we do in verify() below
        EXPECT_CALL(*mockXcvr, updateQsfpData(true)).Times(1).InSequence(s);
        EXPECT_CALL(*mockXcvr, updateQsfpData(false)).Times(1).InSequence(s);
        EXPECT_CALL(*mockXcvr, updateCachedTransceiverInfoLocked(::testing::_))
            .Times(1)
            .InSequence(s);
        setProgramCmisModuleExpectation(true, s);
      },
      [this]() { triggerRemediateEvents(); },
      [this]() {
        // The earlier remediation failed, so the dirty flag shouldn't be set
        EXPECT_FALSE(xcvr_->getDirty_());
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(isTransceiverProgrammed));

        // Then try again, it should succeed
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::REMEDIATE_TRANSCEIVER);
        // Since the remediation failed, the dirty flag should be set now
        EXPECT_TRUE(xcvr_->getDirty_());
        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED);
        const auto& afterRemediationStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(afterRemediationStateMachine.get_attribute(
            isTransceiverProgrammed));

        // Refresh the state machine again which will first do a successful
        // refresh and clear the dirty_ flag and then trigger
        // PROGRAM_TRANSCEIVER
        transceiverManager_->refreshStateMachines();
        EXPECT_FALSE(xcvr_->getDirty_());
        const auto& afterProgrammingStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(afterProgrammingStateMachine.get_attribute(
            isTransceiverProgrammed));
      },
      TransceiverType::MOCK_CMIS,
      kRemediateTransceiverFnStr);
}

TEST_F(TransceiverStateMachineTest, remediateSffTransceiver) {
  enableRemediationTesting();
  auto allStates = getAllStates();
  // Only INACTIVE can accept REMEDIATE_TRANSCEIVER event
  verifyStateMachine(
      {TransceiverStateMachineState::INACTIVE},
      TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED /* expected state */,
      allStates,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        MockSffModule* mockXcvr = static_cast<MockSffModule*>(xcvr_);
        ::testing::InSequence s;
        EXPECT_CALL(*mockXcvr, resetLowPowerMode()).Times(1);
        EXPECT_CALL(*mockXcvr, ensureTxEnabled()).Times(1);
      },
      [this]() { triggerRemediateEvents(); },
      [this]() {
        // Just finished remediation, so the dirty flag should be set
        EXPECT_TRUE(xcvr_->getDirty_());
        // state machine goes back to XPHY_PORTS_PROGRAMMED so that we can
        // reprogram the xcvr again
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        // Trigger a program transceiver which should fail because the cache is
        // still dirty
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER);
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        // Refresh the state machine again which will first do a successful
        // refresh and clear the dirty_ flag and then trigger
        // PROGRAM_TRANSCEIVER
        transceiverManager_->refreshStateMachines();
        EXPECT_FALSE(xcvr_->getDirty_());
        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
        const auto& newStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        // Now isTransceiverProgrammed should be true
        EXPECT_TRUE(newStateMachine.get_attribute(isTransceiverProgrammed));
      },
      TransceiverType::MOCK_SFF,
      kRemediateTransceiverFnStr);
  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      allStates,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        MockSffModule* mockXcvr = static_cast<MockSffModule*>(xcvr_);
        ::testing::InSequence s;
        EXPECT_CALL(*mockXcvr, resetLowPowerMode()).Times(0);
        EXPECT_CALL(*mockXcvr, ensureTxEnabled()).Times(0);
      } /* preUpdate */,
      [this]() { triggerRemediateEvents(); },
      []() {} /* verify */,
      TransceiverType::MOCK_SFF,
      kRemediateTransceiverFnStr);
}

TEST_F(TransceiverStateMachineTest, remediateSffTransceiverFailed) {
  enableRemediationTesting();
  std::set<TransceiverStateMachineState> stateSet = {
      TransceiverStateMachineState::INACTIVE};
  // If resetLowPowerMode() failed, state shouldn't change
  verifyStateUnchanged(
      stateSet,
      [this]() {
        // Before trigger remediation, remove remediation pause
        transceiverManager_->setPauseRemediation(0, nullptr);
        // Give 1s buffer when comparing lastDownTime_ in shouldRemediate()
        /* sleep override */
        sleep(1);

        MockSffModule* mockXcvr = static_cast<MockSffModule*>(xcvr_);
        ::testing::InSequence s;
        EXPECT_CALL(*mockXcvr, resetLowPowerMode())
            .Times(2)
            .WillOnce(ThrowFbossError());
        EXPECT_CALL(*mockXcvr, ensureTxEnabled()).Times(1);
      },
      [this]() { triggerRemediateEvents(); },
      [this]() {
        // The earlier remediation failed, so the dirty flag should not be set
        EXPECT_FALSE(xcvr_->getDirty_());
        // state machine goes back to XPHY_PORTS_PROGRAMMED so that we can
        // reprogram the xcvr again
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(isTransceiverProgrammed));

        // Then try again, it should succeed
        transceiverManager_->updateStateBlocking(
            id_, TransceiverStateMachineEvent::REMEDIATE_TRANSCEIVER);
        EXPECT_TRUE(xcvr_->getDirty_());
        EXPECT_EQ(
            transceiverManager_->getCurrentState(id_),
            TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED);
        const auto& afterRemediationStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(afterRemediationStateMachine.get_attribute(
            isTransceiverProgrammed));

        // Refresh the state machine again which will first do a successful
        // refresh and clear the dirty_ flag and then trigger
        // PROGRAM_TRANSCEIVER
        transceiverManager_->refreshStateMachines();
        EXPECT_FALSE(xcvr_->getDirty_());
        const auto& afterProgrammingStateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_TRUE(afterProgrammingStateMachine.get_attribute(
            isTransceiverProgrammed));
      },
      TransceiverType::MOCK_SFF,
      kRemediateTransceiverFnStr);
}

TEST_F(TransceiverStateMachineTest, agentConfigChangedWarmBoot) {
  auto allStates = getAllStates();
  // Instead of using transceiverManager_->updateStateBlocking(), use
  // triggerAgentConfigChangeEvent() so that we can verify the logic inside this
  // function too
  // After programmed state can accept RESET_TO_DISCOVERED event
  verifyStateMachine(
      {TransceiverStateMachineState::ACTIVE,
       TransceiverStateMachineState::INACTIVE,
       TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED},
      TransceiverStateMachineState::DISCOVERED /* expected state */,
      allStates,
      []() {} /* preUpdate */,
      [this]() { triggerAgentConfigChanged(false); } /* stateUpdate */,
      [this]() {
        // Enter DISCOVERED will also call `resetProgrammingAttributes`
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
      } /* verify */,
      TransceiverType::CMIS,
      kWarmBootAgentConfigChangedFnStr);

  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      allStates,
      []() {} /* preUpdate */,
      [this]() { triggerAgentConfigChanged(false); } /* stateUpdate */,
      []() {} /* verify */,
      TransceiverType::CMIS,
      kWarmBootAgentConfigChangedFnStr);
}

TEST_F(TransceiverStateMachineTest, agentConfigChangedColdBoot) {
  auto allStates = getAllStates();
  // Instead of using transceiverManager_->updateStateBlocking(), use
  // triggerAgentConfigChangeEvent() so that we can verify the logic inside this
  // function too
  // After programmed state can accept RESET_TO_DISCOVERED event
  verifyStateMachine(
      {TransceiverStateMachineState::ACTIVE,
       TransceiverStateMachineState::INACTIVE,
       TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED},
      TransceiverStateMachineState::DISCOVERED /* expected state */,
      allStates,
      [this]() {
        MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
        EXPECT_CALL(*mockXcvr, resetDataPath()).Times(1);
      } /* preUpdate */,
      [this]() { triggerAgentConfigChanged(true); } /* stateUpdate */,
      [this]() {
        // Enter DISCOVERED will also call `resetProgrammingAttributes`
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
        // Cold boot agent will need a reset data path for CMIS module
        EXPECT_TRUE(stateMachine.get_attribute(needResetDataPath));

        // refresh twice state machine so that we can finish programming xcvr
        transceiverManager_->refreshStateMachines();
        transceiverManager_->refreshStateMachines();
        EXPECT_FALSE(stateMachine.get_attribute(needResetDataPath));
      } /* verify */,
      TransceiverType::MOCK_CMIS,
      kColdBootAgentConfigChangedFnStr);

  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      allStates,
      []() {} /* preUpdate */,
      [this]() { triggerAgentConfigChanged(true); } /* stateUpdate */,
      []() {} /* verify */,
      TransceiverType::MOCK_CMIS,
      kColdBootAgentConfigChangedFnStr);
}

TEST_F(TransceiverStateMachineTest, agentConfigChangedWarmBootOnAbsentXcvr) {
  auto allStates = getAllStates();
  verifyStateMachine(
      {TransceiverStateMachineState::ACTIVE,
       TransceiverStateMachineState::INACTIVE,
       TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED},
      TransceiverStateMachineState::NOT_PRESENT /* expected state */,
      allStates,
      [this]() {
        setMockCmisPresence(false);
        xcvr_->detectPresence();
      } /* preUpdate */,
      [this]() { triggerAgentConfigChanged(false); } /* stateUpdate */,
      [this]() {
        // Enter DISCOVERED will also call `resetProgrammingAttributes`
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
      } /* verify */,
      TransceiverType::MOCK_CMIS,
      kWarmBootAgentConfigChangedFnStr);

  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      allStates,
      [this]() {
        setMockCmisPresence(false);
        xcvr_->detectPresence();
      } /* preUpdate */,
      [this]() { triggerAgentConfigChanged(false); } /* stateUpdate */,
      []() {} /* verify */,
      TransceiverType::MOCK_CMIS,
      kWarmBootAgentConfigChangedFnStr);
}

TEST_F(TransceiverStateMachineTest, agentConfigChangedColdBootOnAbsentXcvr) {
  auto allStates = getAllStates();
  // Use ABSENT_XCVR for such transceiver to make sure state can go back to
  // NOT_PRESENT
  verifyStateMachine(
      {TransceiverStateMachineState::ACTIVE,
       TransceiverStateMachineState::INACTIVE,
       TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
       TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
       TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED},
      TransceiverStateMachineState::NOT_PRESENT /* expected state */,
      allStates,
      [this]() {
        setMockCmisPresence(false);
        xcvr_->detectPresence();

        // Even though agent cold boot, because transceiver is absent,
        // resetDataPath() won't be called
        MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
        EXPECT_CALL(*mockXcvr, resetDataPath()).Times(0);
      } /* preUpdate */,
      [this]() { triggerAgentConfigChanged(true); } /* stateUpdate */,
      [this]() {
        // Enter DISCOVERED will also call `resetProgrammingAttributes`
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(id_);
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
        // Cold boot agent will need a reset data path for CMIS module
        EXPECT_TRUE(stateMachine.get_attribute(needResetDataPath));

        // refresh twice state machine so that we can finish programming xcvr
        transceiverManager_->refreshStateMachines();
        transceiverManager_->refreshStateMachines();
        EXPECT_FALSE(stateMachine.get_attribute(needResetDataPath));
      } /* verify */,
      TransceiverType::MOCK_CMIS,
      kColdBootAgentConfigChangedFnStr);

  // Other states should not change even though we try to process the event
  verifyStateUnchanged(
      allStates,
      [this]() {
        setMockCmisPresence(false);
        xcvr_->detectPresence();
      } /* preUpdate */,
      [this]() { triggerAgentConfigChanged(true); } /* stateUpdate */,
      []() {} /* verify */,
      TransceiverType::MOCK_CMIS,
      kColdBootAgentConfigChangedFnStr);
}

TEST_F(TransceiverStateMachineTest, syncPortsOnRemovedTransceiver) {
  std::set<TransceiverStateMachineState> stateSet = {
      TransceiverStateMachineState::ACTIVE};
  verifyStateMachine(
      {TransceiverStateMachineState::ACTIVE},
      TransceiverStateMachineState::NOT_PRESENT,
      stateSet,
      [this]() {
        setMockCmisPresence(false);

        MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
        ::testing::Sequence s;
        // The first refreshLocked() should detect transceiver is removed
        // and dirty, need to updateQsfpData fully
        EXPECT_CALL(*mockXcvr, ensureOutOfReset());
        EXPECT_CALL(*mockXcvr, updateQsfpData(true));
        // Because transceiver is absent, we should use default ModuleStatus
        // to update cached transceiver info
        ModuleStatus moduleStatus;
        EXPECT_CALL(*mockXcvr, updateCachedTransceiverInfoLocked(moduleStatus));
      } /* preUpdate */,
      [this]() {
        // Trigger active state change function just like wedge_agent calls
        // qsfp_service syncPorts(). Bring down the ports
        updateTransceiverActiveState(false /* up */, true /* enabled */);
        // The refresh() will let TransceiverStateMachine trigger next event
        xcvr_->refresh();
      } /* stateUpdate */,
      [this]() {
        // The TransceiverStateMachine should trigger a remove event during
        // refresh(). Now check all programming attributes being removed.
        verifyResetProgrammingAttributes();
      } /* verify */,
      TransceiverType::MOCK_CMIS,
      "Triggering syncPorts with port down on a removed transceiver");
}

TEST_F(TransceiverStateMachineTest, reseatTransceiver) {
  auto removeCmisTransceiver = [this](bool isRemoval) {
    if (isRemoval) {
      XLOG(INFO) << "Verifying Removing Transceiver: " << id_;
      transceiverManager_->overrideMgmtInterface(
          static_cast<int>(id_) + 1,
          uint8_t(TransceiverModuleIdentifier::UNKNOWN));
      transceiverManager_->overrideTransceiverForTesting(id_, nullptr);
    } else {
      XLOG(INFO) << "Verifying Inserting new CMIS Transceiver: " << id_;
      // Mimic a SFF module -> CMIS module replacement
      transceiverManager_->overrideMgmtInterface(
          static_cast<int>(id_) + 1,
          uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS));
      auto xcvrImpl = std::make_unique<MockCmisTransceiverImpl>(id_);
      EXPECT_CALL(*xcvrImpl.get(), detectTransceiver())
          .WillRepeatedly(::testing::Return(true));
      xcvr_ = static_cast<QsfpModule*>(
          transceiverManager_->overrideTransceiverForTesting(
              id_,
              std::make_unique<MockCmisModule>(
                  transceiverManager_.get(), std::move(xcvrImpl))));
    }

    // Check this refreshStateMachines will:
    // 1) If it's a removal operation, such transceiver will be removed and the
    // state machine will be set back to NOT_PRESENT. Example log: P501158293
    // 2) If it's a new transceiver inserted, a new transceiver will be detected
    transceiverManager_->refreshStateMachines();
    const auto& xcvrInfo = transceiverManager_->getTransceiverInfo(id_);
    EXPECT_EQ(*xcvrInfo.present(), !isRemoval);
    // verify that getTransceiverInfo properly returns a timestamp when
    // the transceiver isn't present
    if (isRemoval) {
      EXPECT_GT(*xcvrInfo.timeCollected(), 0);
    }

    // Both cases will kick off a PROGRAM_IPHY event
    EXPECT_EQ(
        transceiverManager_->getCurrentState(id_),
        TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED);
    // Now only isIphyProgrammed should be true
    const auto& stateMachine =
        transceiverManager_->getStateMachineForTesting(id_);
    EXPECT_TRUE(stateMachine.get_attribute(isIphyProgrammed));
    EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
    EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));

    // Use updateStateBlocking() to skip PhyManager check
    // Make sure `programExternalPhyPorts` has been called
    EXPECT_CALL(*transceiverManager_, programExternalPhyPorts(id_, false))
        .Times(1);
    transceiverManager_->updateStateBlocking(
        id_, TransceiverStateMachineEvent::PROGRAM_XPHY);
    EXPECT_EQ(
        transceiverManager_->getCurrentState(id_),
        TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED);
    // Now isXphyProgrammed should also be true
    const auto& stateMachine2 =
        transceiverManager_->getStateMachineForTesting(id_);
    EXPECT_TRUE(stateMachine2.get_attribute(isIphyProgrammed));
    EXPECT_TRUE(stateMachine2.get_attribute(isXphyProgrammed));
    EXPECT_FALSE(stateMachine2.get_attribute(isTransceiverProgrammed));

    // Because the transceiver is not present, the PROGRAM_TRANSCEIVER should be
    // no-op
    if (!isRemoval) {
      // When transceiver is present, do a successful refresh to make the cache
      // valid
      MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
      ::testing::Sequence s;
      EXPECT_CALL(*mockXcvr, updateQsfpData(false)).Times(1).InSequence(s);
      EXPECT_CALL(*mockXcvr, updateCachedTransceiverInfoLocked(::testing::_))
          .Times(1)
          .InSequence(s);
      setProgramCmisModuleExpectation(true);
    }
    // Refresh the state machine again which will first do a successful
    // refresh and clear the dirty_ flag and then trigger
    // PROGRAM_TRANSCEIVER
    transceiverManager_->refreshStateMachines();
    if (!isRemoval) {
      // Only check when xcvr is there
      EXPECT_FALSE(xcvr_->getDirty_());
    }
    EXPECT_EQ(
        transceiverManager_->getCurrentState(id_),
        TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED);
    // Now all programming attributes should be true
    const auto& stateMachine3 =
        transceiverManager_->getStateMachineForTesting(id_);
    EXPECT_TRUE(stateMachine3.get_attribute(isIphyProgrammed));
    EXPECT_TRUE(stateMachine3.get_attribute(isXphyProgrammed));
    EXPECT_TRUE(stateMachine3.get_attribute(isTransceiverProgrammed));
  };

  // Due to this test might need several updates, which are first removing the
  // transceiver and then inserting back, instead of using the general
  // verifyStateMachine(), we decided to break the whole tests into multiple
  // steps.
  // Step1: Mimic agent side ports down
  xcvr_ = overrideTransceiver(TransceiverType::MOCK_SFF);
  setState(TransceiverStateMachineState::INACTIVE);

  // Step2: Remove the transceiver and check the state machine updates
  removeCmisTransceiver(true);

  // Step3: Insert the transceiver back and call refreshStateMachine()
  removeCmisTransceiver(false);
}
} // namespace facebook::fboss
