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

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/VlanMapDelta.h"

using facebook::fboss::DeltaFunctions::forEachChanged;

namespace facebook::fboss {

template <typename AddrT>
auto StaticL2ForNeighborObserver::getTableDelta(const VlanDelta& vlanDelta) {
  if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
    return vlanDelta.getMacDelta();
  } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
    return vlanDelta.getArpDelta();
  } else {
    return vlanDelta.getNdpDelta();
  }
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

template <typename AddrT>
void StaticL2ForNeighborObserver::processUpdates(const StateDelta& stateDelta) {
  for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
    auto newVlan = vlanDelta.getNew();
    if (!newVlan) {
      continue;
    }
    auto vlan = newVlan->getID();

    for (const auto& delta : getTableDelta<AddrT>(vlanDelta)) {
      auto oldEntry = delta.getOld();
      auto newEntry = delta.getNew();

      if (!oldEntry) {
        processAdded(stateDelta.newState(), vlan, newEntry);
      } else if (!newEntry) {
        processRemoved(stateDelta.oldState(), vlan, oldEntry);
      } else {
        processChanged(stateDelta, vlan, oldEntry, newEntry);
      }
    }
  }
}
} // namespace facebook::fboss
