// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ShelManager.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

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

void ShelManager::updateRefCount(
    const RouteNextHopEntry::NextHopSet& routeNhops,
    const std::shared_ptr<SwitchState>& origState,
    bool add) {
  for (const auto& nhop : routeNhops) {
    // NextHops that is resolved to local interfaces
    if (nhop.isResolved() &&
        origState->getSystemPorts()->getNodeIf(SystemPortID(nhop.intf()))) {
      auto iter = intf2RefCnt_.find(nhop.intf());
      if (add) {
        if (iter == intf2RefCnt_.end()) {
          intf2RefCnt_[nhop.intf()] = 1;
        } else {
          iter->second++;
        }
      } else {
        CHECK(iter != intf2RefCnt_.end() && iter->second > 0);
        iter->second--;
        if (iter->second == 0) {
          intf2RefCnt_.erase(iter);
        }
      }
    }
  }
}

} // namespace facebook::fboss
