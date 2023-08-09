/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StaticL2ForNeighborSwSwitchUpdater.h"

#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

StaticL2ForNeighborSwSwitchUpdater::StaticL2ForNeighborSwSwitchUpdater(
    SwSwitch* sw)
    : StaticL2ForNeighborUpdater(
          sw->getHwSwitchHandlerDeprecated()->needL2EntryForNeighbor()),
      sw_(sw) {}

template <typename NeighborEntryT>
void StaticL2ForNeighborSwSwitchUpdater::ensureMacEntry(
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

  sw_->updateState(
      "updateOrAdd static MAC: " + neighbor->str(),
      std::move(staticMacEntryFn));
}

void StaticL2ForNeighborSwSwitchUpdater::ensureMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<NdpEntry>& neighbor) {
  ensureMacEntry(vlan, neighbor);
}

void StaticL2ForNeighborSwSwitchUpdater::ensureMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<ArpEntry>& neighbor) {
  ensureMacEntry(vlan, neighbor);
}

template <typename NeighborEntryT>
void StaticL2ForNeighborSwSwitchUpdater::pruneMacEntry(
    VlanID vlanId,
    const std::shared_ptr<NeighborEntryT>& removedEntry) {
  XLOG(DBG2) << " Neighbor entry removed: " << removedEntry->str();
  auto mac = removedEntry->getMac();
  auto removeMacEntryFn = [vlanId,
                           mac](const std::shared_ptr<SwitchState>& state) {
    // Note that its possible that other neighbors still refer to this MAC.
    // Handle this in 2 steps
    // - Remove MAC
    // - Run ensureMacEntryIfNeighborExists
    // State passed down to HW is the composition of these 2 steps
    bool macPruned = false;
    auto newState = MacTableUtils::removeEntry(state, vlanId, mac);
    if (newState != state) {
      // Check if another neighbor still points to this MAC
      newState = MacTableUtils::updateOrAddStaticEntryIfNbrExists(
          newState, vlanId, mac);
      auto vlan = newState->getVlans()->getNodeIf(vlanId);
      macPruned = vlan->getMacTable()->getMacIf(mac) == nullptr;
    }
    return macPruned ? newState : nullptr;
  };

  sw_->updateState(
      "Prune MAC if unreferenced: " + removedEntry->str(),
      std::move(removeMacEntryFn));
}

void StaticL2ForNeighborSwSwitchUpdater::pruneMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<ArpEntry>& neighbor) {
  pruneMacEntry(vlan, neighbor);
}

void StaticL2ForNeighborSwSwitchUpdater::pruneMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<NdpEntry>& neighbor) {
  pruneMacEntry(vlan, neighbor);
}

void StaticL2ForNeighborSwSwitchUpdater::ensureMacEntryIfNeighborExists(
    VlanID vlan,
    const std::shared_ptr<MacEntry>& macEntry) {
  auto mac = macEntry->getMac();
  auto ensureMac = [mac, vlan](const std::shared_ptr<SwitchState>& state) {
    return MacTableUtils::updateOrAddStaticEntryIfNbrExists(state, vlan, mac);
  };
  sw_->updateState(
      "ensure static MAC for nbr: " + macEntry->str(), std::move(ensureMac));
}
} // namespace facebook::fboss
