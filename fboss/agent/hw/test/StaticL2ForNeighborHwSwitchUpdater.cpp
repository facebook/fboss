/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/StaticL2ForNeighborHwSwitchUpdater.h"

#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

StaticL2ForNeighborHwSwitchUpdater::StaticL2ForNeighborHwSwitchUpdater(
    HwSwitchEnsemble* hwSwitchEnsemble)
    : StaticL2ForNeighborUpdater(
          hwSwitchEnsemble->getHwSwitch()->needL2EntryForNeighbor()),
      hwSwitchEnsemble_(hwSwitchEnsemble) {}

template <typename NeighborEntryT>
void StaticL2ForNeighborHwSwitchUpdater::ensureMacEntry(
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& neighbor) {
  CHECK(neighbor->isReachable());
  auto mac = neighbor->getMac();
  auto port = neighbor->getPort();
  auto newState = MacTableUtils::updateOrAddStaticEntry(
      hwSwitchEnsemble_->getProgrammedState(), port, vlan, mac);
  hwSwitchEnsemble_->applyNewState(newState);
}

void StaticL2ForNeighborHwSwitchUpdater::ensureMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<NdpEntry>& neighbor) {
  ensureMacEntry(vlan, neighbor);
}

void StaticL2ForNeighborHwSwitchUpdater::ensureMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<ArpEntry>& neighbor) {
  ensureMacEntry(vlan, neighbor);
}

template <typename NeighborEntryT>
void StaticL2ForNeighborHwSwitchUpdater::pruneMacEntry(
    VlanID vlan,
    const std::shared_ptr<NeighborEntryT>& removedEntry) {
  XLOG(DBG2) << " Neighbor entry removed: " << removedEntry->str();
  auto mac = removedEntry->getMac();
  // Note that its possible that other neighbors still refer to this MAC.
  // Handle this in 2 steps
  // - Remove MAC
  // - Run ensureMacEntryIfNeighborExists
  // State passed down to HW is the composition of these 2 steps
  auto newState = MacTableUtils::removeEntry(
      hwSwitchEnsemble_->getProgrammedState(), vlan, mac);
  newState =
      MacTableUtils::updateOrAddStaticEntryIfNbrExists(newState, vlan, mac);
  hwSwitchEnsemble_->applyNewState(newState);
}

void StaticL2ForNeighborHwSwitchUpdater::pruneMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<ArpEntry>& neighbor) {
  pruneMacEntry(vlan, neighbor);
}

void StaticL2ForNeighborHwSwitchUpdater::pruneMacEntryForNeighbor(
    VlanID vlan,
    const std::shared_ptr<NdpEntry>& neighbor) {
  pruneMacEntry(vlan, neighbor);
}

void StaticL2ForNeighborHwSwitchUpdater::ensureMacEntryIfNeighborExists(
    VlanID vlan,
    const std::shared_ptr<MacEntry>& macEntry) {
  auto mac = macEntry->getMac();
  auto newState = MacTableUtils::updateOrAddStaticEntryIfNbrExists(
      hwSwitchEnsemble_->getProgrammedState(), vlan, mac);
  hwSwitchEnsemble_->applyNewState(newState);
}
} // namespace facebook::fboss
