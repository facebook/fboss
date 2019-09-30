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
  std::vector<NextHopSaiId> nextHopIds;
  nextHopIds.reserve(swNextHops.size());
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
    SaiNextHopTraits::AdapterHostKey nhk{rifId, ip};
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
      nextHopIds.push_back(neighborHandle->nextHop->adapterKey());
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
  for (const auto& nextHopId : nextHopIds) {
    SaiNextHopGroupMemberTraits::AdapterHostKey memberAdapterHostKey{
        nextHopGroupId, nextHopId};
    SaiNextHopGroupMemberTraits::CreateAttributes memberAttributes{
        nextHopGroupId, nextHopId};
    nextHopGroupHandle->nextHopGroupMembers[nextHopId] =
        memberStore.setObject(memberAdapterHostKey, memberAttributes);
  }
  return nextHopGroupHandle;
}

void SaiNextHopGroupManager::unregisterNeighborResolutionHandling(
    const RouteNextHopEntry::NextHopSet& swNextHops) {
  for (const auto& swNextHop : swNextHops) {
    InterfaceID interfaceId = swNextHop.intf();
    auto routerInterfaceHandle =
        managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
            interfaceId);
    if (!routerInterfaceHandle) {
      XLOG(WARNING) << "Missing SAI router interface for " << interfaceId;
      continue;
    }
    folly::IPAddress ip = swNextHop.addr();
    auto switchId = managerTable_->switchManager().getSwitchSaiId();
    SaiNeighborTraits::NeighborEntry neighborEntry{
        switchId, routerInterfaceHandle->routerInterface->adapterKey(), ip};
    nextHopsByNeighbor_[neighborEntry].erase(swNextHops);
  }
}

void SaiNextHopGroupManager::handleResolvedNeighbor(
    const SaiNeighborTraits::NeighborEntry& neighborEntry,
    NextHopSaiId nextHopId) {
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

    auto& memberStore =
        SaiStore::getInstance()->get<SaiNextHopGroupMemberTraits>();
    auto nextHopGroupId = nextHopGroupHandle->nextHopGroup->adapterKey();
    SaiNextHopGroupMemberTraits::AdapterHostKey memberAdapterHostKey{
        nextHopGroupId, nextHopId};
    SaiNextHopGroupMemberTraits::CreateAttributes memberAttributes{
        nextHopGroupId, nextHopId};
    nextHopGroupHandle->nextHopGroupMembers[nextHopId] =
        memberStore.setObject(memberAdapterHostKey, memberAttributes);
  }
}

void SaiNextHopGroupManager::handleUnresolvedNeighbor(
    const SaiNeighborTraits::NeighborEntry& neighborEntry,
    NextHopSaiId nextHopId) {
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
    nextHopGroupHandle->nextHopGroupMembers.erase(nextHopId);
  }
}

} // namespace facebook::fboss
