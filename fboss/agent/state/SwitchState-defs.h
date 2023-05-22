/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

namespace facebook::fboss {

template <typename EntryClassT, typename NTableT>
void SwitchState::revertNewNeighborEntry(
    const std::shared_ptr<EntryClassT>& newEntry,
    const std::shared_ptr<EntryClassT>& oldEntry,
    std::shared_ptr<SwitchState>* appliedState) {
  const auto ip = newEntry->getIP();
  // We are assuming here that vlan is equal to interface always.
  VlanID vlanId = static_cast<VlanID>(newEntry->getIntfID());

  auto neighborTablePtr = (*appliedState)
                              ->getVlans()
                              ->getNode(vlanId)
                              ->template getNeighborTable<NTableT>()
                              .get();
  // Check that the entry exists
  auto entry = neighborTablePtr->getNodeIf(ip.str());
  CHECK(entry);
  // In this call, we also modify appliedState
  neighborTablePtr = neighborTablePtr->modify(vlanId, appliedState);
  if (!oldEntry) {
    neighborTablePtr->removeNode(ip.str());
  } else {
    neighborTablePtr->updateEntry(ip, oldEntry);
  }
}

template <typename AddressT>
void SwitchState::revertNewRouteEntry(
    const RouterID& id,
    const std::shared_ptr<Route<AddressT>>& newRoute,
    const std::shared_ptr<Route<AddressT>>& oldRoute,
    std::shared_ptr<SwitchState>* appliedState) {
  auto clonedFib = (*appliedState)
                       ->getFibs()
                       ->getNode(id)
                       ->template getFib<AddressT>()
                       ->modify(id, appliedState);
  if (oldRoute) {
    clonedFib->updateNode(oldRoute);
  } else {
    clonedFib->removeNode(newRoute);
  }
}

} // namespace facebook::fboss
