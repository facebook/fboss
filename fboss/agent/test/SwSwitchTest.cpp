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

#include "fboss/agent/Main.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/CounterCache.h"

using namespace facebook::fboss;
using std::string;
using ::testing::_;
using ::testing::Return;

class SwSwitchTest: public ::testing::Test {
public:
  void SetUp() override {
    // Setup a default state object
    auto state = testStateA();
    handle = createTestHandle(state);
    sw = handle->getSw();
    sw->initialConfigApplied(std::chrono::steady_clock::now());
    waitForStateUpdates(sw);
  }

  void TearDown() override {
    sw = nullptr;
    handle.reset();
  }

  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
};

TEST_F(SwSwitchTest, GetPortStats) {
  // get port5 portStats for the first time
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 0);
  auto portStats = sw->portStats(PortID(5));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 1);
  EXPECT_EQ(portStats->getPortName(),
            sw->getState()->getPort(PortID(5))->getName());

  // get port5 directly from PortStatsMap
  portStats = sw->portStats(PortID(5));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 1);
  EXPECT_EQ(portStats->getPortName(),
            sw->getState()->getPort(PortID(5))->getName());

  // get port0
  portStats = sw->portStats(PortID(0));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 2);
  EXPECT_EQ(portStats->getPortName(), "port0");
}
ACTION(ThrowException)
{
  throw std::exception();
}

TEST_F(SwSwitchTest, UpdateStatsExceptionCounter){
  CounterCache counters(sw);

  MockHwSwitch* hw = static_cast<MockHwSwitch*>(sw->getHw());
  EXPECT_CALL(*hw, updateStats(sw->stats()))
    .Times(1)
    .WillRepeatedly(ThrowException());
  sw->updateStats();

  counters.update();
  counters.checkDelta(
    SwitchStats::kCounterPrefix + "update_stats_exceptions.sum.60", 1);

}

TEST_F(SwSwitchTest, HwRejectsUpdateThenAccepts) {
  CounterCache counters(sw);
  // applied and desired state in sync before we begin
  EXPECT_EQ(sw->getAppliedState(), sw->getDesiredState());
  auto origState = sw->getAppliedState();
  auto newState = bringAllPortsUp(sw->getAppliedState()->clone());
  // Have HwSwitch reject this state update. In current implementation
  // this happens only in case of table overflow. However at the SwSwitch
  // layer we don't care *why* the HwSwitch rejected this update, just
  // that it did
  EXPECT_HW_CALL(sw, stateChanged(_)).WillRepeatedly(Return(origState));
  auto stateUpdateFn = [=](const std::shared_ptr<SwitchState>& /*state*/) {
    return newState;
  };
  sw->updateState("Reject update", stateUpdateFn);
  waitForStateUpdates(sw);
  EXPECT_NE(sw->getAppliedState(), sw->getDesiredState());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_out_of_sync", 1);
  EXPECT_EQ(1, counters.value(SwitchStats::kCounterPrefix + "hw_out_of_sync"));
  // Have HwSwitch now accept this update
  EXPECT_HW_CALL(sw, stateChanged(_)).WillRepeatedly(Return(newState));
  sw->updateState("Accept update", stateUpdateFn);
  waitForStateUpdates(sw);
  EXPECT_EQ(sw->getAppliedState(), sw->getDesiredState());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_out_of_sync", -1);
  EXPECT_EQ(0, counters.value(SwitchStats::kCounterPrefix + "hw_out_of_sync"));
}
