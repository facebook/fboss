// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/HwSwitch.h"

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> HwSwitch::stateChanged(
    const std::vector<StateDelta>& deltas,
    const HwWriteBehaviorRAII& /*behavior*/) {
  setProgrammedState(stateChangedImpl(deltas));
  return getProgrammedState();
}

} // namespace facebook::fboss
