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
  deltas.emplace_back(StateDelta(delta.oldState(), delta.newState()));
  return deltas;
}

std::set<EcmpResourceManager::NextHopGroupId>
EcmpResourceManager::createOptimalMergeGroupSet() {
  if (!compressionPenaltyThresholdPct_) {
    return {};
  }
  XLOG(FATAL) << " Merge group algorithm is a TODO";
}

std::shared_ptr<SwitchState>
EcmpResourceManager::InputOutputState::nextDeltaOldSwitchState() const {
  if (out.empty()) {
    return in.oldState();
  }
  return out.back().newState();
}

template <typename AddrT>
std::shared_ptr<NextHopGroupInfo> EcmpResourceManager::ecmpGroupDemandExceeded(
    const std::shared_ptr<Route<AddrT>>& route,
    NextHops2GroupId::iterator nhops2IdItr,
    InputOutputState* inOutState) {
  auto mergeSet = createOptimalMergeGroupSet();
  CHECK(mergeSet.empty()) << "Merge algo is a TODO";
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  bool inserted{false};
  std::tie(grpInfo, inserted) = nextHopGroupIdToInfo_.refOrEmplace(
      nhops2IdItr->second,
      nhops2IdItr->second,
      nhops2IdItr,
      true /*isBackupEcmpGroupType*/);
  // TODO update route and create new state delta vector
  CHECK(inserted);
  return grpInfo;
}

template <typename AddrT>
void EcmpResourceManager::routeAdded(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& added,
    InputOutputState* inOutState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(added->isResolved());
  CHECK_LE(inOutState->nonBackupEcmpGroupsCnt, maxEcmpGroups_);
  auto nhopSet = added->getForwardInfo().getNextHopSet();
  auto [idItr, inserted] =
      nextHopGroup2Id_.insert({nhopSet, findNextAvailableId()});
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  if (inserted) {
    if (inOutState->nonBackupEcmpGroupsCnt == maxEcmpGroups_) {
      grpInfo = ecmpGroupDemandExceeded(added, idItr, inOutState);
    } else {
      std::tie(grpInfo, inserted) = nextHopGroupIdToInfo_.refOrEmplace(
          idItr->second, idItr->second, idItr, false /*isBackupEcmpGroupType*/
      );
      ++inOutState->nonBackupEcmpGroupsCnt;
      CHECK(inserted);
    }
  } else {
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
    InputOutputState* /*inOutState*/) {
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
        if (oldRoute->getForwardInfo().getNextHopSet() !=
            newRoute->getForwardInfo().getNextHopSet()) {
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
