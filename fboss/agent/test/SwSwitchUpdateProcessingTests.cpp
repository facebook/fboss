/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <algorithm>

using namespace facebook::fboss;
using std::string;
using ::testing::_;
using ::testing::ByRef;
using ::testing::Eq;
using ::testing::Return;

class SwSwitchUpdateProcessingTest : public ::testing::TestWithParam<bool> {
 public:
  void SetUp() override {
    // Setup a default state object
    auto state = testStateA();
    state->publish();
    handle = createTestHandle(state);
    sw = handle->getSw();
    sw->initialConfigApplied(std::chrono::steady_clock::now());
    waitForStateUpdates(sw);
    EXPECT_HW_CALL(sw, transactionsSupported())
        .WillRepeatedly(Return(GetParam()));
  }

  void TearDown() override {
    sw = nullptr;
    handle.reset();
  }

 protected:
  using updateFunc = std::function<std::shared_ptr<SwitchState>(
      const std::vector<StateDelta>&)>;
  updateFunc front = [](const std::vector<StateDelta>& deltas) {
    return deltas.front().oldState();
  };
  updateFunc back = [](const std::vector<StateDelta>& deltas) {
    return deltas.back().newState();
  };

  void setStateChangedReturn(const updateFunc& updateFn) {
    if (handle->getHwSwitch()->transactionsSupported()) {
      return setStateChangedTransactionReturn(updateFn);
    }
    EXPECT_HW_CALL(sw, stateChangedImpl(_))
        .WillRepeatedly(::testing::WithArg<0>(::testing::Invoke(updateFn)));
  }

  void setStateChangedTransactionReturn(const updateFunc& updateFn) {
    EXPECT_HW_CALL(sw, stateChangedImpl(_))
        .WillRepeatedly(::testing::WithArg<0>(::testing::Invoke(updateFn)));
  }

  std::shared_ptr<SwitchState> addMirror(
      const std::shared_ptr<SwitchState>& state) {
    auto newState = state->clone();
    auto mirrors = newState->getMirrors()->modify(&newState);
    state::MirrorFields mirror{};
    mirror.name() = "foo";
    mirrors->addNode(
        std::make_shared<Mirror>(mirror),
        HwSwitchMatcher::defaultHwSwitchMatcher());
    newState->publish();
    return newState;
  }

  std::shared_ptr<SwitchState> changeMirror(
      const std::shared_ptr<SwitchState>& state) {
    auto newState = state->clone();
    auto mirrors = newState->getMirrors()->modify(&newState);
    auto mirror = mirrors->getNode("foo")->clone();
    mirror->setEgressPortDesc(PortDescriptor(PortID(1)));
    mirrors->updateNode(mirror, HwSwitchMatcher::defaultHwSwitchMatcher());
    return newState;
  }

  std::shared_ptr<SwitchState> addAcl(
      const std::shared_ptr<SwitchState>& state,
      int idx) {
    auto newState = state->clone();
    auto aclEntry =
        make_shared<AclEntry>(idx, folly::to<std::string>("acl", idx));
    auto acls = newState->getAcls()->modify(&newState);
    acls->addNode(aclEntry, HwSwitchMatcher::defaultHwSwitchMatcher());
    newState->publish();
    return newState;
  }
  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
};

TEST_P(SwSwitchUpdateProcessingTest, HwRejectsUpdateThenAccepts) {
  CounterCache counters(sw);
  auto origState = sw->getState();
  // Have HwSwitch reject this state update. In current implementation
  // this happens only in case of table overflow. However at the SwSwitch
  // layer we don't care *why* the HwSwitch rejected this update, just
  // that it did
  // Reject the update
  setStateChangedReturn(front);
  auto stateUpdateFn = [](const std::shared_ptr<SwitchState>& state) {
    return bringAllPortsUp(state->clone());
  };
  EXPECT_THROW(
      sw->updateStateWithHwFailureProtection("Reject update", stateUpdateFn),
      FbossHwUpdateError);
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_update_failures", 1);
  CHECK_EQ(
      counters.value(SwitchStats::kCounterPrefix + "hw_update_failures"), 1);
  // Another rejected update
  EXPECT_THROW(
      sw->updateStateWithHwFailureProtection("Reject update", stateUpdateFn),
      FbossHwUpdateError);
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_update_failures", 1);
  CHECK_EQ(
      counters.value(SwitchStats::kCounterPrefix + "hw_update_failures"), 2);
  // Have HwSwitch now accept this update
  setStateChangedReturn([](const std::vector<StateDelta>& deltas) {
    CHECK(deltas.back().newState() != deltas.front().oldState());
    return deltas.back().newState();
  });
  sw->updateStateBlocking("Accept update", stateUpdateFn);
  // No increament on successful updates
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_update_failures", -2);
  CHECK_EQ(
      counters.value(SwitchStats::kCounterPrefix + "hw_update_failures"), 0);
  waitForStateUpdates(sw);
}

TEST_P(SwSwitchUpdateProcessingTest, HwFailureProtectedUpdateAtEnd) {
  auto startState = sw->getState();
  startState->publish();
  auto nonHwFailureProtectedUpdateState = this->addMirror(startState);
  auto protectedState = this->changeMirror(nonHwFailureProtectedUpdateState);
  bool transactionsSupported = handle->getHwSwitch()->transactionsSupported();

  auto stateChangedImplCalls = 1 + (transactionsSupported ? 0 : 1);
  auto stateChangedTransactionCalls = transactionsSupported ? 1 : 0;
  EXPECT_HW_CALL(sw, stateChangedImpl(_))
      .Times(stateChangedImplCalls + stateChangedTransactionCalls);
  auto nonHwFailureProtectedUpdateStateUpdateFn =
      [=](const std::shared_ptr<SwitchState>& state) {
        EXPECT_EQ(state, startState);
        return nonHwFailureProtectedUpdateState;
      };
  auto protectedStateUpdateFn = [=](const std::shared_ptr<SwitchState>& state) {
    EXPECT_EQ(state, nonHwFailureProtectedUpdateState);
    return protectedState;
  };
  sw->updateState(
      "Non protected update", nonHwFailureProtectedUpdateStateUpdateFn);
  sw->updateStateWithHwFailureProtection(
      "HwFailureProtectedUpdate update", protectedStateUpdateFn);
  EXPECT_EQ(protectedState, sw->getState());
}

TEST_P(SwSwitchUpdateProcessingTest, BackToBackHwFailureProtectedUpdates) {
  auto startState = sw->getState();
  startState->publish();
  auto protectedState1 = this->addMirror(startState);
  protectedState1->publish();
  auto protectedState2 = this->changeMirror(protectedState1);
  EXPECT_STATE_UPDATE_TIMES(sw, 2);
  auto protectedState1UpdateFn =
      [=](const std::shared_ptr<SwitchState>& state) {
        EXPECT_EQ(state, startState);
        return protectedState1;
      };
  auto protectedState2UpdateFn =
      [=](const std::shared_ptr<SwitchState>& state) {
        EXPECT_EQ(state, protectedState1);
        return protectedState2;
      };
  sw->updateStateWithHwFailureProtection(
      "HwFailureProtectedUpdate update 1", protectedState1UpdateFn);
  sw->updateStateWithHwFailureProtection(
      "HwFailureProtectedUpdate update 2", protectedState2UpdateFn);
  EXPECT_EQ(protectedState2, sw->getState());
}

TEST_P(SwSwitchUpdateProcessingTest, HwFailureProtectedUpdateAtStart) {
  auto startState = sw->getState();
  startState->publish();
  auto protectedState = this->addMirror(startState);
  protectedState->publish();
  auto nonHwFailureProtectedUpdatealState = changeMirror(protectedState);
  bool transactionsSupported = handle->getHwSwitch()->transactionsSupported();
  auto stateChangedImplCalls = 1 + (transactionsSupported ? 0 : 1);
  auto stateChangedTransactionCalls = transactionsSupported ? 1 : 0;
  EXPECT_HW_CALL(sw, stateChangedImpl(_))
      .Times(stateChangedImplCalls + stateChangedTransactionCalls);
  auto protectedStateUpdateFn = [=](const std::shared_ptr<SwitchState>& state) {
    EXPECT_EQ(state, startState);
    return protectedState;
  };
  auto nonProtectedStateUpdateFn =
      [=](const std::shared_ptr<SwitchState>& state) {
        EXPECT_EQ(state, protectedState);
        return nonHwFailureProtectedUpdatealState;
      };
  sw->updateStateWithHwFailureProtection(
      "HwFailureProtectedUpdateal update", protectedStateUpdateFn);
  sw->updateState("Non protected update", nonProtectedStateUpdateFn);
  waitForStateUpdates(sw);
  EXPECT_EQ(nonHwFailureProtectedUpdatealState, sw->getState());
}

TEST_P(
    SwSwitchUpdateProcessingTest,
    FailedHwFailureProtectedUpdateThrowsError) {
  auto origState = sw->getState();
  auto newState = bringAllPortsUp(sw->getState()->clone());
  newState->publish();
  // Have HwSwitch reject this state update. In current implementation
  // this happens only in case of table overflow. However at the SwSwitch
  // layer we don't care *why* the HwSwitch rejected this update, just
  // that it did

  // reject the update
  setStateChangedReturn(front);
  auto stateUpdateFn = [](const std::shared_ptr<SwitchState>& state) {
    return bringAllPortsUp(state->clone());
  };
  EXPECT_THROW(
      sw->updateStateWithHwFailureProtection(
          "HwFailureProtectedUpdate fail", stateUpdateFn),
      FbossHwUpdateError);

  // Next update should be a non protected update since we will schedule it such
  // accept the update
  setStateChangedReturn(back);

  StateDelta expectedDelta(origState, newState);
  auto isEqual = [&expectedDelta](const auto& deltas) {
    return deltas.back().newState()->toThrift() ==
        expectedDelta.newState()->toThrift() &&
        deltas.front().oldState()->toThrift() ==
        expectedDelta.oldState()->toThrift();
  };
  EXPECT_HW_CALL(sw, stateChangedImpl(testing::Truly(isEqual)));
  sw->updateState("Accept update", stateUpdateFn);
  waitForStateUpdates(sw);
}

TEST_P(SwSwitchUpdateProcessingTest, HwFailureProtectedUpdatesDuringExit) {
  auto startState = sw->getState();
  startState->publish();
  std::atomic<bool> updatesPaused{true};
  auto protectedState = startState->clone();
  protectedState->publish();
  auto protectedStateUpdateFn = [&updatesPaused, protectedState, &startState](
                                    const std::shared_ptr<SwitchState>& state) {
    EXPECT_EQ(state, startState);
    return protectedState;
  };
  // Queue a few updates
  std::thread updateThread([&updatesPaused, this, &protectedStateUpdateFn]() {
    try {
      while (updatesPaused.load()) {
      };
      sw->updateStateWithHwFailureProtection(
          "HwFailureProtectedUpdate update ", protectedStateUpdateFn);
    } catch (const FbossHwUpdateError&) {
    }
  });
  std::thread updateThread2([&updatesPaused, this, &protectedStateUpdateFn]() {
    while (updatesPaused.load()) {
    };
    sw->updateStateWithHwFailureProtection(
        "HwFailureProtectedUpdate update ", protectedStateUpdateFn);
  });
  // Stop SwSwitch
  std::thread stopThread([&updatesPaused, this]() {
    sw->stop();
    // unblock updates
    updatesPaused.store(false);
  });
  stopThread.join();
  updateThread.join();
  updateThread2.join();
  EXPECT_EQ(startState, sw->getState());
}

TEST_P(SwSwitchUpdateProcessingTest, ProcessDeltaVector) {
  auto startV0 = sw->getState();
  startV0->publish();

  auto startV1 = this->addAcl(startV0, 1);
  startV1->publish();
  auto startV2 = this->addAcl(startV1, 2);
  startV2->publish();
  auto startV3 = this->addAcl(startV2, 3);
  startV3->publish();

  std::vector<StateDelta> deltas;
  deltas.emplace_back(startV0, startV1);
  deltas.emplace_back(startV1, startV2);
  deltas.emplace_back(startV2, startV3);

  {
    // Reject the update
    setStateChangedReturn(front);
    auto ret = sw->getHwSwitchHandler()->stateChanged(deltas, GetParam());
    EXPECT_EQ(*ret, *startV0);
  }
  {
    // Reject 2nd update
    setStateChangedReturn([](const std::vector<StateDelta>& deltas) {
      return deltas[1].oldState();
    });
    auto ret = sw->getHwSwitchHandler()->stateChanged(deltas, GetParam());
    EXPECT_EQ(*ret, *startV1);
  }
  {
    // accept the update
    setStateChangedReturn(back);
    auto ret = sw->getHwSwitchHandler()->stateChanged(deltas, GetParam());
    EXPECT_EQ(*ret, *startV3);
  }
}

INSTANTIATE_TEST_CASE_P(
    SwSwitchUpdateProcessingTest,
    SwSwitchUpdateProcessingTest,
    ::testing::Values(true, false));
