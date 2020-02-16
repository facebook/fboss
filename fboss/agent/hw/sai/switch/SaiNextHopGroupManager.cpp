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
  std::vector<std::pair<SaiNeighborTraits::NeighborEntry, ResolvedNextHop>>
      neighbor2NextHops;
  neighbor2NextHops.reserve(swNextHops.size());
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
    auto rifId = routerInterfaceHandle->routerInterface->adapterKey();
    folly::IPAddress ip = swNextHop.addr();
    SaiIpNextHopTraits::AdapterHostKey nhk{rifId, ip};
    nextHopGroupAdapterHostKey.insert(nhk);

    // Compute the neighbor that has the sai NextHop for this next hop
    auto switchId = managerTable_->switchManager().getSwitchSaiId();
    SaiNeighborTraits::NeighborEntry neighborEntry{
        switchId, routerInterfaceHandle->routerInterface->adapterKey(), ip};
    // Index this set of next hops by the neighbor for lookup on
    // resolved/unresolved
    nextHopsByNeighbor_[neighborEntry].insert(swNextHops);
    auto neighborHandle =
        managerTable_->neighborManager().getNeighborHandle(neighborEntry);
    if (!neighborHandle) {
      XLOG(INFO) << "L2 Unresolved neighbor for " << ip;
      continue;
    } else {
      // if the neighbor is resolved, save that neighbor's next hop id
      // for creating a next hop group member with
      neighbor2NextHops.emplace_back(
          neighborEntry, folly::poly_cast<ResolvedNextHop>(swNextHop));
    }
  }

  // Create the NextHopGroup and NextHopGroupMembers
  auto& store = SaiStore::getInstance()->get<SaiNextHopGroupTraits>();
  auto& memberStore =
      SaiStore::getInstance()->get<SaiNextHopGroupMemberTraits>();
  SaiNextHopGroupTraits::CreateAttributes nextHopGroupAttributes{
      SAI_NEXT_HOP_GROUP_TYPE_ECMP};
  nextHopGroupHandle->nextHopGroup =
      store.setObject(nextHopGroupAdapterHostKey, nextHopGroupAttributes);
  NextHopGroupSaiId nextHopGroupId =
      nextHopGroupHandle->nextHopGroup->adapterKey();
  for (const auto& neighbor2NextHop : neighbor2NextHops) {
    const auto& neighborEntry = neighbor2NextHop.first;
    const auto& nexthop = neighbor2NextHop.second;
    auto nexthopHandle = managerTable_->nextHopManager().refOrEmplace(nexthop);
    auto nextHopId = nexthopHandle->adapterKey();
    auto weight = nexthop.weight() == ECMP_WEIGHT ? 1 : nexthop.weight();
    SaiNextHopGroupMemberTraits::AdapterHostKey memberAdapterHostKey{
        nextHopGroupId, nextHopId};
    SaiNextHopGroupMemberTraits::CreateAttributes memberAttributes{
        nextHopGroupId, nextHopId, weight};
    auto member = memberStore.setObject(memberAdapterHostKey, memberAttributes);
    nextHopGroupHandle->nextHopGroupMembers[neighborEntry].push_back(
        std::make_shared<SaiNextHopGroupMemberHandle>(
            std::move(nexthopHandle), std::move(member)));
  }
  return nextHopGroupHandle;
}

void SaiNextHopGroupManager::handleResolvedNeighbor(
    const SaiNeighborTraits::NeighborEntry& neighborEntry) {
  auto itr = nextHopsByNeighbor_.find(neighborEntry);
  if (itr == nextHopsByNeighbor_.end()) {
    XLOG(INFO) << "No next hop group using newly resolved neighbor "
               << neighborEntry.ip();
    return;
  }
  for (const auto& nextHopSet : itr->second) {
    SaiNextHopGroupHandle* nextHopGroupHandle = handles_.get(nextHopSet);
    if (!nextHopGroupHandle) {
      XLOG(WARNING)
          << "No next hop group using next hop set associated with newly "
          << "resolved neighbor " << neighborEntry.ip();
      continue;
    }

    const auto& swNextHop = std::find_if(
        nextHopSet.begin(),
        nextHopSet.end(),
        [&neighborEntry](const auto& nextHop) {
          // TODO: try to use interface here as well as ip
          return nextHop.addr() == neighborEntry.ip();
        });

    auto& memberStore =
        SaiStore::getInstance()->get<SaiNextHopGroupMemberTraits>();
    auto nextHopGroupId = nextHopGroupHandle->nextHopGroup->adapterKey();
    const auto& weight =
        swNextHop->weight() == ECMP_WEIGHT ? 1 : swNextHop->weight();

    auto nexthopHandle = managerTable_->nextHopManager().refOrEmplace(
        folly::poly_cast<ResolvedNextHop>(*swNextHop));
    auto nextHopId = nexthopHandle->adapterKey();
    SaiNextHopGroupMemberTraits::AdapterHostKey memberAdapterHostKey{
        nextHopGroupId, nextHopId};
    SaiNextHopGroupMemberTraits::CreateAttributes memberAttributes{
        nextHopGroupId, nextHopId, weight};
    auto member = memberStore.setObject(memberAdapterHostKey, memberAttributes);
    nextHopGroupHandle->nextHopGroupMembers[neighborEntry].push_back(
        std::make_shared<SaiNextHopGroupMemberHandle>(
            std::move(nexthopHandle), std::move(member)));
  }
}

void SaiNextHopGroupManager::handleUnresolvedNeighbor(
    const SaiNeighborTraits::NeighborEntry& neighborEntry) {
  auto itr = nextHopsByNeighbor_.find(neighborEntry);
  if (itr == nextHopsByNeighbor_.end()) {
    XLOG(DBG2) << "No next hop group using newly unresolved neighbor "
               << neighborEntry.ip();
    return;
  }
  for (const auto& nextHopSet : itr->second) {
    SaiNextHopGroupHandle* nextHopGroupHandle = handles_.get(nextHopSet);
    if (!nextHopGroupHandle) {
      XLOG(FATAL)
          << "No next hop group using next hop set associated with newly "
          << "unresolved neighbor " << neighborEntry.ip();
    }
    nextHopGroupHandle->nextHopGroupMembers.erase(neighborEntry);
  }
}

} // namespace facebook::fboss
