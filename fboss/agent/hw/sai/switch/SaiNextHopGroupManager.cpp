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

SaiNextHopGroupMembership::SaiNextHopGroupMembership(
    SaiNextHopGroupTraits::AdapterKey groupId,
    const ResolvedNextHop& nexthop)
    : groupId_(groupId), nexthop_(nexthop) {}

void SaiNextHopGroupMembership::joinNextHopGroup(
    SaiManagerTable* managerTable) {
  saiNextHop_ = managerTable->nextHopManager().refOrEmplace(nexthop_);
  CHECK(saiNextHop_.has_value()) << "failed to get next hop";
  auto nexthopId = std::visit(
      [](const auto& arg) { return arg->adapterKey(); }, saiNextHop_.value());

  SaiNextHopGroupMemberTraits::AdapterHostKey memberAdapterHostKey{groupId_,
                                                                   nexthopId};
  SaiNextHopGroupMemberTraits::CreateAttributes memberAttributes{
      groupId_,
      nexthopId,
      (nexthop_.weight() == ECMP_WEIGHT ? 1 : nexthop_.weight())};
  auto& memberStore =
      SaiStore::getInstance()->get<SaiNextHopGroupMemberTraits>();
  member_ = memberStore.setObject(memberAdapterHostKey, memberAttributes);
}

void SaiNextHopGroupMembership::leaveNextHopGroup() {
  member_.reset();
  saiNextHop_.reset();
}

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
    neighbor2NextHops.emplace_back(
        neighborEntry, folly::poly_cast<ResolvedNextHop>(swNextHop));
  }

  // Create the NextHopGroup and NextHopGroupMembers
  auto& store = SaiStore::getInstance()->get<SaiNextHopGroupTraits>();
  SaiNextHopGroupTraits::CreateAttributes nextHopGroupAttributes{
      SAI_NEXT_HOP_GROUP_TYPE_ECMP};
  nextHopGroupHandle->nextHopGroup =
      store.setObject(nextHopGroupAdapterHostKey, nextHopGroupAttributes);
  NextHopGroupSaiId nextHopGroupId =
      nextHopGroupHandle->nextHopGroup->adapterKey();
  for (const auto& neighbor2NextHop : neighbor2NextHops) {
    const auto& neighborEntry = neighbor2NextHop.first;
    const auto& nexthop = neighbor2NextHop.second;
    auto membership =
        std::make_shared<SaiNextHopGroupMembership>(nextHopGroupId, nexthop);
    nextHopGroupHandle->neighbor2Memberships[neighborEntry].push_back(
        membership);
  }

  for (auto neighbor2MembershipsEntry :
       nextHopGroupHandle->neighbor2Memberships) {
    auto neighborHandle = managerTable_->neighborManager().getNeighborHandle(
        neighbor2MembershipsEntry.first);
    if (!neighborHandle) {
      XLOG(INFO) << "L2 Unresolved neighbor for "
                 << neighbor2MembershipsEntry.first.ip();
      continue;
    }
    for (auto membership : neighbor2MembershipsEntry.second) {
      membership->joinNextHopGroup(managerTable_);
    }
  }
  return nextHopGroupHandle;
}

void SaiNextHopGroupManager::handleResolvedNeighbor(
    const SaiNeighborTraits::NeighborEntry& neighborEntry) {
  for (auto itr : handles_) {
    auto groupHandle = itr.second.lock();
    auto neighborIter = groupHandle->neighbor2Memberships.find(neighborEntry);
    if (neighborIter == groupHandle->neighbor2Memberships.end()) {
      continue;
    }
    for (auto membership : groupHandle->neighbor2Memberships[neighborEntry]) {
      membership->joinNextHopGroup(managerTable_);
    }
  }
}

void SaiNextHopGroupManager::handleUnresolvedNeighbor(
    const SaiNeighborTraits::NeighborEntry& neighborEntry) {
  for (auto itr : handles_) {
    auto groupHandle = itr.second.lock();
    auto neighborIter = groupHandle->neighbor2Memberships.find(neighborEntry);
    if (neighborIter == groupHandle->neighbor2Memberships.end()) {
      continue;
    }
    for (auto membership : groupHandle->neighbor2Memberships[neighborEntry]) {
      membership->leaveNextHopGroup();
    }
  }
}

} // namespace facebook::fboss
