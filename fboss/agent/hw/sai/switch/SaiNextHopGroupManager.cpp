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
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/logging/xlog.h>

#include <algorithm>

namespace facebook {
namespace fboss {

SaiNextHopGroupMember::SaiNextHopGroupMember(
    SaiApiTable* apiTable,
    const NextHopGroupApiParameters::MemberAttributes& attributes,
    const sai_object_id_t& switchId)
    : apiTable_(apiTable), attributes_(attributes) {
  auto& nextHopGroupApi = apiTable_->nextHopGroupApi();
  id_ = nextHopGroupApi.createMember(attributes_.attrs(), switchId);
}

SaiNextHopGroupMember::~SaiNextHopGroupMember() {
  auto& nextHopGroupApi = apiTable_->nextHopGroupApi();
  nextHopGroupApi.remove(id_);
}

bool SaiNextHopGroupMember::operator==(
    const SaiNextHopGroupMember& other) const {
  return attributes_ == other.attributes_;
}

bool SaiNextHopGroupMember::operator!=(
    const SaiNextHopGroupMember& other) const {
  return !(*this == other);
}

SaiNextHopGroup::SaiNextHopGroup(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const NextHopGroupApiParameters::Attributes& attributes,
    const RouteNextHopEntry::NextHopSet& swNextHops)
    : apiTable_(apiTable),
      managerTable_(managerTable),
      attributes_(attributes),
      swNextHops_(swNextHops) {
  auto& nextHopGroupApi = apiTable_->nextHopGroupApi();
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  id_ = nextHopGroupApi.create(attributes_.attrs(), switchId);
}

SaiNextHopGroup::~SaiNextHopGroup() {
  members_.clear();
  managerTable_->nextHopGroupManager().unregisterNeighborResolutionHandling(
      swNextHops_);
  auto& nextHopGroupApi = apiTable_->nextHopGroupApi();
  nextHopGroupApi.remove(id_);
}

bool SaiNextHopGroup::operator==(const SaiNextHopGroup& other) const {
  if (attributes_ != other.attributes_) {
    return false;
  }
  return members_ == other.members_;
}

bool SaiNextHopGroup::operator!=(const SaiNextHopGroup& other) const {
  return !(*this == other);
}

bool SaiNextHopGroup::empty() const {
  return members_.empty();
}

void SaiNextHopGroup::addMember(sai_object_id_t nextHopId) {
  NextHopGroupApiParameters::MemberAttributes attributes{{id_, nextHopId}};
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto member =
      std::make_unique<SaiNextHopGroupMember>(apiTable_, attributes, switchId);
  sai_object_id_t memberId = member->id();
  members_.emplace(std::make_pair(memberId, std::move(member)));
  memberIdMap_.emplace(std::make_pair(memberId, id_));
}

void SaiNextHopGroup::removeMember(sai_object_id_t nextHopId) {
  memberIdMap_.erase(nextHopId);
  members_.erase(nextHopId);
}

SaiNextHopGroupManager::SaiNextHopGroupManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiNextHopGroup>
SaiNextHopGroupManager::incRefOrAddNextHopGroup(
    const RouteNextHopEntry::NextHopSet& swNextHops) {
  NextHopGroupApiParameters::Attributes attrs{{SAI_NEXT_HOP_GROUP_TYPE_ECMP}};
  auto ins = nextHopGroups_.refOrEmplace(
      swNextHops, apiTable_, managerTable_, attrs, swNextHops);
  std::shared_ptr<SaiNextHopGroup> nextHopGroup = ins.first;
  if (!ins.second) {
    return nextHopGroup;
  }
  for (const auto& swNextHop : swNextHops) {
    InterfaceID interfaceId = swNextHop.intf();
    auto routerInterfaceHandle =
        managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
            interfaceId);
    if (!routerInterfaceHandle) {
      throw FbossError("Missing SAI router interface for ", interfaceId);
    }
    folly::IPAddress ip = swNextHop.addr();
    auto switchId = managerTable_->switchManager().getSwitchSaiId();
    NeighborApiParameters::EntryType neighborEntry{
        switchId, routerInterfaceHandle->routerInterface->adapterKey(), ip};
    nextHopsByNeighbor_[neighborEntry].insert(swNextHops);
    auto neighbor = managerTable_->neighborManager().getNeighbor(neighborEntry);
    if (!neighbor) {
      XLOG(INFO) << "L2 Unresolved neighbor for " << ip;
      continue;
    } else {
      nextHopGroup->addMember(neighbor->nextHopId());
    }
  }
  return nextHopGroup;
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
    NeighborApiParameters::EntryType neighborEntry{
        switchId, routerInterfaceHandle->routerInterface->adapterKey(), ip};
    nextHopsByNeighbor_[neighborEntry].erase(swNextHops);
  }
}

void SaiNextHopGroupManager::handleResolvedNeighbor(
    const NeighborApiParameters::EntryType& neighborEntry,
    sai_object_id_t nextHopId) {
  auto itr = nextHopsByNeighbor_.find(neighborEntry);
  if (itr == nextHopsByNeighbor_.end()) {
    XLOG(INFO) << "No next hop group using newly resolved neighbor "
               << neighborEntry.ip();
    return;
  }
  for (const auto& nextHopSet : itr->second) {
    SaiNextHopGroup* nextHopGroup = nextHopGroups_.get(nextHopSet);
    if (!nextHopGroup) {
      XLOG(WARNING)
          << "No next hop group using next hop set associated with newly "
          << "resolved neighbor " << neighborEntry.ip();
      continue;
    }
    nextHopGroup->addMember(nextHopId);
  }
}

void SaiNextHopGroupManager::handleUnresolvedNeighbor(
    const NeighborApiParameters::EntryType& neighborEntry,
    sai_object_id_t nextHopId) {
  auto itr = nextHopsByNeighbor_.find(neighborEntry);
  if (itr == nextHopsByNeighbor_.end()) {
    XLOG(DBG2) << "No next hop group using newly unresolved neighbor "
               << neighborEntry.ip();
    return;
  }
  for (const auto& nextHopSet : itr->second) {
    SaiNextHopGroup* nextHopGroup = nextHopGroups_.get(nextHopSet);
    if (!nextHopGroup) {
      XLOG(FATAL)
          << "No next hop group using next hop set associated with newly "
          << "unresolved neighbor " << neighborEntry.ip();
    }
    nextHopGroup->removeMember(nextHopId);
  }
}

} // namespace fboss
} // namespace facebook
