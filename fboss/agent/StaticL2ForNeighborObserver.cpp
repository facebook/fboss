/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StaticL2ForNeighborObserver.h"

#include "fboss/agent/VlanTableDeltaCallbackGenerator.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"

#include <type_traits>

DECLARE_bool(create_l2_entry_for_nbr);

namespace facebook::fboss {

void StaticL2ForNeighborObserver::stateUpdated(const StateDelta& stateDelta) {
  if (!FLAGS_create_l2_entry_for_nbr) {
    return;
  }
  VlanTableDeltaCallbackGenerator::genCallbacks(stateDelta, *this);
}

template <typename NeighborEntryT>
void StaticL2ForNeighborObserver::assertNeighborEntry(
    const NeighborEntryT& /*neighbor*/) {
  static_assert(
      std::is_same_v<ArpEntry, NeighborEntryT> ||
      std::is_same_v<NdpEntry, NeighborEntryT>);
}
template <typename NeighborEntryT>
void StaticL2ForNeighborObserver::processAdded(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID /*vlan*/,
    const std::shared_ptr<NeighborEntryT>& addedEntry) {
  assertNeighborEntry(*addedEntry);
}

template <typename NeighborEntryT>
void StaticL2ForNeighborObserver::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID /*vlan*/,
    const std::shared_ptr<NeighborEntryT>& removedEntry) {
  assertNeighborEntry(*removedEntry);
  // TODO
}

template <typename NeighborEntryT>
void StaticL2ForNeighborObserver::processChanged(
    const StateDelta& /*stateDelta*/,
    VlanID /*vlan*/,
    const std::shared_ptr<NeighborEntryT>& oldEntry,
    const std::shared_ptr<NeighborEntryT>& newEntry) {
  assertNeighborEntry(*oldEntry);
  assertNeighborEntry(*newEntry);
}

void StaticL2ForNeighborObserver::processAdded(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID /*vlan*/,
    const std::shared_ptr<MacEntry>& /*macEntry*/) {
  // TODO
}
void StaticL2ForNeighborObserver::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID /*vlan*/,
    const std::shared_ptr<MacEntry>& /*macEntry*/) {
  // TODO
}
void StaticL2ForNeighborObserver::processChanged(
    const StateDelta& /*stateDelta*/,
    VlanID /*vlan*/,
    const std::shared_ptr<MacEntry>& /*oldEntry*/,
    const std::shared_ptr<MacEntry>& /*newEntry*/) {
  // TODO
}
} // namespace facebook::fboss
