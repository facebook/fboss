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
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/VlanMapDelta.h"

using facebook::fboss::DeltaFunctions::forEachChanged;

namespace facebook::fboss {

void StaticL2ForNeighborObserver::stateUpdated(const StateDelta& stateDelta) {
  VlanTableDeltaCallbackGenerator::genCallbacks(stateDelta, *this);
}

template <typename AddedEntryT>
void StaticL2ForNeighborObserver::processAdded(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID /*vlan*/,
    const std::shared_ptr<AddedEntryT>& /*addedEntry*/) {
  // TODO
}

template <typename RemovedEntryT>
void StaticL2ForNeighborObserver::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID /*vlan*/,
    const std::shared_ptr<RemovedEntryT>& /*removedEntry*/) {
  // TODO
}

template <typename ChangedEntryT>
void StaticL2ForNeighborObserver::processChanged(
    const StateDelta& /*stateDelta*/,
    VlanID /*vlan*/,
    const std::shared_ptr<ChangedEntryT>& /*oldEntry*/,
    const std::shared_ptr<ChangedEntryT>& /*newEntry*/) {
  // TODO
}

} // namespace facebook::fboss
