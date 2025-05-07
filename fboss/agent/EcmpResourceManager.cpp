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
  CHECK(!preUpdateState_.has_value());
  preUpdateState_ = PreUpdateState(mergedGroups_, nextHopGroup2Id_);

  uint32_t nonBackupEcmpGroupsCnt{0};
  std::for_each(
      nextHopGroupIdToInfo_.cbegin(),
      nextHopGroupIdToInfo_.cend(),
      [&nonBackupEcmpGroupsCnt](const auto& idAndInfo) {
        // TODO - account for merged groups
        nonBackupEcmpGroupsCnt +=
            idAndInfo.second.lock()->isBackupEcmpGroupType() ? 0 : 1;
      });
  InputOutputState inOutState(nonBackupEcmpGroupsCnt, delta);
  processRouteUpdates<folly::IPAddressV4>(delta, &inOutState);
  processRouteUpdates<folly::IPAddressV6>(delta, &inOutState);
  CHECK(!inOutState.out.empty());
  /*
   * We start out deltas with StateDelta(oldState, oldState) and then
   * process the routes. In case the first route itself causes a overflow
   * we will have a no-op first delta in the vector. If so prune it
   */
  if (inOutState.out.front().oldState() == inOutState.out.front().newState()) {
    inOutState.out.erase(inOutState.out.begin());
  }
  return std::move(inOutState.out);
}

std::set<EcmpResourceManager::NextHopGroupId>
EcmpResourceManager::createOptimalMergeGroupSet() {
  if (!compressionPenaltyThresholdPct_) {
    return {};
  }
  XLOG(FATAL) << " Merge group algorithm is a TODO";
}

EcmpResourceManager::InputOutputState::InputOutputState(
    uint32_t _nonBackupEcmpGroupsCnt,
    const StateDelta& _in)
    : nonBackupEcmpGroupsCnt(_nonBackupEcmpGroupsCnt) {
  /*
   * Note that for first StateDelta we push in.oldState() for both
   * old and new state in the first StateDelta, since we will process
   * and add/update/delete routes on top of the old state.
   */
  _in.oldState()->publish();
  _in.newState()->publish();
  out.emplace_back(_in.oldState(), _in.oldState());
}

template <typename AddrT>
void EcmpResourceManager::InputOutputState::addOrUpdateRoute(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    bool ecmpDemandExceeded) {
  if (ecmpDemandExceeded) {
    CHECK(
        newRoute->getForwardInfo().getOverrideEcmpSwitchingMode().has_value());
  }
  auto curStateDelta = getCurrentStateDelta();
  auto oldState = curStateDelta.newState();
  CHECK(oldState->isPublished());
  auto newState = oldState->clone();
  auto fib = newState->getFibs()->getNode(rid)->getFib<AddrT>()->modify(
      rid, &newState);
  auto existingRoute = fib->getRouteIf(newRoute->prefix());
  if (existingRoute) {
    fib->updateNode(newRoute);
  } else {
    fib->addNode(newRoute);
  }
  if (!ecmpDemandExceeded) {
    // Still within ECMP limits, replaced the current delta.
    // To do this, we need to do 2 things
    // - use the current delta's old state as a base for
    // new delta
    // - Replace the current (last in the list) delta with
    // StateDelta(out.back().oldState(), newState);
    oldState = out.back().oldState();
    out.pop_back();
  }
  newState->publish();
  out.emplace_back(oldState, newState);
}

template <typename AddrT>
void EcmpResourceManager::InputOutputState::deleteRoute(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& delRoute) {
  auto curStateDelta = getCurrentStateDelta();
  auto oldState = curStateDelta.newState();
  CHECK(oldState->isPublished());
  auto newState = oldState->clone();
  auto fib = newState->getFibs()->getNode(rid)->getFib<AddrT>()->modify(
      rid, &newState);
  fib->removeNode(delRoute);
  // replace current delta
  // To do this, we need to do 2 things
  // - use the current delta's old state as a base for
  // new delta
  // - Replace the current (last in the list) delta with
  // StateDelta(out.back().oldState(), newState);
  oldState = out.back().oldState();
  out.pop_back();
  newState->publish();
  out.emplace_back(oldState, newState);
}

template <typename AddrT>
std::shared_ptr<NextHopGroupInfo> EcmpResourceManager::ecmpGroupDemandExceeded(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& route,
    NextHops2GroupId::iterator nhops2IdItr,
    InputOutputState* inOutState) {
  auto mergeSet = createOptimalMergeGroupSet();
  CHECK(mergeSet.empty()) << "Merge algo is a TODO";
  CHECK(backupEcmpGroupType_.has_value());
  XLOG(DBG2) << " Ecmp group demand exceeded available resources"
             << " route: " << route->str();
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  bool inserted{false};
  std::tie(grpInfo, inserted) = nextHopGroupIdToInfo_.refOrEmplace(
      nhops2IdItr->second,
      nhops2IdItr->second,
      nhops2IdItr,
      true /*isBackupEcmpGroupType*/);
  CHECK(inserted);
  const auto& curForwardInfo = route->getForwardInfo();
  auto newForwardInfo = RouteNextHopEntry(
      curForwardInfo.normalizedNextHops(),
      curForwardInfo.getAdminDistance(),
      curForwardInfo.getCounterID(),
      curForwardInfo.getClassID(),
      backupEcmpGroupType_);
  auto newRoute = route->clone();
  newRoute->setResolved(std::move(newForwardInfo));
  newRoute->publish();
  inOutState->addOrUpdateRoute(rid, newRoute, true /*ecmpDemandExceeded*/);
  return grpInfo;
}

template <typename AddrT>
void EcmpResourceManager::routeAdded(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& added,
    InputOutputState* inOutState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(added->isResolved());
  CHECK(added->isPublished());
  CHECK_LE(inOutState->nonBackupEcmpGroupsCnt, maxEcmpGroups_);
  auto nhopSet = added->getForwardInfo().normalizedNextHops();
  auto [idItr, inserted] =
      nextHopGroup2Id_.insert({nhopSet, findNextAvailableId()});
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  if (inserted) {
    if (inOutState->nonBackupEcmpGroupsCnt == maxEcmpGroups_) {
      grpInfo = ecmpGroupDemandExceeded(rid, added, idItr, inOutState);
    } else {
      std::tie(grpInfo, inserted) = nextHopGroupIdToInfo_.refOrEmplace(
          idItr->second, idItr->second, idItr, false /*isBackupEcmpGroupType*/
      );
      CHECK(inserted);
      inOutState->addOrUpdateRoute(rid, added, false /* ecmpDemandExceeded*/);
      // New ECMP group added but limit is not exceeded yet
      ++inOutState->nonBackupEcmpGroupsCnt;
      XLOG(DBG2) << "Add Route: " << added->str()
                 << " primray ecmp group count incremented to: "
                 << inOutState->nonBackupEcmpGroupsCnt;
    }
  } else {
    XLOG(DBG4) << "Add route: " << added->str()
               << " primray ecmp group count unchanged: "
               << inOutState->nonBackupEcmpGroupsCnt;
    inOutState->addOrUpdateRoute(rid, added, false /* ecmpDemandExceeded*/);
    grpInfo = nextHopGroupIdToInfo_.ref(idItr->second);
  }
  CHECK(grpInfo);
  auto [pitr, pfxInserted] = prefixToGroupInfo_.insert(
      {added->prefix().toCidrNetwork(), std::move(grpInfo)});
  CHECK(pfxInserted);
  pitr->second->incRouteUsageCount();
  CHECK_GT(pitr->second->getRouteUsageCount(), 0);
  CHECK_LE(inOutState->nonBackupEcmpGroupsCnt, maxEcmpGroups_);
}

template <typename AddrT>
void EcmpResourceManager::routeDeleted(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& removed,
    InputOutputState* inOutState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(removed->isResolved());
  CHECK(removed->isPublished());
  inOutState->deleteRoute(rid, removed);
  NextHopGroupId groupId{kMinNextHopGroupId - 1};
  {
    auto pitr = prefixToGroupInfo_.find(removed->prefix().toCidrNetwork());
    CHECK(pitr != prefixToGroupInfo_.end());
    groupId = pitr->second->getID();
    prefixToGroupInfo_.erase(pitr);
  }
  auto groupInfo = nextHopGroupIdToInfo_.ref(groupId);
  if (!groupInfo) {
    if (!removed->getForwardInfo().getOverrideEcmpSwitchingMode().has_value()) {
      // Last reference to this ECMP group gone, check if this group was
      // of primary ECMP group type
      --inOutState->nonBackupEcmpGroupsCnt;
      XLOG(DBG2) << "Delete route: " << removed->str()
                 << " primray ecmp group count decremented to: "
                 << inOutState->nonBackupEcmpGroupsCnt;
    }
    nextHopGroup2Id_.erase(removed->getForwardInfo().normalizedNextHops());
  } else {
    XLOG(DBG2) << "Delete route: " << removed->str()
               << " primray ecmp group count unchanged: "
               << inOutState->nonBackupEcmpGroupsCnt;
    groupInfo->decRouteUsageCount();
    CHECK_GT(groupInfo->getRouteUsageCount(), 0);
  }
}

template <typename AddrT>
void EcmpResourceManager::processRouteUpdates(
    const StateDelta& delta,
    InputOutputState* inOutState) {
  forEachChangedRoute<AddrT>(
      delta,
      [this, inOutState](
          RouterID rid, const auto& oldRoute, const auto& newRoute) {
        if (!oldRoute->isResolved() && !newRoute->isResolved()) {
          return;
        }
        if (oldRoute->isResolved() && !newRoute->isResolved()) {
          routeDeleted(rid, oldRoute, inOutState);
          return;
        }
        if (!oldRoute->isResolved() && newRoute->isResolved()) {
          routeAdded(rid, newRoute, inOutState);
          return;
        }
        // Both old and new are resolve
        CHECK(oldRoute->isResolved() && newRoute->isResolved());
        if (oldRoute->getForwardInfo().normalizedNextHops() !=
            newRoute->getForwardInfo().normalizedNextHops()) {
          routeDeleted(rid, oldRoute, inOutState);
          routeAdded(rid, newRoute, inOutState);
        }
      },
      [this, inOutState](RouterID rid, const auto& newRoute) {
        if (newRoute->isResolved()) {
          routeAdded(rid, newRoute, inOutState);
        }
      },
      [this, inOutState](RouterID rid, const auto& oldRoute) {
        if (oldRoute->isResolved()) {
          routeDeleted(rid, oldRoute, inOutState);
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
  XLOG(DBG2) << " Update done";
  preUpdateState_.reset();
}

void EcmpResourceManager::updateFailed(const StateDelta& delta) {
  XLOG(DBG2) << " Update failed";
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
