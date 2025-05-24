// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/HwSwitch.h"

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> HwSwitch::stateChanged(
    const StateDelta& delta,
    const HwWriteBehaviorRAII& /*behavior*/) {
  // TODO (ravi) convert above delta to vector
  std::vector<StateDelta> deltas;
  deltas.emplace_back(delta.oldState(), delta.newState());
  setProgrammedState(stateChangedImpl(deltas));
  return getProgrammedState();
}

} // namespace facebook::fboss
