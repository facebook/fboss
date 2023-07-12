/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StaticL2ForNeighborUpdater.h"

#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/NeighborTableDeltaCallbackGenerator.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <type_traits>

namespace facebook::fboss {

namespace {
template <typename NeighborEntryT>
bool isReachable(const std::shared_ptr<NeighborEntryT>& entry) {
  return entry->isReachable() && !entry->getMac().isBroadcast();
}
} // namespace
void StaticL2ForNeighborUpdater::stateUpdated(const StateDelta& stateDelta) {
  if (!needL2EntryForNeighbor_) {
    return;
  }
  NeighborTableDeltaCallbackGenerator::genCallbacks(stateDelta, *this);
}

template <typename NeighborEntryT>
void StaticL2ForNeighborUpdater::assertNeighborEntry(
    const NeighborEntryT& /*neighbor*/) {
  static_assert(
      std::is_same_v<ArpEntry, NeighborEntryT> ||
      std::is_same_v<NdpEntry, NeighborEntryT>);
}
template <typename NeighborEntryT>
void StaticL2ForNeighborUpdater::processAdded(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& addedEntry) {
  assertNeighborEntry(*addedEntry);
  if (isReachable(addedEntry)) {
    XLOG(DBG2) << " Neighbor entry added: " << addedEntry->str();
    ensureMacEntryForNeighbor(vlan, addedEntry);
  }
}

template <typename NeighborEntryT>
void StaticL2ForNeighborUpdater::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& removedEntry) {
  assertNeighborEntry(*removedEntry);
  if (isReachable(removedEntry)) {
    XLOG(DBG2) << " Neighbor entry removed: " << removedEntry->str();
    pruneMacEntryForNeighbor(vlan, removedEntry);
  }
}

template <typename NeighborEntryT>
void StaticL2ForNeighborUpdater::processChanged(
    const StateDelta& stateDelta,
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& oldEntry,
    const std::shared_ptr<NeighborEntryT>& newEntry) {
  assertNeighborEntry(*oldEntry);
  assertNeighborEntry(*newEntry);
  if ((isReachable(oldEntry) != isReachable(newEntry)) ||
      (oldEntry->getMac() != newEntry->getMac())) {
    XLOG(DBG2) << " Neighbor entry changed, old: " << oldEntry->str()
               << " new: " << newEntry->str();
    processRemoved<NeighborEntryT>(stateDelta.oldState(), vlan, oldEntry);
    processAdded<NeighborEntryT>(stateDelta.newState(), vlan, newEntry);
  }
}

void StaticL2ForNeighborUpdater::processAdded(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID vlan,
    const std::shared_ptr<MacEntry>& macEntry) {
  XLOG(DBG4) << " Mac added: " << macEntry->str();
  ensureMacEntryIfNeighborExists(vlan, macEntry);
}

void StaticL2ForNeighborUpdater::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID vlan,
    const std::shared_ptr<MacEntry>& macEntry) {
  XLOG(DBG4) << " Mac removed: " << macEntry->str();
  ensureMacEntryIfNeighborExists(vlan, macEntry);
}

void StaticL2ForNeighborUpdater::processChanged(
    const StateDelta& /*stateDelta*/,
    VlanID vlan,
    const std::shared_ptr<MacEntry>& /*oldEntry*/,
    const std::shared_ptr<MacEntry>& newEntry) {
  XLOG(DBG4) << " Mac changed: " << newEntry->str();
  ensureMacEntryIfNeighborExists(vlan, newEntry);
}
} // namespace facebook::fboss
