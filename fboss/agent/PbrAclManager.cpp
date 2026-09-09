// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/PbrAclManager.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

std::vector<StateDelta> PbrAclManager::modifyState(
    const std::vector<StateDelta>& deltas) {
  return modifyStateImpl(deltas);
}

std::vector<StateDelta> PbrAclManager::reconstructFromSwitchState(
    const std::shared_ptr<SwitchState>& curState) {
  XLOG(DBG2) << "PbrAclManager reconstructing from switch state";
  std::vector<StateDelta> deltas;
  deltas.emplace_back(std::make_shared<SwitchState>(), curState);
  return modifyStateImpl(deltas);
}

void PbrAclManager::updateDone() {
  XLOG(DBG2) << "PbrAclManager update done";
}

void PbrAclManager::updateFailed(
    const std::shared_ptr<SwitchState>& /*curState*/) {
  XLOG(DBG2) << "PbrAclManager update failed";
}

std::shared_ptr<SwitchState> PbrAclManager::processDelta(
    const StateDelta& delta) {
  return delta.newState();
}

std::vector<StateDelta> PbrAclManager::modifyStateImpl(
    const std::vector<StateDelta>& deltas) {
  if (deltas.empty()) {
    return {};
  }
  std::vector<StateDelta> retDeltas;
  retDeltas.reserve(deltas.size());
  auto oldState = deltas.begin()->oldState();
  for (const auto& delta : deltas) {
    auto modifiedState = processDelta(StateDelta(oldState, delta.newState()));
    retDeltas.emplace_back(oldState, modifiedState);
    oldState = modifiedState;
  }
  return retDeltas;
}

} // namespace facebook::fboss
