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

#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwitchStatsObserver.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

namespace {
auto constexpr kSystemPortId = 42;
auto constexpr kRemoteSwitchId = 42;
auto constexpr kNumNewPorts = 3;
} // namespace

namespace facebook::fboss {
class SwitchStatsObserverTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto cfg = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&cfg);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
    waitForStateUpdates(sw_);
  }

  HwSwitchMatcher matcher() const {
    return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}));
  }

  void updateSysPortMap(MultiSwitchSystemPortMap* sysPorts, bool add) {
    for (int i = 0; i < kNumNewPorts; i++) {
      if (add) {
        sysPorts->addNode(
            makeSysPort(std::nullopt, kSystemPortId + i, kRemoteSwitchId),
            matcher());
      } else {
        sysPorts->removeNode(kSystemPortId + i);
      }
    }
  }

 protected:
  SwSwitch* sw_{nullptr};
  std::unique_ptr<HwTestHandle> handle_{nullptr};
};

TEST_F(SwitchStatsObserverTest, systemPortCounters) {
  CounterCache counters(sw_);
  sw_->updateStats();
  counters.update();

  auto localSysPortCounter = SwitchStats::kCounterPrefix + "systemPort";
  EXPECT_TRUE(counters.checkExist(localSysPortCounter));
  auto initialCounterVal = counters.value(localSysPortCounter);
  EXPECT_GT(initialCounterVal, 0);

  sw_->updateStateBlocking(
      "Add new sysPort", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateSysPortMap(
            newState->getSystemPorts()->modify(&newState), /* add */ true);
        return newState;
      });
  counters.update();
  EXPECT_EQ(
      initialCounterVal + kNumNewPorts, counters.value(localSysPortCounter));

  sw_->updateStateBlocking(
      "Remove sysPort", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateSysPortMap(
            newState->getSystemPorts()->modify(&newState), /* add */ false);
        return newState;
      });
  counters.update();
  EXPECT_EQ(initialCounterVal, counters.value(localSysPortCounter));
}

TEST_F(SwitchStatsObserverTest, remoteSystemPortCounters) {
  CounterCache counters(sw_);

  sw_->updateStateBlocking(
      "Add new remote sysPort", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateSysPortMap(
            newState->getRemoteSystemPorts()->modify(&newState),
            /* add */ true);
        return newState;
      });
  counters.update();

  auto remoteSysPortCounter = SwitchStats::kCounterPrefix + "remoteSystemPort";
  EXPECT_TRUE(counters.checkExist(remoteSysPortCounter));
  auto initialCounterVal = counters.value(remoteSysPortCounter);
  EXPECT_EQ(initialCounterVal, kNumNewPorts);

  sw_->updateStateBlocking(
      "Remove sysPort", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateSysPortMap(
            newState->getRemoteSystemPorts()->modify(&newState),
            /* add */ false);
        return newState;
      });
  counters.update();
  EXPECT_EQ(counters.value(remoteSysPortCounter), 0);
}
} // namespace facebook::fboss
