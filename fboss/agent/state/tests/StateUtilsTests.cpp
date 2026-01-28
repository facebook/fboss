/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/StateUtils.h"

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/common/Utils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_shared;
using std::shared_ptr;

namespace {

HwSwitchMatcher hwMatcher() {
  return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}));
}

} // namespace

class ComputeReversedDeltasTest : public ::testing::Test {
 protected:
  void SetUp() override {
    platform_ = createMockPlatform();
    auto emptyConfig = cfg::SwitchConfig();
    baseState_ = publishAndApplyConfig(
        make_shared<SwitchState>(), &emptyConfig, platform_.get());
    addSwitchInfo(baseState_);
    baseState_->publish();
  }

  std::shared_ptr<SwitchState> addPort(
      const std::shared_ptr<SwitchState>& state,
      PortID portId) {
    auto newState = state->clone();
    auto ports = newState->getPorts()->modify(&newState);
    state::PortFields portFields;
    portFields.portId() = portId;
    portFields.portName() = folly::sformat("port{}", static_cast<int>(portId));
    auto port = std::make_shared<Port>(std::move(portFields));
    ports->addNode(port, hwMatcher());
    newState->publish();
    return newState;
  }

  std::shared_ptr<SwitchState> removePort(
      const std::shared_ptr<SwitchState>& state,
      PortID portId) {
    auto newState = state->clone();
    auto ports = newState->getPorts()->modify(&newState);
    ports->removeNode(portId);
    newState->publish();
    return newState;
  }

  std::vector<fsdb::OperDelta> buildOperDeltas(
      const std::vector<std::pair<
          std::shared_ptr<SwitchState>,
          std::shared_ptr<SwitchState>>>& statePairs) {
    std::vector<fsdb::OperDelta> operDeltas;
    for (const auto& [oldState, newState] : statePairs) {
      auto operDelta = fsdb::computeOperDelta(oldState, newState, {}, true);
      operDeltas.push_back(std::move(operDelta));
    }
    return operDeltas;
  }

  std::unique_ptr<MockPlatform> platform_;
  std::shared_ptr<SwitchState> baseState_;
};

TEST_F(ComputeReversedDeltasTest, EmptyDeltas) {
  std::vector<fsdb::OperDelta> emptyDeltas;

  auto reversedDeltas =
      utility::computeReversedDeltas(emptyDeltas, baseState_, baseState_);

  EXPECT_TRUE(reversedDeltas.empty());
}

TEST_F(ComputeReversedDeltasTest, SingleDeltaFailureAtFirstDelta) {
  // Current state is baseState_
  // We're adding a port (state1)
  // But intermediate state == current state means failure happened at first
  // delta In this case, we should get an empty reversed deltas vector (go
  // directly to initial state)
  auto state1 = addPort(baseState_, PortID(100));

  auto operDeltas = buildOperDeltas({{baseState_, state1}});

  // When intermediateState == currentState, no rollback deltas are needed
  // because we failed at the very first delta
  auto reversedDeltas =
      utility::computeReversedDeltas(operDeltas, baseState_, baseState_);

  EXPECT_TRUE(reversedDeltas.empty());
}

TEST_F(ComputeReversedDeltasTest, SingleDeltaSuccessfullyApplied) {
  // Current state is baseState_
  // We successfully applied delta to get state1
  // Then something caused rollback
  // intermediate state = state1 (last successfully applied state)
  auto state1 = addPort(baseState_, PortID(100));

  auto operDeltas = buildOperDeltas({{baseState_, state1}});

  auto reversedDeltas =
      utility::computeReversedDeltas(operDeltas, baseState_, state1);

  // Should have one reversed delta: state1 -> baseState_
  ASSERT_EQ(reversedDeltas.size(), 1);
  EXPECT_EQ(*reversedDeltas[0].oldState(), *state1);
  EXPECT_EQ(*reversedDeltas[0].newState(), *baseState_);
}

TEST_F(ComputeReversedDeltasTest, MultipleDeltasAllApplied) {
  // baseState_ -> state1 -> state2 -> state3
  // All deltas applied successfully, then rollback from state3
  auto state1 = addPort(baseState_, PortID(100));
  auto state2 = addPort(state1, PortID(101));
  auto state3 = addPort(state2, PortID(102));

  auto operDeltas = buildOperDeltas({
      {baseState_, state1},
      {state1, state2},
      {state2, state3},
  });

  // Intermediate state = state3 (all deltas applied)
  auto reversedDeltas =
      utility::computeReversedDeltas(operDeltas, baseState_, state3);

  // Should have 3 reversed deltas in order: (state3->state2), (state2->state1),
  // (state1->baseState_)
  ASSERT_EQ(reversedDeltas.size(), 3);

  // First reversed delta: state3 -> state2
  EXPECT_EQ(*reversedDeltas[0].oldState(), *state3);
  EXPECT_EQ(*reversedDeltas[0].newState(), *state2);

  // Second reversed delta: state2 -> state1
  EXPECT_EQ(*reversedDeltas[1].oldState(), *state2);
  EXPECT_EQ(*reversedDeltas[1].newState(), *state1);

  // Third reversed delta: state1 -> baseState_
  EXPECT_EQ(*reversedDeltas[2].oldState(), *state1);
  EXPECT_EQ(*reversedDeltas[2].newState(), *baseState_);
}

TEST_F(ComputeReversedDeltasTest, MultipleDeltasPartiallyApplied) {
  // baseState_ -> state1 -> state2 -> state3
  // Only state1 and state2 were applied, failed at state3
  // Intermediate state = state2
  auto state1 = addPort(baseState_, PortID(100));
  auto state2 = addPort(state1, PortID(101));
  auto state3 = addPort(state2, PortID(102));

  auto operDeltas = buildOperDeltas({
      {baseState_, state1},
      {state1, state2},
      {state2, state3},
  });

  // Intermediate state = state2 (failed at third delta)
  auto reversedDeltas =
      utility::computeReversedDeltas(operDeltas, baseState_, state2);

  // Should have 2 reversed deltas: (state2->state1), (state1->baseState_)
  ASSERT_EQ(reversedDeltas.size(), 2);

  // First reversed delta: state2 -> state1
  EXPECT_EQ(*reversedDeltas[0].oldState(), *state2);
  EXPECT_EQ(*reversedDeltas[0].newState(), *state1);

  // Second reversed delta: state1 -> baseState_
  EXPECT_EQ(*reversedDeltas[1].oldState(), *state1);
  EXPECT_EQ(*reversedDeltas[1].newState(), *baseState_);
}

TEST_F(ComputeReversedDeltasTest, MultipleDeltasOnlyFirstApplied) {
  // baseState_ -> state1 -> state2
  // Only state1 was applied, failed at state2
  // Intermediate state = state1
  auto state1 = addPort(baseState_, PortID(100));
  auto state2 = addPort(state1, PortID(101));

  auto operDeltas = buildOperDeltas({
      {baseState_, state1},
      {state1, state2},
  });

  // Intermediate state = state1 (failed at second delta)
  auto reversedDeltas =
      utility::computeReversedDeltas(operDeltas, baseState_, state1);

  // Should have 1 reversed delta: (state1->baseState_)
  ASSERT_EQ(reversedDeltas.size(), 1);

  // Reversed delta: state1 -> baseState_
  EXPECT_EQ(*reversedDeltas[0].oldState(), *state1);
  EXPECT_EQ(*reversedDeltas[0].newState(), *baseState_);
}

TEST_F(ComputeReversedDeltasTest, PortAddRemoveSequence) {
  // Test a sequence of add and remove operations
  // baseState_ -> state1 (add port100) -> state2 (add port101) -> state3
  // (remove port100)
  auto state1 = addPort(baseState_, PortID(100));
  auto state2 = addPort(state1, PortID(101));
  auto state3 = removePort(state2, PortID(100));

  auto operDeltas = buildOperDeltas({
      {baseState_, state1},
      {state1, state2},
      {state2, state3},
  });

  // All deltas applied, rollback from state3
  auto reversedDeltas =
      utility::computeReversedDeltas(operDeltas, baseState_, state3);

  ASSERT_EQ(reversedDeltas.size(), 3);

  // Verify the rollback will restore the states in reverse order
  EXPECT_EQ(*reversedDeltas[0].oldState(), *state3);
  EXPECT_EQ(*reversedDeltas[0].newState(), *state2);
  EXPECT_EQ(*reversedDeltas[1].oldState(), *state2);
  EXPECT_EQ(*reversedDeltas[1].newState(), *state1);
  EXPECT_EQ(*reversedDeltas[2].oldState(), *state1);
  EXPECT_EQ(*reversedDeltas[2].newState(), *baseState_);
}
