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

#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"
#include "fboss/agent/test/TestUtils.h"

#include <algorithm>

using namespace facebook::fboss;

class SwSwitchHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    const std::map<int64_t, cfg::SwitchInfo> switchInfoMap = {
        {0, cfg::SwitchInfo()}, {1, cfg::SwitchInfo()}};
    hwSwitchHandler_ = std::make_unique<MultiHwSwitchHandler>(
        switchInfoMap,
        [](const SwitchID& switchId, const cfg::SwitchInfo& info) {
          return std::make_unique<
              facebook::fboss::NonMonolithicHwSwitchHandler>(switchId, info);
        });
    hwSwitchHandler_->start();
  }

 protected:
  std::unique_ptr<MultiHwSwitchHandler> hwSwitchHandler_{nullptr};
};

TEST_F(SwSwitchHandlerTest, GetOperDelta) {
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = std::make_shared<SwitchState>();
  auto newSwitchSettings = std::make_shared<SwitchSettings>();
  auto multiSwitchSwitchSettings = std::make_unique<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(
      HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}))
          .matcherString(),
      newSwitchSettings);
  multiSwitchSwitchSettings->addNode(
      HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(1)}))
          .matcherString(),
      newSwitchSettings);
  stateV1->resetSwitchSettings(std::move(multiSwitchSwitchSettings));

  auto config = testConfigA();
  config.switchSettings()->switchIdToSwitchInfo() = {
      {0, createSwitchInfo(cfg::SwitchType::NPU)},
      {1, createSwitchInfo(cfg::SwitchType::NPU)}};

  auto addRandomDelay = []() {
    std::this_thread::sleep_for(std::chrono::milliseconds{random() % 100});
  };

  auto delta = StateDelta(stateV0, stateV1);
  std::thread stateUpdateThread([this, &delta, &addRandomDelay]() {
    addRandomDelay();
    hwSwitchHandler_->stateChanged(delta, true);
    hwSwitchHandler_->stateChanged(delta, true);
    addRandomDelay();
    hwSwitchHandler_->cancelOperDeltaRequest(0);
    hwSwitchHandler_->cancelOperDeltaRequest(1);
  });

  auto clientThreadBody = [this, &delta, &addRandomDelay](int64_t switchId) {
    OperDeltaFilter filter((SwitchID(switchId)));
    // connect and get next state delta
    addRandomDelay();
    auto operDelta = hwSwitchHandler_->getNextStateOperDelta(switchId);
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // request for next state delta. serves as ack for previous one
    operDelta = hwSwitchHandler_->getNextStateOperDelta(switchId);
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // this request will be cancelled
    operDelta = hwSwitchHandler_->getNextStateOperDelta(switchId);
    // this request will be cancelled
    EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
  };

  std::thread clientRequestThread1([&]() { clientThreadBody(0); });
  std::thread clientRequestThread2([&]() { clientThreadBody(1); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}
