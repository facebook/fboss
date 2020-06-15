/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/logging/xlog.h>

#include <algorithm>

namespace facebook::fboss {

SaiNextHopGroupManager::SaiNextHopGroupManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiNextHopGroupHandle>
SaiNextHopGroupManager::incRefOrAddNextHopGroup(
    const RouteNextHopEntry::NextHopSet& swNextHops) {
  auto ins = handles_.refOrEmplace(swNextHops);
  std::shared_ptr<SaiNextHopGroupHandle> nextHopGroupHandle = ins.first;
  if (!ins.second) {
    return nextHopGroupHandle;
  }
  SaiNextHopGroupTraits::AdapterHostKey nextHopGroupAdapterHostKey;
  // Populate the set of rifId, IP pairs for the NextHopGroup's
  // AdapterHostKey, and a set of next hop ids to create members for
  // N.B.: creating a next hop group member relies on the next hop group
  // already existing, so we cannot create them inline in the loop (since
  // creating the next hop group requires going through all the next hops
  // to figure out the AdapterHostKey)
  for (const auto& swNextHop : swNextHops) {
    // Compute the sai id of the next hop's router interface
    InterfaceID interfaceId = swNextHop.intf();
    auto routerInterfaceHandle =
        managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
            interfaceId);
    if (!routerInterfaceHandle) {
      throw FbossError("Missing SAI router interface for ", interfaceId);
    }
    auto nhk = managerTable_->nextHopManager().getAdapterHostKey(
        folly::poly_cast<ResolvedNextHop>(swNextHop));
    nextHopGroupAdapterHostKey.insert(nhk);
  }

  // Create the NextHopGroup and NextHopGroupMembers
  auto& store = SaiStore::getInstance()->get<SaiNextHopGroupTraits>();
  SaiNextHopGroupTraits::CreateAttributes nextHopGroupAttributes{
      SAI_NEXT_HOP_GROUP_TYPE_ECMP};
  nextHopGroupHandle->nextHopGroup =
      store.setObject(nextHopGroupAdapterHostKey, nextHopGroupAttributes);
  NextHopGroupSaiId nextHopGroupId =
      nextHopGroupHandle->nextHopGroup->adapterKey();

  for (const auto& swNextHop : swNextHops) {
    auto resolvedNextHop = folly::poly_cast<ResolvedNextHop>(swNextHop);
    auto key = std::make_pair(nextHopGroupId, resolvedNextHop);
    auto result = managedNextHopGroupMembers_.refOrEmplace(
        key, managerTable_, nextHopGroupId, resolvedNextHop);
    nextHopGroupHandle->members_.push_back(result.first);
  }
  return nextHopGroupHandle;
}

ManagedNextHopGroupMember::ManagedNextHopGroupMember(
    SaiManagerTable* managerTable,
    SaiNextHopGroupTraits::AdapterKey nexthopGroupId,
    const ResolvedNextHop& nexthop) {
  managedNextHop_ = managerTable->nextHopManager().refOrEmplaceNextHop(nexthop);

  auto nextHopKey = managerTable->nextHopManager().getAdapterHostKey(nexthop);
  auto nextHopWeight = (nexthop.weight() == ECMP_WEIGHT ? 1 : nexthop.weight());
  if (auto* ipKey =
          std::get_if<SaiIpNextHopTraits::AdapterHostKey>(&nextHopKey)) {
    // make an IP subscriber
    auto managedNextHopGroupMember =
        std::make_shared<ManagedIpNextHopGroupMember>(
            nexthopGroupId, nextHopWeight, *ipKey);
    SaiObjectEventPublisher::getInstance()->get<SaiIpNextHopTraits>().subscribe(
        managedNextHopGroupMember);
    managedNextHopGroupMember_ = managedNextHopGroupMember;
  } else if (
      auto* mplsKey =
          std::get_if<SaiMplsNextHopTraits::AdapterHostKey>(&nextHopKey)) {
    // make an MPLS subscriber
    auto managedNextHopGroupMember =
        std::make_shared<ManagedMplsNextHopGroupMember>(
            nexthopGroupId, nextHopWeight, *mplsKey);
    SaiObjectEventPublisher::getInstance()
        ->get<SaiMplsNextHopTraits>()
        .subscribe(managedNextHopGroupMember);
    managedNextHopGroupMember_ = managedNextHopGroupMember;
  }
}
} // namespace facebook::fboss
