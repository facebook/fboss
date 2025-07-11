// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ShelManager.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

std::vector<StateDelta> ShelManager::modifyState(
    const std::vector<StateDelta>& deltas) {
  // TODO: Handle list of deltas instead of single delta
  CHECK_EQ(deltas.size(), 1);

  // TODO(zecheng): implement this function
  std::vector<StateDelta> retDeltas;
  retDeltas.emplace_back(
      deltas.begin()->oldState(), deltas.begin()->newState());
  return retDeltas;
}
void ShelManager::updateDone() {
  // TODO(zecheng): implement this function
}
void ShelManager::updateFailed(
    const std::shared_ptr<SwitchState>& /*curState*/) {
  // TODO(zecheng): implement this function
}

} // namespace facebook::fboss
