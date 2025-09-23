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

  // We default to CMIS module for testing.
  QsfpModule* overrideTransceiver(bool multiPort) {
    // Set port status to DOWN so that we can remove the transceiver correctly
    auto overrideCmisModule = [this, multiPort]() {
      // This override function use ids starting from 1
      transceiverManager_->overrideMgmtInterface(
          static_cast<int>(tcvrId_) + 1,
          uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS));
      XLOG(INFO) << "Making CMIS QSFP for " << tcvrId_;
      std::unique_ptr<FakeTransceiverImpl> xcvrImpl;
      if (multiPort) {
        qsfpImpls_.push_back(std::make_unique<Cmis400GFr4MultiPortTransceiver>(
            tcvrId_, transceiverManager_.get()));
      } else {
        qsfpImpls_.push_back(std::make_unique<Cmis200GTransceiver>(
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
    }
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

  std::optional<PortStateMachineState> optionalPortState(
      bool condition,
      PortStateMachineState state) {
    return condition ? std::optional{state} : std::nullopt;
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
      const std::string& updateStr) {
    for (const auto& tcvrPortStatePair : supportedStates) {
      auto multiPort = isMultiPort(tcvrPortStatePair);
      XLOG(INFO) << "Verifying Transceiver=0 state CHANGED by " << updateStr
                 << " from " << logState(tcvrPortStatePair) << " to "
                 << logState(expectedState);

      // Always create a new transceiver so that we can make sure the state
      // can go back to the beginning state
      xcvr_ = overrideTransceiver(multiPort);
      setState(tcvrPortStatePair.first, tcvrPortStatePair.second, multiPort);

      // Call preUpdate() before actual stateUpdate()
      preUpdate();

      // Trigger state update
      stateUpdate();

      // Check current state matches expected state
      auto curState = TcvrPortStatePair{
          transceiverManager_->getCurrentState(tcvrId_),
          {portManager_->getPortState(portId1_),
           optionalPortState(multiPort, portManager_->getPortState(portId3_))}};
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
      bool holdTransceiversLockWhileUpdating = false) {
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
        stateUpdateFnStr);
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
      const std::string& updateStr) {
    for (const auto& tcvrPortStatePair : supportedStates) {
      auto multiPort = isMultiPort(tcvrPortStatePair);
      XLOG(INFO) << "Verifying Transceiver=0 state UNCHANGED by " << updateStr
                 << " for " << logState(tcvrPortStatePair)
                 << ", multiPort=" << multiPort;
      // Always create a new transceiver so that we can make sure the state
      // can go back to the beginning state
      xcvr_ = overrideTransceiver(multiPort);
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
      VERIFY_FN verify) {
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
        supportedStates, preUpdate, stateUpdateFn, verify, stateUpdateFnStr);
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
      TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED);
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
            TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED,
            {PortStateMachineState::INITIALIZED,
             optionalPortState(
                 multiPort,
                 PortStateMachineState::INITIALIZED)}} /* expected state */,
        []() {} /* preUpdate */,
        [this]() { initializePortsThroughRefresh(); } /* stateUpdate */,
        []() {} /* verify */,
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
        [this]() {} /* preUpdate */,
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

} // namespace facebook::fboss
