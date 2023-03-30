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
  return (sw_->getState()->getTeFlowTable()->size() > 0) &&
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
  auto newState = state->clone();
  if (!updateTeFlowEntries(newState)) {
    return std::shared_ptr<SwitchState>(nullptr);
  }
  return newState;
}

std::shared_ptr<TeFlowTable> TeFlowNexthopHandler::updateTeFlowEntries(
    std::shared_ptr<SwitchState>& newState) {
  bool changed = false;
  auto teFlowTable = newState->getTeFlowTable();
  for (const auto& [flowStr, originalEntry] : std::as_const(*teFlowTable)) {
    if (updateTeFlowEntry(originalEntry, newState)) {
      changed = true;
    }
  }
  if (!changed) {
    return nullptr;
  }
  return newState->getTeFlowTable();
}

bool TeFlowNexthopHandler::updateTeFlowEntry(
    const std::shared_ptr<TeFlowEntry>& originalEntry,
    std::shared_ptr<SwitchState>& newState) {
  std::vector<NextHopThrift> resolvedNextHops;
  for (const auto& entry : std::as_const(*originalEntry->getNextHops())) {
    // THRIFT_COPY: Investigate if next hop thrift structure can be generically
    // represented as facebook::fboss::NextHop
    auto nexthop = entry->toThrift();
    if (TeFlowEntry::isNexthopResolved(nexthop, newState)) {
      resolvedNextHops.emplace_back(nexthop);
    }
  }
  if (originalEntry->getResolvedNextHops()->toThrift() != resolvedNextHops) {
    auto newEntry = originalEntry->modify(&newState);
    newEntry->setEnabled(!resolvedNextHops.empty());
    newEntry->setResolvedNextHops(std::move(resolvedNextHops));
    XLOG(DBG3) << "TeFlowNexthopHandler: Setting nexthop changed for flow: "
               << newEntry->str();
    return true;
  }
  return false;
}

} // namespace facebook::fboss
