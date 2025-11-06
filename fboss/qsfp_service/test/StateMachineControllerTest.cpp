/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/StateMachineController.h"

#include "fboss/qsfp_service/TypedStateMachineUpdate.h"
#include "fboss/qsfp_service/test/MockStateMachine.h"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>
#include <folly/Synchronized.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <future>

namespace facebook::fboss {

using boost::msm::back::state_machine;
using boost::msm::front::euml::init_;

class StateMachineControllerTest : public ::testing::Test {
 public:
  using MockStateMachineController =
      StateMachineController<MockID, MockEvent, MockState, MockStateMachine>;
  using MockStateMachineUpdate = TypedStateMachineUpdate<MockEvent>;
  using BlockingMockStateMachineUpdate = BlockingStateMachineUpdate<MockEvent>;

 protected:
  void SetUp() override {
    controller_ = std::make_unique<MockStateMachineController>(
        MockStateMachineController(MockID::ID_1));
  }
  std::unique_ptr<MockStateMachineController> controller_;
};

TEST_F(StateMachineControllerTest, AddExecuteSingleUpdate) {
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_1));
  controller_->executeSingleUpdate();

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_2);
}

TEST_F(StateMachineControllerTest, AddExecuteRepeatedUpdate) {
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_1));
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_1));
  controller_->executeSingleUpdate();
  controller_->executeSingleUpdate();

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_2);
}

TEST_F(StateMachineControllerTest, AddExecuteInvalidTransitionUpdate) {
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_3));
  controller_->executeSingleUpdate();

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_1);
}

TEST_F(StateMachineControllerTest, AddExecuteNoUpdatesNoException) {
  EXPECT_NO_THROW(controller_->executeSingleUpdate());
}

TEST_F(StateMachineControllerTest, AddExecuteUpdateAfterBlock) {
  controller_->blockNewUpdates();
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_1));

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_1);
}

TEST_F(StateMachineControllerTest, AddExecuteMultipleUpdatesIndividually) {
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_1));
  controller_->executeSingleUpdate();
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_2));
  controller_->executeSingleUpdate();

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_3);
}

TEST_F(StateMachineControllerTest, AddExecuteMultipleUpdatesInSameQueue) {
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_1));
  controller_->enqueueUpdate(
      std::make_unique<MockStateMachineUpdate>(MockEvent::EVENT_2));
  controller_->executeSingleUpdate();
  controller_->executeSingleUpdate();

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_3);
}

TEST_F(StateMachineControllerTest, BlockingUpdateAddExecuteSingleUpdate) {
  auto result = std::make_shared<BlockingStateMachineUpdateResult>();
  controller_->enqueueUpdate(
      std::make_unique<BlockingMockStateMachineUpdate>(
          MockEvent::EVENT_1, result));

  auto future = std::async(
      std::launch::async, [&]() { controller_->executeSingleUpdate(); });

  result->wait();

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_2);
}

TEST_F(StateMachineControllerTest, BlockingUpdateAddExecuteMultipleUpdates) {
  auto result = std::make_shared<BlockingStateMachineUpdateResult>();
  controller_->enqueueUpdate(
      std::make_unique<BlockingMockStateMachineUpdate>(
          MockEvent::EVENT_1, result));
  auto result2 = std::make_shared<BlockingStateMachineUpdateResult>();
  controller_->enqueueUpdate(
      std::make_unique<BlockingMockStateMachineUpdate>(
          MockEvent::EVENT_2, result2));

  auto future = std::async(std::launch::async, [&]() {
    controller_->executeSingleUpdate();
    controller_->executeSingleUpdate();
  });

  result2->wait();
  result->wait();

  EXPECT_EQ(controller_->getCurrentState(), MockState::STATE_3);
}
} // namespace facebook::fboss
