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

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

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
  void setStateChangedReturn(const std::shared_ptr<SwitchState>& state) {
    if (handle->getHwSwitch()->transactionsSupported()) {
      EXPECT_HW_CALL(sw, stateChangedTransaction(_))
          .WillRepeatedly(Return(state));
    } else {
      EXPECT_HW_CALL(sw, stateChangedImpl(_)).WillRepeatedly(Return(state));
    }
  }

  void setStateChangedReturn(
      std::function<std::shared_ptr<SwitchState>(const StateDelta& delta)>
          updateFn) {
    if (handle->getHwSwitch()->transactionsSupported()) {
      return setStateChangedTransactionReturn(updateFn);
    }
    EXPECT_HW_CALL(sw, stateChangedImpl(_))
        .WillRepeatedly(::testing::WithArg<0>(::testing::Invoke(updateFn)));
  }

  void setStateChangedTransactionReturn(
      std::function<std::shared_ptr<SwitchState>(const StateDelta& delta)>
          updateFn) {
    if (!FLAGS_enable_state_oper_delta) {
      EXPECT_HW_CALL(sw, stateChangedTransaction(_))
          .WillRepeatedly(::testing::WithArg<0>(::testing::Invoke(updateFn)));
    } else {
      EXPECT_HW_CALL(sw, stateChangedImpl(_))
          .WillRepeatedly(::testing::WithArg<0>(::testing::Invoke(updateFn)));
    }
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
  setStateChangedReturn([](const StateDelta& delta) {
    // Reject the update
    return delta.oldState();
  });
  auto stateUpdateFn = [](const std::shared_ptr<SwitchState>& state) {
    return bringAllPortsUp(state->clone());
  };
  EXPECT_THROW(
      sw->updateStateWithHwFailureProtection("Reject update", stateUpdateFn),
      FbossHwUpdateError);
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_update_failures", 1);
  // Have HwSwitch now accept this update
  setStateChangedReturn([](const StateDelta& delta) {
    CHECK(delta.newState() != delta.oldState());
    return delta.newState();
  });
  sw->updateState("Accept update", stateUpdateFn);
  // No increament on successful updates
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_update_failures", 0);
  waitForStateUpdates(sw);
}

TEST_P(SwSwitchUpdateProcessingTest, HwFailureProtectedUpdateAtEnd) {
  auto startState = sw->getState();
  startState->publish();
  auto nonHwFailureProtectedUpdateState = startState->clone();
  nonHwFailureProtectedUpdateState->publish();
  auto protectedState = nonHwFailureProtectedUpdateState->clone();
  bool transactionsSupported = handle->getHwSwitch()->transactionsSupported();

  auto stateChangedImplCalls = 1 + (transactionsSupported ? 0 : 1);
  auto stateChangedTransactionCalls = transactionsSupported ? 1 : 0;
  if (FLAGS_enable_state_oper_delta) {
    EXPECT_HW_CALL(sw, stateChangedImpl(_))
        .Times(stateChangedImplCalls + stateChangedTransactionCalls);
  } else {
    EXPECT_HW_CALL(sw, stateChangedImpl(_)).Times(stateChangedImplCalls);
    EXPECT_HW_CALL(sw, stateChangedTransaction(_))
        .Times(stateChangedTransactionCalls);
  }
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
  auto protectedState1 = startState->clone();
  protectedState1->publish();
  auto protectedState2 = protectedState1->clone();
  if (handle->getHwSwitch()->transactionsSupported()) {
    EXPECT_STATE_UPDATE_TRANSACTION_TIMES(sw, 2);
  } else {
    EXPECT_STATE_UPDATE_TIMES(sw, 2);
  }
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
  auto protectedState = startState->clone();
  protectedState->publish();
  auto nonHwFailureProtectedUpdatealState = protectedState->clone();
  bool transactionsSupported = handle->getHwSwitch()->transactionsSupported();
  auto stateChangedImplCalls = 1 + (transactionsSupported ? 0 : 1);
  auto stateChangedTransactionCalls = transactionsSupported ? 1 : 0;
  if (FLAGS_enable_state_oper_delta) {
    EXPECT_HW_CALL(sw, stateChangedImpl(_))
        .Times(stateChangedImplCalls + stateChangedTransactionCalls);
  } else {
    EXPECT_HW_CALL(sw, stateChangedImpl(_)).Times(stateChangedImplCalls);
    EXPECT_HW_CALL(sw, stateChangedTransaction(_))
        .Times(stateChangedTransactionCalls);
  }
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

  setStateChangedReturn([](const StateDelta& delta) {
    /* reject the update */
    return delta.oldState();
  });
  auto stateUpdateFn = [](const std::shared_ptr<SwitchState>& state) {
    return bringAllPortsUp(state->clone());
  };
  EXPECT_THROW(
      sw->updateStateWithHwFailureProtection(
          "HwFailureProtectedUpdate fail", stateUpdateFn),
      FbossHwUpdateError);

  // Next update should be a non protected update since we will schedule it such
  setStateChangedReturn([](const StateDelta& delta) {
    /* accept the update */
    return delta.newState();
  });

  StateDelta expectedDelta(origState, newState);
  auto isEqual = [&expectedDelta](const auto& delta) {
    return delta.newState()->toThrift() ==
        expectedDelta.newState()->toThrift() &&
        delta.oldState()->toThrift() == expectedDelta.oldState()->toThrift();
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

INSTANTIATE_TEST_CASE_P(
    SwSwitchUpdateProcessingTest,
    SwSwitchUpdateProcessingTest,
    ::testing::Values(true, false));
