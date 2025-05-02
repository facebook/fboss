/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/EcmpResourceManager.h"

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/StateDelta.h"

#include <gflags/gflags.h>
#include <limits>

namespace facebook::fboss {

std::vector<StateDelta> EcmpResourceManager::consolidate(
    const StateDelta& delta) {
  std::vector<StateDelta> deltas;
  CHECK(!preUpdateState_.has_value());
  preUpdateState_ = PreUpdateState(mergedGroups_, nextHopGroup2Id_);
  processRouteUpdates<folly::IPAddressV4>(delta);
  processRouteUpdates<folly::IPAddressV6>(delta);
  deltas.emplace_back(StateDelta(delta.oldState(), delta.newState()));
  return deltas;
}

template <typename AddrT>
void EcmpResourceManager::routeAdded(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& added) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(added->isResolved());
  auto nhopSet = added->getForwardInfo().getNextHopSet();
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  auto [idItr, inserted] =
      nextHopGroup2Id_.insert({nhopSet, findNextAvailableId()});
  if (inserted) {
    std::tie(grpInfo, inserted) =
        nextHopGroupIdToInfo_.refOrEmplace(idItr->second, idItr->second, idItr);
    CHECK(inserted);
  } else {
    grpInfo = nextHopGroupIdToInfo_.ref(idItr->second);
  }
  CHECK(grpInfo);
  auto [pitr, pfxInserted] = prefixToGroupInfo_.insert(
      {added->prefix().toCidrNetwork(), std::move(grpInfo)});
  CHECK(pfxInserted);
  pitr->second->incRouteUsageCount();
  CHECK_GT(pitr->second->getRouteUsageCount(), 0);
}

template <typename AddrT>
void EcmpResourceManager::routeDeleted(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& removed) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(removed->isResolved());
  NextHopGroupId groupId{kMinNextHopGroupId - 1};
  {
    auto pitr = prefixToGroupInfo_.find(removed->prefix().toCidrNetwork());
    CHECK(pitr != prefixToGroupInfo_.end());
    groupId = pitr->second->getID();
    prefixToGroupInfo_.erase(pitr);
  }
  auto groupInfo = nextHopGroupIdToInfo_.ref(groupId);
  if (!groupInfo) {
    nextHopGroup2Id_.erase(removed->getForwardInfo().getNextHopSet());
  } else {
    groupInfo->decRouteUsageCount();
    CHECK_GT(groupInfo->getRouteUsageCount(), 0);
  }
}

template <typename AddrT>
void EcmpResourceManager::processRouteUpdates(const StateDelta& delta) {
  forEachChangedRoute<AddrT>(
      delta,
      [this](RouterID rid, const auto& oldRoute, const auto& newRoute) {
        if (!oldRoute->isResolved() && !newRoute->isResolved()) {
          return;
        }
        if (oldRoute->isResolved() && !newRoute->isResolved()) {
          routeDeleted(rid, oldRoute);
          return;
        }
        if (!oldRoute->isResolved() && newRoute->isResolved()) {
          routeAdded(rid, newRoute);
          return;
        }
        // Both old and new are resolve
        CHECK(oldRoute->isResolved() && newRoute->isResolved());
        if (oldRoute->getForwardInfo().getNextHopSet() !=
            newRoute->getForwardInfo().getNextHopSet()) {
          routeDeleted(rid, oldRoute);
          routeAdded(rid, newRoute);
        }
      },
      [this](RouterID rid, const auto& newRoute) {
        if (newRoute->isResolved()) {
          routeAdded(rid, newRoute);
        }
      },
      [this](RouterID rid, const auto& oldRoute) {
        if (oldRoute->isResolved()) {
          routeDeleted(rid, oldRoute);
        }
      });
}

EcmpResourceManager::NextHopGroupId EcmpResourceManager::findNextAvailableId()
    const {
  std::unordered_set<NextHopGroupId> allocatedIds;
  auto fillAllocatedIds = [&allocatedIds](const auto& nhopGroup2Id) {
    for (const auto& [_, id] : nhopGroup2Id) {
      allocatedIds.insert(id);
    }
  };
  CHECK(preUpdateState_.has_value());
  fillAllocatedIds(nextHopGroup2Id_);
  fillAllocatedIds(preUpdateState_->nextHopGroup2Id);
  for (auto start = kMinNextHopGroupId;
       start < std::numeric_limits<NextHopGroupId>::max();
       ++start) {
    if (allocatedIds.find(start) == allocatedIds.end()) {
      return start;
    }
  }
  throw FbossError("Unable to find id to allocate for new next hop group");
}

size_t EcmpResourceManager::getRouteUsageCount(NextHopGroupId nhopGrpId) const {
  auto grpInfo = nextHopGroupIdToInfo_.ref(nhopGrpId);
  if (grpInfo) {
    return grpInfo->getRouteUsageCount();
  }
  throw FbossError("Unable to find nhop group ID: ", nhopGrpId);
}
void EcmpResourceManager::updateDone(const StateDelta& /*delta*/) {
  preUpdateState_.reset();
}

void EcmpResourceManager::updateFailed(const StateDelta& delta) {
  CHECK(preUpdateState_.has_value());
  nextHopGroup2Id_ = preUpdateState_->nextHopGroup2Id;
  mergedGroups_ = preUpdateState_->mergedGroups;
  /* clear state which needs to be resored from previous state*/
  prefixToGroupInfo_.clear();
  nextHopGroupIdToInfo_.clear();
  candidateMergeGroups_.clear();
  preUpdateState_.reset();
  /* restore state from previous state*/
  consolidate(StateDelta(std::make_shared<SwitchState>(), delta.oldState()));
}
} // namespace facebook::fboss
