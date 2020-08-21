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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/VlanTableDeltaCallbackGenerator.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/SwitchState.h"

#include <type_traits>

namespace facebook::fboss {

void StaticL2ForNeighborObserver::stateUpdated(const StateDelta& stateDelta) {
  if (!sw_->getHw()->needL2EntryForNeighbor()) {
    return;
  }
  VlanTableDeltaCallbackGenerator::genCallbacks(stateDelta, *this);
}

auto StaticL2ForNeighborObserver::getMacEntry(
    VlanID vlanId,
    folly::MacAddress mac) const {
  auto vlan = sw_->getState()->getVlans()->getVlan(vlanId);
  return vlan->getMacTable()->getNodeIf(mac);
}

template <typename NeighborEntryT>
void StaticL2ForNeighborObserver::ensureMacEntry(
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& neighbor) {
  CHECK(neighbor->isReachable());
  auto mac = neighbor->getMac();
  auto port = neighbor->getPort();
  auto staticMacEntryFn =
      [vlan, mac, port](const std::shared_ptr<SwitchState>& state) {
        auto newState =
            MacTableUtils::updateOrAddStaticEntry(state, port, vlan, mac);
        return newState != state ? newState : nullptr;
      };

  sw_->updateState("updateOrAdd static MAC: ", std::move(staticMacEntryFn));
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
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& addedEntry) {
  assertNeighborEntry(*addedEntry);
  if (addedEntry->isReachable()) {
    XLOG(INFO) << " Neighbor entry added: " << addedEntry->str();
    ensureMacEntry(vlan, addedEntry);
  }
}

template <typename NeighborEntryT>
void StaticL2ForNeighborObserver::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& removedEntry) {
  assertNeighborEntry(*removedEntry);
  XLOG(INFO) << " Neighbor entry removed: " << removedEntry->str();
  auto mac = removedEntry->getMac();
  auto removeMacEntryFn = [vlan,
                           mac](const std::shared_ptr<SwitchState>& state) {
    // Note that its possible that other neighbors still refer to this MAC.
    // Handle this in 2 steps
    // - Remove MAC
    // - Run ensureMacEntryIfNeighborExists
    // State passed down to HW is the composition of these 2 steps
    auto newState = MacTableUtils::removeEntry(state, vlan, mac);
    newState =
        MacTableUtils::updateOrAddStaticEntryIfNbrExists(newState, vlan, mac);
    return newState != state ? newState : nullptr;
  };

  sw_->updateState("Prune MAC if unreferenced: ", std::move(removeMacEntryFn));
}

template <typename NeighborEntryT>
void StaticL2ForNeighborObserver::processChanged(
    const StateDelta& stateDelta,
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& oldEntry,
    const std::shared_ptr<NeighborEntryT>& newEntry) {
  assertNeighborEntry(*oldEntry);
  assertNeighborEntry(*newEntry);
  if ((oldEntry->isReachable() != newEntry->isReachable()) ||
      (oldEntry->getMac() != newEntry->getMac())) {
    XLOG(INFO) << " Neighbor entry changed, old: " << oldEntry->str()
               << " new: " << newEntry->str();
    processRemoved<NeighborEntryT>(stateDelta.oldState(), vlan, oldEntry);
    processAdded<NeighborEntryT>(stateDelta.newState(), vlan, newEntry);
  }
}

void StaticL2ForNeighborObserver::ensureMacEntryIfNeighborExists(
    VlanID vlan,
    const std::shared_ptr<MacEntry>& macEntry) {
  auto mac = macEntry->getMac();
  auto ensureMac = [mac, vlan](const std::shared_ptr<SwitchState>& state) {
    return MacTableUtils::updateOrAddStaticEntryIfNbrExists(state, vlan, mac);
  };
  sw_->updateState("ensure static MAC for nbr", std::move(ensureMac));
}

void StaticL2ForNeighborObserver::processAdded(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID vlan,
    const std::shared_ptr<MacEntry>& macEntry) {
  XLOG(DBG4) << " Mac added: " << macEntry->str();
  ensureMacEntryIfNeighborExists(vlan, macEntry);
}

void StaticL2ForNeighborObserver::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    VlanID vlan,
    const std::shared_ptr<MacEntry>& macEntry) {
  XLOG(DBG4) << " Mac removed: " << macEntry->str();
  ensureMacEntryIfNeighborExists(vlan, macEntry);
}

void StaticL2ForNeighborObserver::processChanged(
    const StateDelta& /*stateDelta*/,
    VlanID vlan,
    const std::shared_ptr<MacEntry>& /*oldEntry*/,
    const std::shared_ptr<MacEntry>& newEntry) {
  XLOG(DBG4) << " Mac changed: " << newEntry->str();
  ensureMacEntryIfNeighborExists(vlan, newEntry);
}
} // namespace facebook::fboss
