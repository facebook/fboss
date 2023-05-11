// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/TeFlowNexthopHandler.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/TeFlowTable.h"

namespace facebook::fboss {

using DeltaFunctions::isEmpty;

TeFlowNexthopHandler::TeFlowNexthopHandler(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "TeFlowNexthopHandler");
}
TeFlowNexthopHandler::~TeFlowNexthopHandler() {
  sw_->unregisterStateObserver(this);
}
bool TeFlowNexthopHandler::hasTeFlowChanges(const StateDelta& delta) {
  return (sw_->getState()->getMultiSwitchTeFlowTable()->numNodes() > 0) &&
      (!isEmpty(delta.getVlansDelta()));
}

/*
 * Resolve TE flow nexthops when neighbor entry changes
 * On a local link down event, the following the event sequence
 *  - Agent detects link down from sdk callback. Updates neighbor reachability
 *  - TeFlowNextHopHandler reacts to neighbor changes and disables
 *    impacted flows
 *  - TE agent is notified of neighbor change and removes the flow entries
 *  - Controller is notified and removes the impacted entries. This is no-op for
 *    wedge agent.
 */

void TeFlowNexthopHandler::stateUpdated(const StateDelta& delta) {
  if (!hasTeFlowChanges(delta)) {
    return;
  }

  auto updateTeFlowsFn = [this](const std::shared_ptr<SwitchState>& state) {
    return handleUpdate(state);
  };
  sw_->updateState("Updating TeFlows", updateTeFlowsFn);
}

std::shared_ptr<SwitchState> TeFlowNexthopHandler::handleUpdate(
    const std::shared_ptr<SwitchState>& state) {
  return updateTeFlowEntries(state);
}

std::shared_ptr<SwitchState> TeFlowNexthopHandler::updateTeFlowEntries(
    const std::shared_ptr<SwitchState>& state) {
  bool changed = false;
  auto newState = state->clone();
  auto multiTeFlowTable =
      newState->getMultiSwitchTeFlowTable()->modify(&newState);
  for (auto iter = multiTeFlowTable->begin(); iter != multiTeFlowTable->end();
       iter++) {
    auto teFlowTable = iter->second;
    for (const auto& [flowStr, originalEntry] : std::as_const(*teFlowTable)) {
      auto newEntry = originalEntry->clone();
      if (updateTeFlowEntry(newEntry, newState)) {
        teFlowTable->updateNode(newEntry);
        changed = true;
      }
    }
  }
  if (!changed) {
    return nullptr;
  }
  return newState;
}

bool TeFlowNexthopHandler::updateTeFlowEntry(
    const std::shared_ptr<TeFlowEntry>& newEntry,
    std::shared_ptr<SwitchState>& newState) {
  std::vector<NextHopThrift> resolvedNextHops;
  for (const auto& entry : std::as_const(*newEntry->getNextHops())) {
    // THRIFT_COPY: Investigate if next hop thrift structure can be generically
    // represented as facebook::fboss::NextHop
    auto nexthop = entry->toThrift();
    if (TeFlowEntry::isNexthopResolved(nexthop, newState)) {
      resolvedNextHops.emplace_back(nexthop);
    }
  }
  if (newEntry->getResolvedNextHops()->toThrift() != resolvedNextHops) {
    newEntry->setEnabled(!resolvedNextHops.empty());
    newEntry->setResolvedNextHops(std::move(resolvedNextHops));
    XLOG(DBG3) << "TeFlowNexthopHandler: Setting nexthop changed for flow: "
               << newEntry->str();
    return true;
  }
  return false;
}

} // namespace facebook::fboss
