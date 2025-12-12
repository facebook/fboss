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

DECLARE_bool(intf_nbr_tables);

namespace facebook::fboss {

template <typename EntryClassT, typename NTableT>
void SwitchState::revertNewNeighborEntry(
    const std::shared_ptr<EntryClassT>& newEntry,
    const std::shared_ptr<EntryClassT>& oldEntry,
    std::shared_ptr<SwitchState>* appliedState) {
  using NeighborEntryT = std::
      conditional_t<std::is_same<NTableT, ArpTable>::value, ArpEntry, NdpEntry>;
  std::shared_ptr<NeighborEntryT> entry;

  const auto ip = newEntry->getIP();

  NTableT* neighborTablePtr;
  // In this call, we also modify appliedState
  if (FLAGS_intf_nbr_tables) {
    auto intf =
        (*appliedState)->getInterfaces()->getNode(newEntry->getIntfID());
    neighborTablePtr = intf->template getNeighborTable<NTableT>()->modify(
        newEntry->getIntfID(), appliedState);
  } else {
    // We are assuming here that vlan is equal to interface always.
    VlanID vlanId = static_cast<VlanID>(newEntry->getIntfID());
    auto vlan = (*appliedState)->getVlans()->getNode(vlanId);
    neighborTablePtr = vlan->template getNeighborTable<NTableT>()->modify(
        vlanId, appliedState);
  }

  // Check that the entry exists
  entry = neighborTablePtr->getNodeIf(ip.str());
  CHECK(entry);

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
