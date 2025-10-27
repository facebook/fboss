/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/*
 * PortStateMachineTest will exist alongside TransceiverStateMachineTest until
 * the PortManager migration is complete. The tests here will be used to
 * validate state machine transitions in Port Manager mode only. Any
 * transceiver-specific (e.g. SFF / CMIS) testing will be kept in
 * TransceiverStateMachineTest.cpp until we consolidate the files.
 */

#include "fboss/qsfp_service/test/MockPhyManager.h"
#include "fboss/qsfp_service/test/MockPortManager.h"

#include "fboss/qsfp_service/test/MockManagerConstructorArgs.h"
#include "fboss/qsfp_service/test/MockTransceivers.h"
#include "fboss/qsfp_service/test/TransceiverManagerTestHelper.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

class PortStateMachineTest : public TransceiverManagerTestHelper {
  // <Port(1) State, Port(3) States>
  using PortStates =
      std::pair<PortStateMachineState, std::optional<PortStateMachineState>>;

 public:
  using TcvrPortStatePair = std::pair<TransceiverStateMachineState, PortStates>;
  void SetUp() override {
    TransceiverManagerTestHelper::SetUp();
    // Setup GFlags for Full Port Manager Functionality
    gflags::SetCommandLineOptionWithMode(
        "port_manager_mode", "t", gflags::SET_FLAGS_DEFAULT);

    resetManagers();
  }

  void setMockCmisPresence(bool isPresent) {
    MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
    auto xcvrImpl = mockXcvr->getTransceiverImpl();
    EXPECT_CALL(*xcvrImpl, detectTransceiver())
        .WillRepeatedly(::testing::Return(isPresent));
  }

  void setMockCmisTransceiverReady(bool isReady) {
    MockCmisModule* mockXcvr = static_cast<MockCmisModule*>(xcvr_);
    EXPECT_CALL(*mockXcvr, ensureTransceiverReadyLocked())
        .WillRepeatedly(::testing::Return(isReady));
  }

  // We default to CMIS module for testing.
  QsfpModule* overrideTransceiver(bool multiPort, bool isMock = false) {
    // Set port status to DOWN so that we can remove the transceiver correctly
    auto overrideCmisModule = [this, multiPort, isMock]() {
      // This override function use ids starting from 1
      transceiverManager_->overrideMgmtInterface(
          static_cast<int>(tcvrId_) + 1,
          uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS));
      if (isMock) {
        XLOG(INFO) << "Making Mock CMIS QSFP for " << tcvrId_;
        cmisQsfpImpls_.push_back(
            std::make_unique<MockCmisTransceiverImpl>(
                tcvrId_, transceiverManager_.get()));
        EXPECT_CALL(*cmisQsfpImpls_.back().get(), detectTransceiver())
            .WillRepeatedly(::testing::Return(true));
        return transceiverManager_->overrideTransceiverForTesting(
            tcvrId_,
            std::make_unique<MockCmisModule>(
                transceiverManager_->getPortNames(tcvrId_),
                cmisQsfpImpls_.back().get(),
                tcvrConfig_,
                transceiverManager_->getTransceiverName(tcvrId_)));
      } else {
        XLOG(INFO) << "Making CMIS QSFP for " << tcvrId_;
        std::unique_ptr<FakeTransceiverImpl> xcvrImpl;
        if (multiPort) {
          qsfpImpls_.push_back(
              std::make_unique<Cmis400GFr4MultiPortTransceiver>(
                  tcvrId_, transceiverManager_.get()));
        } else {
          qsfpImpls_.push_back(
              std::make_unique<Cmis200GTransceiver>(
                  tcvrId_, transceiverManager_.get()));
        }
        return transceiverManager_->overrideTransceiverForTesting(
            tcvrId_,
            std::make_unique<CmisModule>(
                transceiverManager_->getPortNames(tcvrId_),
                qsfpImpls_.back().get(),
                tcvrConfig_,
                true /*supportRemediate*/,
                transceiverManager_->getTransceiverName(tcvrId_)));
      }
    };

    Transceiver* xcvr = overrideCmisModule();
    EXPECT_TRUE(xcvr->getLastDownTime() != 0);

    // Remove the override config we set before
    transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
        emptyOverrideTcvrToPortAndProfile_);

    auto qsfpModuleXcvr = static_cast<QsfpModule*>(xcvr);
    // Assign a default transceiver info so that we can set expectations based
    // on it even when transceiverInfo hasn't been updated for the first time
    *(qsfpModuleXcvr->info_.wlock()) = TransceiverInfo{};
    return qsfpModuleXcvr;
  }

  void resetManagers() {
    // Create Platform Mapping Object
    const auto platformMapping =
        makeFakePlatformMapping(numModules, numPortsPerModule);
    const std::shared_ptr<const PlatformMapping> castedPlatformMapping =
        platformMapping;

    // Create Threads Object
    const auto threadsMap = makeSlotThreadHelper(platformMapping);

    // Create PhyManager Object
    std::unique_ptr<MockPhyManager> phyManager =
        std::make_unique<MockPhyManager>(platformMapping.get());
    phyManager_ = phyManager.get();

    // Create Transceiver Manager
    transceiverManager_ = std::make_unique<MockWedgeManager>(
        numModules, numPortsPerModule, platformMapping, threadsMap);

    // Create Port Manager
    portManager_ = std::make_unique<MockPortManager>(
        transceiverManager_.get(),
        std::move(phyManager),
        castedPlatformMapping,
        threadsMap);
  }

  void initManagers() {
    transceiverManager_->init();
    portManager_->init();
  }

  void initializePortsThroughRefresh() {
    transceiverManager_->refreshTransceivers();
    portManager_->updateTransceiverPortStatus();
  }

  void disablePortsThroughRefresh() {
    portManager_->setOverrideAgentPortStatusForTesting({}, {});
    transceiverManager_->refreshTransceivers();
    portManager_->updateTransceiverPortStatus();
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
    portManager_->setOverrideAgentConfigAppliedInfoForTesting(
        configAppliedInfo);

    portManager_->triggerAgentConfigChangeEvent();
  }

  void refreshAndTriggerProgramming() {
    transceiverManager_->refreshTransceivers();
    portManager_->updateTransceiverPortStatus();
    portManager_
        ->detectTransceiverDiscoveredAndReinitializeCorrespondingPorts();
    portManager_->triggerProgrammingEvents();
    transceiverManager_->triggerProgrammingEvents();
  }

  void setState(
      TransceiverStateMachineState tcvrState,
      PortStates portStates,
      bool multiPort) {
    if (multiPort) {
      transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
          overrideMultiPortTcvrToPortAndProfile_);
      portManager_->setOverrideAgentPortStatusForTesting(
          {portId1_, portId3_}, {portId1_, portId3_});
    } else {
      transceiverManager_->setOverrideTcvrToPortAndProfileForTesting(
          overrideTcvrToPortAndProfile_);
      portManager_->setOverrideAgentPortStatusForTesting(
          {portId1_}, {portId1_});
    }
    if (tcvrState == TransceiverStateMachineState::DISCOVERED &&
        portStates.first == PortStateMachineState::INITIALIZED &&
        portStates.second ==
            optionalPortState(multiPort, PortStateMachineState::INITIALIZED)) {
      initializePortsThroughRefresh();
    } else if (
        tcvrState == TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED &&
        portStates.first == PortStateMachineState::PORT_UP &&
        portStates.second ==
            optionalPortState(multiPort, PortStateMachineState::PORT_UP)) {
      for (int i = 0; i < 5; ++i) {
        refreshAndTriggerProgramming();
      }
    }

    XLOG(INFO) << "setState changed state to "
               << logState(getCurrentState(multiPort));
  }

  bool isMultiPort(const TcvrPortStatePair& tcvrPortStatePair) {
    return tcvrPortStatePair.second.second.has_value();
  }

  std::string logState(const TcvrPortStatePair& tcvrPortStatePair) {
    return "tcvrState=" +
        apache::thrift::util::enumNameSafe(tcvrPortStatePair.first) +
        ", port1State=" +
        apache::thrift::util::enumNameSafe(tcvrPortStatePair.second.first) +
        (tcvrPortStatePair.second.second.has_value()
             ? ", port3State=" +
                 apache::thrift::util::enumNameSafe(
                     tcvrPortStatePair.second.second.value())
             : "");
  }

  TcvrPortStatePair getCurrentState(bool multiPort) {
    return TcvrPortStatePair{
        transceiverManager_->getCurrentState(tcvrId_),
        {portManager_->getPortState(portId1_),
         optionalPortState(multiPort, portManager_->getPortState(portId3_))}};
  }

  std::optional<PortStateMachineState> optionalPortState(
      bool condition,
      PortStateMachineState state) {
    return condition ? std::optional{state} : std::nullopt;
  }

  void updateTransceiverActiveState(bool up, bool enabled, PortID portId) {
    PortStatus status;
    status.enabled() = enabled;
    status.up() = up;
    portManager_->updatePortActiveState({{portId, status}});
    // Sleep 1s to avoid the state machine handling the event too fast
    /* sleep override */
    sleep(1);
  }

  template <
      typename PRE_UPDATE_FN,
      typename STATE_UPDATE_FN,
      typename VERIFY_FN>
  void verifyStateMachine(
      const std::vector<TcvrPortStatePair>& supportedStates,
      TcvrPortStatePair expectedState,
      PRE_UPDATE_FN preUpdate,
      STATE_UPDATE_FN stateUpdate,
      VERIFY_FN verify,
      const std::string& updateStr,
      bool isMock = false) {
    for (const auto& tcvrPortStatePair : supportedStates) {
      auto multiPort = isMultiPort(tcvrPortStatePair);
      XLOG(INFO) << "Verifying Transceiver=0 state CHANGED by " << updateStr
                 << " from " << logState(tcvrPortStatePair) << " to "
                 << logState(expectedState);

      // Always create a new transceiver so that we can make sure the state
      // can go back to the beginning state
      xcvr_ = overrideTransceiver(multiPort, isMock);
      setState(tcvrPortStatePair.first, tcvrPortStatePair.second, multiPort);

      // Call preUpdate() before actual stateUpdate()
      preUpdate();

      // Trigger state update
      stateUpdate();

      // Check current state matches expected state
      auto curState = getCurrentState(multiPort);
      EXPECT_EQ(curState, expectedState)
          << "Transceiver=0 state doesn't match after " << updateStr
          << ", preState=" << logState(tcvrPortStatePair)
          << ", expected new state=" << logState(expectedState)
          << ", actual=" << logState(curState);

      // Verify the result after update finishes
      verify();

      ::testing::Mock::VerifyAndClearExpectations(transceiverManager_.get());
      ::testing::Mock::VerifyAndClearExpectations(xcvr_);
    }
  }

  template <typename PRE_UPDATE_FN, typename VERIFY_FN>
  void verifyStateMachine(
      const std::vector<TcvrPortStatePair>& supportedStates,
      TcvrPortStatePair expectedState,
      TransceiverStateMachineEvent event,
      PortStateMachineEvent portEvent1,
      std::optional<PortStateMachineEvent> portEvent3,
      PRE_UPDATE_FN preUpdate,
      VERIFY_FN verify,
      bool holdTransceiversLockWhileUpdating = false,
      bool isMock = false) {
    auto stateUpdateFn = [this,
                          event,
                          portEvent1,
                          portEvent3,
                          holdTransceiversLockWhileUpdating]() {
      if (holdTransceiversLockWhileUpdating) {
        auto lockedTransceivers =
            transceiverManager_->getSynchronizedTransceivers().rlock();
        transceiverManager_->updateStateBlocking(tcvrId_, event);
      } else {
        transceiverManager_->updateStateBlocking(tcvrId_, event);
      }
      portManager_->updateStateBlocking(portId1_, portEvent1);
      if (portEvent3.has_value()) {
        portManager_->updateStateBlocking(portId3_, portEvent3.value());
      }
    };
    auto stateUpdateFnStr = folly::to<std::string>(
        "Event=", apache::thrift::util::enumNameSafe(event));
    verifyStateMachine(
        supportedStates,
        expectedState,
        preUpdate,
        stateUpdateFn,
        verify,
        stateUpdateFnStr,
        isMock);
  }

  template <
      typename PRE_UPDATE_FN,
      typename STATE_UPDATE_FN,
      typename VERIFY_FN>
  void verifyStateUnchanged(
      const std::vector<TcvrPortStatePair>& supportedStates,
      PRE_UPDATE_FN preUpdate,
      STATE_UPDATE_FN stateUpdate,
      VERIFY_FN verify,
      const std::string& updateStr,
      bool isMock = false) {
    for (const auto& tcvrPortStatePair : supportedStates) {
      auto multiPort = isMultiPort(tcvrPortStatePair);
      XLOG(INFO) << "Verifying Transceiver=0 state UNCHANGED by " << updateStr
                 << " for " << logState(tcvrPortStatePair)
                 << ", multiPort=" << multiPort;
      // Always create a new transceiver so that we can make sure the state
      // can go back to the beginning state
      xcvr_ = overrideTransceiver(multiPort, isMock);
      setState(tcvrPortStatePair.first, tcvrPortStatePair.second, multiPort);

      // Call preUpdate() before actual stateUpdate()
      preUpdate();

      // Trigger state update
      stateUpdate();

      // Check current state matches original state (unchanged)
      auto curState = TcvrPortStatePair{
          transceiverManager_->getCurrentState(tcvrId_),
          {portManager_->getPortState(portId1_),
           optionalPortState(multiPort, portManager_->getPortState(portId3_))}};
      EXPECT_EQ(curState, tcvrPortStatePair)
          << "Transceiver=0 state changed unexpectedly after " << updateStr
          << ", expected unchanged state=" << logState(tcvrPortStatePair)
          << ", actual=" << logState(curState);

      // Verify the result after update finishes
      verify();

      ::testing::Mock::VerifyAndClearExpectations(transceiverManager_.get());
      ::testing::Mock::VerifyAndClearExpectations(xcvr_);
    }
  }

  template <typename PRE_UPDATE_FN, typename VERIFY_FN>
  void verifyStateUnchanged(
      TransceiverStateMachineEvent tcvrEvent,
      PortStateMachineEvent portEvent1,
      std::optional<PortStateMachineEvent> portEvent3,
      const std::vector<TcvrPortStatePair>& supportedStates,
      PRE_UPDATE_FN preUpdate,
      VERIFY_FN verify,
      bool isMock = false) {
    if (supportedStates.empty()) {
      return;
    }
    auto stateUpdateFn = [this, tcvrEvent, portEvent1, portEvent3]() {
      transceiverManager_->updateStateBlocking(tcvrId_, tcvrEvent);
      portManager_->updateStateBlocking(portId1_, portEvent1);
      if (portEvent3.has_value()) {
        portManager_->updateStateBlocking(portId3_, portEvent3.value());
      }
    };
    auto stateUpdateFnStr = folly::to<std::string>(
        "Event=", apache::thrift::util::enumNameSafe(tcvrEvent));
    verifyStateUnchanged(
        supportedStates,
        preUpdate,
        stateUpdateFn,
        verify,
        stateUpdateFnStr,
        isMock);
  }

  void verifyXphyNeedResetDataPath(bool multiPort, bool expected) {
    EXPECT_EQ(portManager_->getXphyNeedResetDataPath(portId1_), expected);
    if (multiPort) {
      EXPECT_EQ(portManager_->getXphyNeedResetDataPath(portId3_), expected);
    }
  }

  // Manager Attributes
  int numPortsPerModule{8};

  // Port Manager
  MockPhyManager* phyManager_{};
  std::unique_ptr<MockPortManager> portManager_;

  // Tcvr Data
  QsfpModule* xcvr_{};
  const TransceiverID tcvrId_ = TransceiverID(0);
  std::vector<std::unique_ptr<MockCmisTransceiverImpl>> cmisQsfpImpls_;

  // Port Data
  std::string kPortName1 = "eth1/1/1";
  std::string kPortName3 = "eth1/1/3";
  const PortID portId1_ = PortID(1);
  const PortID portId3_ = PortID(3);

  // Port Profiles
  const cfg::PortProfileID profile_ =
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL;
  const cfg::PortProfileID multiPortProfile_ =
      cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL;

  // TcvrToPortAndProfileOverrides
  const TransceiverManager::OverrideTcvrToPortAndProfile
      overrideTcvrToPortAndProfile_ = {
          {tcvrId_,
           {
               {portId1_, profile_},
           }}};
  const TransceiverManager::OverrideTcvrToPortAndProfile
      overrideMultiPortTcvrToPortAndProfile_ = {
          {tcvrId_,
           {
               {portId1_, multiPortProfile_},
               {portId3_, multiPortProfile_},
           }}};
  const TransceiverManager::OverrideTcvrToPortAndProfile
      emptyOverrideTcvrToPortAndProfile_ = {};
};

TEST_F(PortStateMachineTest, defaultState) {
  // At this point, refresh cycle hasn't been run yet, so everything should be
  // NOT_PRESENT / UNINITIALIZED.
  ASSERT_EQ(
      transceiverManager_->getCurrentState(tcvrId_),
      TransceiverStateMachineState::NOT_PRESENT);
  ASSERT_EQ(
      portManager_->getPortState(portId1_),
      PortStateMachineState::UNINITIALIZED);
  ASSERT_EQ(
      portManager_->getPortState(portId3_),
      PortStateMachineState::UNINITIALIZED);
}

TEST_F(PortStateMachineTest, defaultStateAfterInit) {
  // At this point, only refreshTransceivers has been run (as a part of
  // TransceiverManager->init()), so expect transceiver to be discovered and
  // ports to be uninitialized
  initManagers();
  ASSERT_EQ(
      transceiverManager_->getCurrentState(tcvrId_),
      TransceiverStateMachineState::DISCOVERED);
  ASSERT_EQ(
      portManager_->getPortState(portId1_),
      PortStateMachineState::UNINITIALIZED);
  ASSERT_EQ(
      portManager_->getPortState(portId3_),
      PortStateMachineState::UNINITIALIZED);
}

TEST_F(PortStateMachineTest, enablePorts) {
  for (auto multiPort : {false, true}) {
    initManagers();
    XLOG(INFO) << "Verifying portsInitialized for multiPort = " << multiPort;
    verifyStateMachine(
        {TcvrPortStatePair{
            TransceiverStateMachineState::NOT_PRESENT,
            {PortStateMachineState::UNINITIALIZED,
             optionalPortState(
                 multiPort, PortStateMachineState::UNINITIALIZED)}}},
        TcvrPortStatePair{
            TransceiverStateMachineState::DISCOVERED,
            {PortStateMachineState::INITIALIZED,
             optionalPortState(
                 multiPort,
                 PortStateMachineState::INITIALIZED)}} /* expected state */,
        []() {} /* preUpdate */,
        [this]() { initializePortsThroughRefresh(); } /* stateUpdate */,
        [this, multiPort]() {
          verifyXphyNeedResetDataPath(multiPort, false /* expected */);
        } /* verify */,
        "portsInitialized");
    // Prepare for testing with next multiPort value
    resetManagers();
  }
}

TEST_F(PortStateMachineTest, disablePorts) {
  for (auto multiPort : {false, true}) {
    initManagers();
    XLOG(INFO) << "Verifying disablePorts for multiPort = " << multiPort;
    verifyStateMachine(
        {TcvrPortStatePair{
            TransceiverStateMachineState::DISCOVERED,
            {PortStateMachineState::INITIALIZED,
             optionalPortState(
                 multiPort, PortStateMachineState::INITIALIZED)}}},
        TcvrPortStatePair{
            TransceiverStateMachineState::DISCOVERED,
            {PortStateMachineState::UNINITIALIZED,
             optionalPortState(
                 multiPort, PortStateMachineState::UNINITIALIZED)}} /* expected
                                                                  state */
        ,
        []() {} /* preUpdate */,
        [this]() { disablePortsThroughRefresh(); } /* stateUpdate */,
        []() {} /* verify */,
        "portsDisabled");
    // Prepare for testing with next multiPort value
    resetManagers();
  }
}

TEST_F(
    PortStateMachineTest,
    tryProgrammingTransceiverWithoutPhyProgrammingComplete) {
  for (auto multiPort : {false, true}) {
    initManagers();
    XLOG(INFO)
        << "Verifying tryProgrammingTransceiverWithoutPhyProgrammingComplete for multiPort = "
        << multiPort;
    verifyStateMachine(
        {TcvrPortStatePair{
            TransceiverStateMachineState::DISCOVERED,
            {PortStateMachineState::INITIALIZED,
             optionalPortState(
                 multiPort, PortStateMachineState::INITIALIZED)}}},
        TcvrPortStatePair{
            TransceiverStateMachineState::TRANSCEIVER_READY,
            {PortStateMachineState::INITIALIZED,
             optionalPortState(multiPort, PortStateMachineState::INITIALIZED)}}
        /* expected
        state */
        ,
        []() {} /* preUpdate */,
        [this]() {
          for (auto i = 0; i < 10; ++i) {
            transceiverManager_->triggerProgrammingEvents();
          }
        } /* stateUpdate */,
        []() {} /* verify */,
        "triggerTcvrProgramming");
    // Prepare for testing with next multiPort value
    resetManagers();
  }
}

TEST_F(PortStateMachineTest, furthestStatesWhenTransceiverNotPresent) {
  initManagers();
  XLOG(INFO) << "Verifying furthestStatesWhenTransceiverNotPresent";

  verifyStateMachine(
      {
          TcvrPortStatePair{
              TransceiverStateMachineState::NOT_PRESENT,
              {PortStateMachineState::INITIALIZED, std::nullopt}},
      },
      TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
          {PortStateMachineState::PORT_UP, std::nullopt}} /* expected state */,
      [this]() {
        setMockCmisPresence(false);
        xcvr_->detectPresence();
      } /* preUpdate */,
      [this]() {
        for (int i = 0; i < 5; ++i) {
          refreshAndTriggerProgramming();
        }
      } /* stateUpdate */,
      []() {} /* verify */,
      "furthestStatesWhenTransceiverNotPresent",
      true /* isMock */);
}

TEST_F(PortStateMachineTest, furthestStatesWhenTransceiverNotReady) {
  initManagers();
  XLOG(INFO) << "Verifying furthestStatesWhenTransceiverNotReady";

  verifyStateMachine(
      {
          TcvrPortStatePair{
              TransceiverStateMachineState::NOT_PRESENT,
              {PortStateMachineState::INITIALIZED, std::nullopt}},
      },
      TcvrPortStatePair{
          TransceiverStateMachineState::DISCOVERED,
          {PortStateMachineState::XPHY_PORTS_PROGRAMMED,
           std::nullopt}} /* expected state */,
      [this]() {
        setMockCmisPresence(true);
        setMockCmisTransceiverReady(false);
      } /* preUpdate */,
      [this]() {
        for (int i = 0; i < 5; ++i) {
          refreshAndTriggerProgramming();
        }
      } /* stateUpdate */,
      []() {} /* verify */,
      "furthestStatesWhenTransceiverNotReady",
      true /* isMock */);
}

TEST_F(PortStateMachineTest, fullSimpleRefreshCycle) {
  for (auto multiPort : {false, true}) {
    initManagers();
    XLOG(INFO) << "Verifying fullSimpleRefreshCycle = " << multiPort;

    verifyStateMachine(
        {TcvrPortStatePair{
            TransceiverStateMachineState::NOT_PRESENT,
            {PortStateMachineState::UNINITIALIZED,
             optionalPortState(
                 multiPort, PortStateMachineState::UNINITIALIZED)}}},
        TcvrPortStatePair{
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
            {PortStateMachineState::PORT_UP,
             optionalPortState(
                 multiPort,
                 PortStateMachineState::PORT_UP)}} /* expected state */,
        []() {} /* preUpdate */,
        [this]() {
          for (int i = 0; i < 5; ++i) {
            refreshAndTriggerProgramming();
          }
        } /* stateUpdate */,
        [this, multiPort]() {
          verifyXphyNeedResetDataPath(multiPort, false /* expected */);
        } /* verify */,
        "allProgramming completes");

    // Prepare for testing with next multiPort value
    resetManagers();
  }
}

TEST_F(
    PortStateMachineTest,
    transceiverDownReinitializesPortThenTransceiverUp) {
  auto assertCurrentStateEquals = [this](TcvrPortStatePair expectedState) {
    auto currentState = getCurrentState(false);
    ASSERT_EQ(currentState, expectedState)
        << "Intermediate state doesn't match expected after transceiver down, "
        << "expected=" << logState(expectedState)
        << ", actual=" << logState(currentState);
  };

  initManagers();

  // Set the original state.
  xcvr_ = overrideTransceiver(false, true);
  setState(
      TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
      {PortStateMachineState::PORT_UP, std::nullopt},
      false);

  // Mark transceiver as not present.
  setMockCmisPresence(false);
  setMockCmisTransceiverReady(false);
  portManager_->setOverrideAgentPortStatusForTesting({}, {portId1_, portId3_});
  transceiverManager_->refreshTransceivers();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::NOT_PRESENT,
          {PortStateMachineState::PORT_UP, std::nullopt}});

  // Try re-initializing ports, but port should stay up until agent brings port
  // down.
  portManager_->detectTransceiverDiscoveredAndReinitializeCorrespondingPorts();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::NOT_PRESENT,
          {PortStateMachineState::PORT_UP, std::nullopt}});

  // Agent brings port down.
  portManager_->updateTransceiverPortStatus();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::NOT_PRESENT, // Not present workflow
          {PortStateMachineState::PORT_DOWN, std::nullopt}});

  // Not present workflow brings transceiver to TRANSCEIVER_READY even though
  // transceiver isn't present.
  portManager_->triggerProgrammingEvents();
  transceiverManager_->triggerProgrammingEvents();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_READY, // Not present
                                                           // workflow
          {PortStateMachineState::PORT_DOWN, std::nullopt}});

  // Transceiver is now present, gets discovered and progressed to the next
  // state.
  setMockCmisPresence(true);
  setMockCmisTransceiverReady(true);
  transceiverManager_->refreshTransceivers();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::DISCOVERED,
          {PortStateMachineState::PORT_DOWN, std::nullopt}});

  // Port is detected as needing to re-initialize.
  portManager_->detectTransceiverDiscoveredAndReinitializeCorrespondingPorts();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::DISCOVERED,
          {PortStateMachineState::INITIALIZED, std::nullopt}});

  // Ensure port status from agent doesn't progress any states.
  portManager_->updateTransceiverPortStatus();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::DISCOVERED,
          {PortStateMachineState::INITIALIZED, std::nullopt}});

  // First round of programming.
  portManager_->triggerProgrammingEvents();
  transceiverManager_->triggerProgrammingEvents();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_READY, // Not present
                                                           // workflow
          {PortStateMachineState::IPHY_PORTS_PROGRAMMED, std::nullopt}});

  // Second round of programming.
  refreshAndTriggerProgramming();
  refreshAndTriggerProgramming();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED, // Not present
                                                                // workflow
          {PortStateMachineState::TRANSCEIVERS_PROGRAMMED, std::nullopt}});

  // Agent brings port down based on latest status;
  refreshAndTriggerProgramming();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED, // Not present
                                                                // workflow
          {PortStateMachineState::PORT_DOWN, std::nullopt}});

  // Agent sets port up.
  portManager_->setOverrideAgentPortStatusForTesting(
      {portId1_, portId3_}, {portId1_, portId3_});
  refreshAndTriggerProgramming();
  assertCurrentStateEquals(
      TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED, // Not present
                                                                // workflow
          {PortStateMachineState::PORT_UP, std::nullopt}});
}

TEST_F(PortStateMachineTest, agentConfigChangedColdBoot) {
  for (auto multiPort : {false, true}) {
    initManagers();
    XLOG(INFO) << "Verifying agentConfigChangedColdBoot = " << multiPort;

    verifyStateMachine(
        {TcvrPortStatePair{
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
            {PortStateMachineState::PORT_UP,
             optionalPortState(multiPort, PortStateMachineState::PORT_UP)}}},
        TcvrPortStatePair{
            TransceiverStateMachineState::DISCOVERED,
            {PortStateMachineState::UNINITIALIZED,
             optionalPortState(
                 multiPort,
                 PortStateMachineState::UNINITIALIZED)}} /* expected state */,
        []() {} /* preUpdate */,
        [this]() { triggerAgentConfigChanged(true); } /* stateUpdate */,
        [this, multiPort]() {
          const auto& stateMachine =
              transceiverManager_->getStateMachineForTesting(tcvrId_);
          EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
          EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
          verifyXphyNeedResetDataPath(multiPort, true /* expected */);

          portManager_->refreshStateMachines();
          portManager_->refreshStateMachines();
          portManager_->refreshStateMachines();

          verifyXphyNeedResetDataPath(multiPort, false /* expected */);
        } /* verify */,
        "agentConfigChanged ColdBoot");

    resetManagers();
  }
}

TEST_F(PortStateMachineTest, agentConfigChangedWarmBoot) {
  for (auto multiPort : {false, true}) {
    initManagers();
    XLOG(INFO) << "Verifying agentConfigChangedWarmBoot = " << multiPort;

    verifyStateMachine(
        {TcvrPortStatePair{
            TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
            {PortStateMachineState::PORT_UP,
             optionalPortState(multiPort, PortStateMachineState::PORT_UP)}}},
        TcvrPortStatePair{
            TransceiverStateMachineState::DISCOVERED,
            {PortStateMachineState::UNINITIALIZED,
             optionalPortState(
                 multiPort, PortStateMachineState::UNINITIALIZED)}}
        /* expected state
         */
        ,
        []() {} /* preUpdate */,
        [this]() { triggerAgentConfigChanged(false); } /* stateUpdate */,
        [this, multiPort]() {
          // Enter DISCOVERED will also call `resetProgrammingAttributes`
          const auto& stateMachine =
              transceiverManager_->getStateMachineForTesting(tcvrId_);
          EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
          EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
          EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
          EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));

          verifyXphyNeedResetDataPath(multiPort, false /* expected */);
        } /* verify */,
        "agentConfigChanged WarmBoot");

    resetManagers();
  }
}

TEST_F(PortStateMachineTest, agentConfigChangedColdBootOnAbsentXcvr) {
  initManagers();
  XLOG(INFO) << "Verifying agentConfigChangedColdBoot = ";

  verifyStateMachine(
      {TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
          {PortStateMachineState::PORT_UP, std::nullopt}}},
      TcvrPortStatePair{
          TransceiverStateMachineState::NOT_PRESENT,
          {PortStateMachineState::UNINITIALIZED, std::nullopt}}
      /* expected state
       */
      ,
      [this]() {
        setMockCmisPresence(false);
        xcvr_->detectPresence();
      } /* preUpdate */,
      [this]() { triggerAgentConfigChanged(true); } /* stateUpdate */,
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(tcvrId_);
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
        verifyXphyNeedResetDataPath(false /* multiPort */, true /* expected */);

        portManager_->refreshStateMachines();
        portManager_->refreshStateMachines();
        portManager_->refreshStateMachines();
        verifyXphyNeedResetDataPath(
            false /* multiPort */, false /* expected */);
      } /* verify */,
      "agentConfigChanged ColdBoot",
      true /* isMock */);

  resetManagers();
}

TEST_F(PortStateMachineTest, agentConfigChangedWarmBootOnAbsentXcvr) {
  initManagers();
  XLOG(INFO) << "Verifying agentConfigChangedWarmBootOnAbsentXcvr = ";

  verifyStateMachine(
      {TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
          {PortStateMachineState::PORT_UP, std::nullopt}}},
      TcvrPortStatePair{
          TransceiverStateMachineState::NOT_PRESENT,
          {PortStateMachineState::UNINITIALIZED, std::nullopt}}
      /* expected state
       */
      ,
      [this]() {
        setMockCmisPresence(false);
        xcvr_->detectPresence();
      } /* preUpdate */,
      [this]() { triggerAgentConfigChanged(false); } /* stateUpdate */,
      [this]() {
        // Enter DISCOVERED will also call `resetProgrammingAttributes`
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(tcvrId_);
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));

        verifyXphyNeedResetDataPath(
            false /* multiPort */, false /* expected */);
      } /* verify */,
      "agentConfigChanged ColdBoot",
      true /* isMock */);

  resetManagers();
}

TEST_F(PortStateMachineTest, syncPortsOnRemovedTransceiver) {
  initManagers();
  verifyStateMachine(
      {TcvrPortStatePair{
          TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED,
          {PortStateMachineState::PORT_UP, std::nullopt}}},
      TcvrPortStatePair{
          TransceiverStateMachineState::NOT_PRESENT,
          {PortStateMachineState::PORT_DOWN, std::nullopt}},
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
        updateTransceiverActiveState(
            false /* up */, true /* enabled */, portId1_);
        // The refresh() will let TransceiverStateMachine trigger next event
        xcvr_->refresh();
      } /* stateUpdate */,
      [this]() {
        const auto& stateMachine =
            transceiverManager_->getStateMachineForTesting(tcvrId_);
        EXPECT_FALSE(stateMachine.get_attribute(isIphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isXphyProgrammed));
        EXPECT_FALSE(stateMachine.get_attribute(isTransceiverProgrammed));
        EXPECT_TRUE(stateMachine.get_attribute(needMarkLastDownTime));
      } /* verify */,
      "Triggering syncPorts with port down on a removed transceiver",
      true);
}

} // namespace facebook::fboss
