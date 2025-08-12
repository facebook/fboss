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
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/StateDelta.h"

#include <limits>

namespace facebook::fboss {

const NextHopGroupInfo* EcmpResourceManager::getGroupInfo(
    RouterID rid,
    const folly::CIDRNetwork& nw) const {
  auto pitr = prefixToGroupInfo_.find({rid, nw});
  return pitr == prefixToGroupInfo_.end() ? nullptr : pitr->second.get();
}

EcmpResourceManager::EcmpResourceManager(
    uint32_t maxHwEcmpGroups,
    int compressionPenaltyThresholdPct,
    std::optional<cfg::SwitchingMode> backupEcmpGroupType,
    SwitchStats* stats)
    // We keep a buffer of 2 for transient increment in ECMP groups when
    // pushing updates down to HW
    : maxEcmpGroups_(
          maxHwEcmpGroups -
          FLAGS_ecmp_resource_manager_make_before_break_buffer),
      compressionPenaltyThresholdPct_(compressionPenaltyThresholdPct),
      backupEcmpGroupType_(backupEcmpGroupType),
      switchStats_(stats) {
  CHECK_GT(
      maxHwEcmpGroups, FLAGS_ecmp_resource_manager_make_before_break_buffer);
  validateCfgUpdate(compressionPenaltyThresholdPct_, backupEcmpGroupType_);

  if (switchStats_) {
    switchStats_->setPrimaryEcmpGroupsExhausted(false);
    switchStats_->setPrimaryEcmpGroupsCount(0);
    switchStats_->setBackupEcmpGroupsCount(0);
  }
}

std::vector<StateDelta> EcmpResourceManager::modifyState(
    const std::vector<StateDelta>& deltas) {
  // TODO: Handle list of deltas instead of single delta
  CHECK_EQ(deltas.size(), 1);
  return consolidate(*deltas.begin());
}

std::vector<StateDelta> EcmpResourceManager::consolidate(
    const StateDelta& delta) {
  CHECK(!preUpdateState_.has_value());
  auto makeRet = [](const StateDelta& in) {
    std::vector<StateDelta> deltas;
    deltas.emplace_back(in.oldState(), in.newState());
    return deltas;
  };
  // No relevant change return early
  if (delta.getFlowletSwitchingConfigDelta().getOld() ==
          delta.getFlowletSwitchingConfigDelta().getNew() &&
      DeltaFunctions::isEmpty(delta.getSwitchSettingsDelta()) &&
      DeltaFunctions::isEmpty(delta.getFibsDelta())) {
    return makeRet(delta);
  }

  preUpdateState_ =
      PreUpdateState(mergedGroups_, nextHopGroup2Id_, backupEcmpGroupType_);

  handleSwitchSettingsDelta(delta);
  auto switchingModeChangeResult = handleFlowletSwitchConfigDelta(delta);
  if (DeltaFunctions::isEmpty(delta.getFibsDelta())) {
    if (switchingModeChangeResult.has_value()) {
      return std::move(switchingModeChangeResult->out);
    }
    return makeRet(delta);
  }

  uint32_t nonBackupEcmpGroupsCnt{0};
  std::optional<InputOutputState> inOutState(
      std::move(switchingModeChangeResult));
  if (!inOutState.has_value()) {
    inOutState = InputOutputState(nonBackupEcmpGroupsCnt, delta);
  }
  std::for_each(
      nextHopGroupIdToInfo_.cbegin(),
      nextHopGroupIdToInfo_.cend(),
      [&nonBackupEcmpGroupsCnt](const auto& idAndInfo) {
        // TODO - account for merged groups
        nonBackupEcmpGroupsCnt +=
            idAndInfo.second.lock()->isBackupEcmpGroupType() ? 0 : 1;
      });
  inOutState->nonBackupEcmpGroupsCnt = nonBackupEcmpGroupsCnt;
  return consolidateImpl(delta, &(*inOutState));
}

std::vector<StateDelta> EcmpResourceManager::consolidateImpl(
    const StateDelta& delta,
    InputOutputState* inOutState) {
  processRouteUpdates(delta, inOutState);
  reclaimEcmpGroups(inOutState);
  CHECK(!inOutState->out.empty());
  if (!inOutState->updated) {
    /*
     * If inOutState was not updated, just return the original delta
     */
    inOutState->out.clear();
    inOutState->out.emplace_back(delta.oldState(), delta.newState());
  }
  if (switchStats_) {
    switchStats_->setPrimaryEcmpGroupsCount(inOutState->nonBackupEcmpGroupsCnt);
    auto backupEcmpGroupCount = backupEcmpGroupType_.has_value()
        ? nextHopGroup2Id_.size() - inOutState->nonBackupEcmpGroupsCnt
        : 0;
    switchStats_->setBackupEcmpGroupsCount(backupEcmpGroupCount);
    switchStats_->setPrimaryEcmpGroupsExhausted(backupEcmpGroupCount > 0);
  }
  return std::move(inOutState->out);
}

void EcmpResourceManager::reclaimEcmpGroups(InputOutputState* inOutState) {
  CHECK_LE(inOutState->nonBackupEcmpGroupsCnt, maxEcmpGroups_);
  auto canReclaim = maxEcmpGroups_ - inOutState->nonBackupEcmpGroupsCnt;
  if (!canReclaim) {
    XLOG(DBG2) << " Unable to reclaim any non primary groups";
    return;
  }
  XLOG(DBG2) << " Can reclaim : " << canReclaim << " non primary groups";
  std::unordered_set<NextHopGroupId> allBackupGroupIds;
  std::vector<std::shared_ptr<NextHopGroupInfo>> backupGroupsSorted;
  std::for_each(
      nextHopGroupIdToInfo_.begin(),
      nextHopGroupIdToInfo_.end(),
      [&backupGroupsSorted, &allBackupGroupIds](const auto& idAndGrpRef) {
        auto groupInfo = idAndGrpRef.second.lock();
        if (groupInfo->isBackupEcmpGroupType()) {
          backupGroupsSorted.push_back(groupInfo);
          allBackupGroupIds.insert(groupInfo->getID());
        }
      });
  if (backupGroupsSorted.empty()) {
    return;
  }
  XLOG(DBG2) << " Will reclaim : "
             << std::min(
                    canReclaim,
                    static_cast<uint32_t>(backupGroupsSorted.size()))
             << " non primary groups";
  std::unordered_set<NextHopGroupId> groupIdsToReclaim;
  if (allBackupGroupIds.size() > canReclaim) {
    // Sort groups by number of routes pointing to this group.
    std::sort(
        backupGroupsSorted.begin(),
        backupGroupsSorted.end(),
        [](const auto& lgroup, const auto& rgroup) {
          return lgroup->getRouteUsageCount() < rgroup->getRouteUsageCount();
        });
    int claimed = 0;
    for (auto gitr = backupGroupsSorted.rbegin();
         gitr != backupGroupsSorted.rend() && claimed < canReclaim;
         ++gitr, ++claimed) {
      groupIdsToReclaim.insert((*gitr)->getID());
    }
  } else {
    groupIdsToReclaim = std::move(allBackupGroupIds);
  }
  auto oldState = inOutState->out.back().newState();
  auto newState = oldState->clone();
  for (auto& [ridAndPfx, grpInfo] : prefixToGroupInfo_) {
    if (!groupIdsToReclaim.contains(grpInfo->getID())) {
      continue;
    }

    grpInfo->setIsBackupEcmpGroupType(false);
    auto updateFib = [](const auto& routePfx, auto fib) {
      auto route = fib->exactMatch(routePfx)->clone();
      const auto& curForwardInfo = route->getForwardInfo();
      auto newForwardInfo = RouteNextHopEntry(
          curForwardInfo.getNextHopSet(),
          curForwardInfo.getAdminDistance(),
          curForwardInfo.getCounterID(),
          curForwardInfo.getClassID());
      XLOG(DBG2) << " Reclaimed and moved : " << route->str()
                 << " to primary ECMP group type";
      route->setResolved(newForwardInfo);
      route->publish();
      fib->updateNode(route);
    };
    const auto& [rid, pfx] = ridAndPfx;
    if (pfx.first.isV6()) {
      auto fib6 =
          newState->getFibs()->getNode(rid)->getFibV6()->modify(rid, &newState);
      RoutePrefixV6 routePfx(pfx.first.asV6(), pfx.second);
      updateFib(routePfx, fib6);
    } else {
      auto fib4 =
          newState->getFibs()->getNode(rid)->getFibV4()->modify(rid, &newState);
      RoutePrefixV4 routePfx(pfx.first.asV4(), pfx.second);
      updateFib(routePfx, fib4);
    }
  }
  newState->publish();
  /*
   * We do reclaim on a separate delta since the space is
   * guaranteed to be freed up *only* after the previous deltas
   * were completely applied.
   * Consider a case where we have space for only 2 primary ECMP
   * groups. And we have the following state (say state0)
   * R0-> G0 (bkup)
   * R1-> G1
   * R2-> G2
   * Next we get a new state (say state1)
   * R0-> G0(bkup)
   * R1-> G1
   * R2-> G1
   * With our current approach, the first delta enqueued will be
   * StateDelta(state0, state1);
   * This will now trigger a reclaim, getting us to state2
   * R0 -> G0
   * R1 -> G1
   * R2 -> G1
   * Had we combined state1 and state2 and just sent down
   * a single delta StateDelta(state0, state2), we would have
   * overflowed the ECMP limit when processing R0
   */
  inOutState->out.emplace_back(oldState, newState);
  inOutState->nonBackupEcmpGroupsCnt += groupIdsToReclaim.size();
  inOutState->updated = true;
  XLOG(DBG2) << " Primary ECMP Groups after reclaim: "
             << inOutState->nonBackupEcmpGroupsCnt;
}

int EcmpResourceManager::ConsolidationInfo::maxPenalty() const {
  CHECK(!groupId2Penalty.empty());
  return std::max_element(
             groupId2Penalty.begin(),
             groupId2Penalty.end(),
             [](const auto& idAndPenaltOne, const auto& idAndPenaltTwo) {
               return idAndPenaltOne.second < idAndPenaltTwo.second;
             })
      ->second;
}

std::set<EcmpResourceManager::NextHopGroupId>
EcmpResourceManager::getOptimalMergeGroupSet() const {
  if (!compressionPenaltyThresholdPct_) {
    return {};
  }
  CHECK(!candidateMergeGroups_.empty());
  auto citr = std::min_element(
      candidateMergeGroups_.begin(),
      candidateMergeGroups_.end(),
      [](const auto& groupsAndInfoOne, const auto& groupsAndInfoTwo) {
        return groupsAndInfoOne.second.maxPenalty() <
            groupsAndInfoTwo.second.maxPenalty();
      });

  return citr->first;
}

EcmpResourceManager::InputOutputState::InputOutputState(
    uint32_t _nonBackupEcmpGroupsCnt,
    const StateDelta& _in,
    const PreUpdateState& _groupIdCache)
    : nonBackupEcmpGroupsCnt(_nonBackupEcmpGroupsCnt),
      groupIdCache(_groupIdCache) {
  /*
   * Note that for first StateDelta we push in.oldState() for both
   * old and new state in the first StateDelta, since we will process
   * and add/update/delete routes on top of the old state.
   */
  _in.oldState()->publish();
  _in.newState()->publish();
  /*
   * EcmpResourceManager builds new deltas by traversing the FIB delta and
   * checking for ECMP limit. We still need to honor any non-FIB changes
   * that came with the original delta. So in the base delta, start things
   * off from oldState, newState with Old state FIBs.
   * Then we traverse FIBs delta and build up new deltas.
   *
   * Typically we send FIB deltas on their own. With 2
   * exceptions
   *  - Config application
   *  - Warm boot.
   *
   * We could special case these but EcmpResourceManager
   * should not make assumptions about the kind of delta we get.
   *
   * Downside of this is that for the case where we have - only
   * FIBs delta + overflow on the first route. It does create a
   * extra empty delta in the vector of deltas.
   */
  auto newStateWithOldFibs = _in.newState()->clone();
  if (_in.oldState()->getFibs() && !_in.oldState()->getFibs()->empty()) {
    newStateWithOldFibs->resetForwardingInformationBases(
        _in.oldState()->getFibs());
    DCHECK(DeltaFunctions::isEmpty(
        StateDelta(_in.oldState(), newStateWithOldFibs).getFibsDelta()));
  } else {
    // Cater for when old state is empty - e.g. warmboot,
    // rollback
    auto mfib = std::make_shared<MultiSwitchForwardingInformationBaseMap>();
    for (const auto& [matcherStr, curMfib] :
         std::as_const(*_in.newState()->getFibs())) {
      HwSwitchMatcher matcher(matcherStr);
      for (const auto& [rid, _] : std::as_const(*curMfib)) {
        mfib->updateForwardingInformationBaseContainer(
            std::make_shared<ForwardingInformationBaseContainer>(RouterID(rid)),
            matcher);
      }
    }
    newStateWithOldFibs->resetForwardingInformationBases(std::move(mfib));
  }
  newStateWithOldFibs->publish();
  out.emplace_back(_in.oldState(), newStateWithOldFibs);
}

template <typename AddrT>
void EcmpResourceManager::InputOutputState::addOrUpdateRoute(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    bool ecmpDemandExceeded,
    bool addNewDelta) {
  auto curStateDelta = getCurrentStateDelta();
  auto oldState = curStateDelta.newState();
  CHECK(oldState->isPublished());
  auto newState = oldState->clone();
  auto fib = newState->getFibs()->getNode(rid)->getFib<AddrT>()->modify(
      rid, &newState);
  auto existingRoute = fib->getRouteIf(newRoute->prefix());
  if (existingRoute) {
    XLOG(DBG4) << " Updated existing route: " << newRoute->str()
               << " route points to backup ecmp type: "
               << (newRoute->getForwardInfo()
                           .getOverrideEcmpSwitchingMode()
                           .has_value()
                       ? "Y"
                       : "N");
    fib->updateNode(newRoute);
  } else {
    XLOG(DBG4) << " Added new route: " << newRoute->str()
               << " route points to backup ecmp type: "
               << (newRoute->getForwardInfo()
                           .getOverrideEcmpSwitchingMode()
                           .has_value()
                       ? "Y"
                       : "N");
    fib->addNode(newRoute);
  }
  if (!addNewDelta) {
    // Still working on the current, replaced the current delta.
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
std::shared_ptr<NextHopGroupInfo>
EcmpResourceManager::updateForwardingInfoAndInsertDelta(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& route,
    NextHops2GroupId::iterator nhops2IdItr,
    bool ecmpDemandExceeded,
    InputOutputState* inOutState) {
  auto grpInfo = nextHopGroupIdToInfo_.ref(nhops2IdItr->second);

  if (!grpInfo) {
    CHECK(ecmpDemandExceeded);
    bool insertedGrp{false};
    if (backupEcmpGroupType_.has_value()) {
      std::tie(grpInfo, insertedGrp) = nextHopGroupIdToInfo_.refOrEmplace(
          nhops2IdItr->second,
          nhops2IdItr->second,
          nhops2IdItr,
          true /*isBackupEcmpGroupType*/);
    } else {
      CHECK(compressionPenaltyThresholdPct_);
      auto mergeSet = getOptimalMergeGroupSet();
      CHECK(!mergeSet.empty())
          << "Ecmp overflow, but no candidates available for merge";
      auto citr = candidateMergeGroups_.find(mergeSet);
      CHECK(citr != candidateMergeGroups_.end());
      auto [mitr, mergedGroupsInerted] =
          mergedGroups_.insert({citr->first, citr->second});
      CHECK(mergedGroupsInerted);
      // Added to merged groups, no longer a candidate merge.
      pruneFromCandidateMerges(mergeSet);
      // Since this is a new group, it cannot be part of the
      // optimal merge groups (since it just being created).
      std::tie(grpInfo, insertedGrp) = nextHopGroupIdToInfo_.refOrEmplace(
          nhops2IdItr->second,
          nhops2IdItr->second,
          nhops2IdItr,
          false /*isBackupEcmpGroupType*/);
      /*
       * Since merged group will create a new ECMP group. We create a
       * single new delta for migrating all relvant prefixes to merged
       * group and adding the new group that got us to exceed the ECMP
       * limit
       */
      bool addNewDelta{true};
      for (auto& [ridAndPfx, pfxGrpInfo] : prefixToGroupInfo_) {
        if (!mergeSet.contains(pfxGrpInfo->getID())) {
          continue;
        }
        if (!pfxGrpInfo->hasOverrideNextHops()) {
          // Converting from primary group to merged group
          // decrement primary group count
          --inOutState->nonBackupEcmpGroupsCnt;
        }
        pfxGrpInfo->setMergedGroupInfoItr(mitr);
        auto newState = inOutState->out.back().newState();
        auto fib = newState->getFibs()->getNode(rid)->getFib<AddrT>();
        std::shared_ptr<Route<AddrT>> existingRoute;
        if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
          existingRoute = fib->getRouteIf(RoutePrefix<AddrT>(
              ridAndPfx.second.first.asV6(), ridAndPfx.second.second));
        } else {
          existingRoute = fib->getRouteIf(RoutePrefix<AddrT>(
              ridAndPfx.second.first.asV4(), ridAndPfx.second.second));
        }
        CHECK(existingRoute);
        updateForwardingInfoAndInsertDelta(
            rid,
            existingRoute,
            pfxGrpInfo,
            ecmpDemandExceeded,
            inOutState,
            addNewDelta);
        // Only the first prefix update needs to start a new delta.
        // Rest will just queue updates on that same delta
        addNewDelta &= false;
      }
      // We added one merged group, so increment nonBackupEcmpGroupsCnt
      ++inOutState->nonBackupEcmpGroupsCnt;
    }
    CHECK(insertedGrp);
  } else if (compressionPenaltyThresholdPct_) {
    // Bump up penalty for now referenced group
  }
  return updateForwardingInfoAndInsertDelta(
      rid, route, grpInfo, ecmpDemandExceeded, inOutState);
}

template <typename AddrT>
std::shared_ptr<NextHopGroupInfo>
EcmpResourceManager::updateForwardingInfoAndInsertDelta(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& route,
    std::shared_ptr<NextHopGroupInfo>& grpInfo,
    bool ecmpDemandExceeded,
    InputOutputState* inOutState,
    bool addNewDelta) {
  if (ecmpDemandExceeded && backupEcmpGroupType_) {
    // If ecmpDemandExceeded and we have backupEcmpGroupType_ set,
    // then this group should spillover to backup ecmpType config.
    // For merge group mode, we are not guaranteed which groups
    // will be picked for merge, so we can't assert that the
    // new groups  will always be of merge type.
    CHECK(grpInfo->isBackupEcmpGroupType());
  }
  const auto& curForwardInfo = route->getForwardInfo();
  auto newForwardInfo = RouteNextHopEntry(
      curForwardInfo.normalizedNextHops(),
      curForwardInfo.getAdminDistance(),
      curForwardInfo.getCounterID(),
      curForwardInfo.getClassID(),
      backupEcmpGroupType_,
      std::optional<RouteNextHopSet>(grpInfo->getOverrideNextHops()));
  auto newRoute = route->clone();
  newRoute->setResolved(std::move(newForwardInfo));
  newRoute->publish();
  inOutState->addOrUpdateRoute(rid, newRoute, ecmpDemandExceeded, addNewDelta);
  inOutState->updated = true;
  return grpInfo;
}

std::vector<StateDelta> EcmpResourceManager::reconstructFromSwitchState(
    const std::shared_ptr<SwitchState>& curState) {
  if (!preUpdateState_.has_value()) {
    preUpdateState_ = PreUpdateState();
  }
  /*
   * TODO - when we goto merged groups, cur state will no longer
   * be efficient for rebuilding state. As for merged, nhops we will
   * endup needing to look up all combinations of individual groups
   * and find merges. So we will need to store more state around
   * warm boots.
   */
  // Clear state which needs to be resored from given state
  nextHopGroup2Id_.clear();
  mergedGroups_.clear();
  prefixToGroupInfo_.clear();
  nextHopGroupIdToInfo_.clear();
  candidateMergeGroups_.clear();
  /*
   * Restore state from previous input state. To do this we simply
   * replay the state in consolidate. Input state has a guarantee
   * that that
   * Num nhop groups of primary type <= maxEcmpGroups and there
   * may or may not be groups of backupEcmpType
   * Now there are a few sub cases for input state
   * i. No backup ecmp nhop groups and 0 or more primary ECMP group types.
   * This is simple, consolidate will simply run through the routes and
   * populate internal data structures
   * ii. Have both primary and backup ECMP route nhop groups. And primary
   * ECMP groups == maxEcmpGroups_. As we run through the routes, we will
   * see a mix of nhop groups, some that are of type primary and others
   * that are of type backupEcmpGroupType. We record them as such in our
   * data structures.  Note that in this replay routes may come in a
   * different order than when we originally got them. However since
   * we are not creating any new backup nhop groups (since
   * num nhop groups of primary type <= maxEcmpGroups) our data structures
   * should simply reflect the state in input state.
   * iii. Have both primary and backup ECMP route nhop groups. And primary
   * ECMP groups < maxEcmpGroups_. This is similar to ii. except that we
   * will now be able to reclaim some of the backup nhop groups.
   * */
  StateDelta delta(std::make_shared<SwitchState>(), curState);
  InputOutputState inOutState(0, delta, *preUpdateState_);
  auto deltas = consolidateImpl(delta, &inOutState);
  // LE 2, since reclaim always starts with a new delta
  CHECK_LE(deltas.size(), 2);
  StateDelta toRet(deltas.front().oldState(), deltas.back().newState());
  deltas.clear();
  deltas.emplace_back(std::move(toRet));
  return deltas;
}

template <typename AddrT>
bool EcmpResourceManager::routesEqual(
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::shared_ptr<Route<AddrT>>& newRoute) const {
  return oldRoute->getForwardInfo().normalizedNextHops() ==
      newRoute->getForwardInfo().normalizedNextHops() &&
      oldRoute->getForwardInfo().getOverrideEcmpSwitchingMode() ==
      newRoute->getForwardInfo().getOverrideEcmpSwitchingMode();
}

template <typename AddrT>
void EcmpResourceManager::routeAddedOrUpdated(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    InputOutputState* inOutState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(newRoute->isResolved());
  CHECK(newRoute->isPublished());
  CHECK_LE(inOutState->nonBackupEcmpGroupsCnt, maxEcmpGroups_);
  bool ecmpLimitReached = inOutState->nonBackupEcmpGroupsCnt == maxEcmpGroups_;
  if (oldRoute) {
    DCHECK(!routesEqual(oldRoute, newRoute));
    if (oldRoute->getForwardInfo().normalizedNextHops() !=
        newRoute->getForwardInfo().normalizedNextHops()) {
      /*
       * Update internal data structures only if nhops changes.
       * There are other route changes (e.g. classID, counterID)
       * which are no-op to us. If we delete the route here
       * we may endup deleting and recreating nhop group, which
       * is unnecessary.
       */
      routeDeleted(rid, oldRoute, true /*isUpdate*/, inOutState);
    }
  }
  auto nhopSet = newRoute->getForwardInfo().normalizedNextHops();
  auto [idItr, inserted] = nextHopGroup2Id_.insert(
      {nhopSet, findCachedOrNewIdForNhops(nhopSet, *inOutState)});
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  bool isBackupEcmpGroupType =
      newRoute->getForwardInfo().getOverrideEcmpSwitchingMode().has_value();
  if (inserted) {
    if (ecmpLimitReached && !isBackupEcmpGroupType) {
      /*
       * If ECMP limit is reached and route does not point to a backup
       * ecmp type nhop group, then update route forwarding info
       */
      XLOG(DBG2) << " Ecmp group demand exceeded available resources on: "
                 << (oldRoute ? "add" : "update")
                 << " route: " << newRoute->str();
      grpInfo = updateForwardingInfoAndInsertDelta(
          rid, newRoute, idItr, ecmpLimitReached, inOutState);
      // If new group is not of backup ecmp type, increment non
      // backup ecmp group count
      inOutState->nonBackupEcmpGroupsCnt +=
          grpInfo->isBackupEcmpGroupType() ? 0 : 1;
      XLOG(DBG2) << " Route  " << (oldRoute ? "update " : "add ")
                 << newRoute->str()
                 << " points to new group: " << grpInfo->getID()
                 << " primray ecmp group count "
                 << (grpInfo->isBackupEcmpGroupType() ? "unchanged: "
                                                      : "incremented to: ")
                 << inOutState->nonBackupEcmpGroupsCnt;
    } else {
      std::tie(grpInfo, inserted) = nextHopGroupIdToInfo_.refOrEmplace(
          idItr->second, idItr->second, idItr, isBackupEcmpGroupType);
      CHECK(inserted);
      inOutState->addOrUpdateRoute(
          rid, newRoute, false /* ecmpDemandExceeded*/);
      inOutState->nonBackupEcmpGroupsCnt += isBackupEcmpGroupType ? 0 : 1;
      XLOG(DBG2) << " Route: " << (oldRoute ? "update " : "add ")
                 << newRoute->str()
                 << " points to new group: " << grpInfo->getID()
                 << " primray ecmp group count incremented to: "
                 << inOutState->nonBackupEcmpGroupsCnt;
    }
  } else {
    // Route points to a existing group
    grpInfo = nextHopGroupIdToInfo_.ref(idItr->second);
    if (grpInfo->isBackupEcmpGroupType() && !isBackupEcmpGroupType) {
      auto existingGrpInfo = updateForwardingInfoAndInsertDelta(
          rid, newRoute, idItr, false /*ecmpLimitReached*/, inOutState);
      CHECK_EQ(existingGrpInfo, grpInfo);
    } else {
      inOutState->addOrUpdateRoute(rid, newRoute, false /*ecmpDemandExceeded*/);
    }
    XLOG(DBG4) << " Route  " << (oldRoute ? "update " : "add ")
               << " points to existing group: " << grpInfo->getID()
               << " primray ecmp group count unchanged: "
               << inOutState->nonBackupEcmpGroupsCnt;
  }
  CHECK(grpInfo);
  auto [pitr, pfxInserted] = prefixToGroupInfo_.insert(
      {{rid, newRoute->prefix().toCidrNetwork()}, std::move(grpInfo)});
  if (pfxInserted) {
    /*
     * If a new prefix points to this group increment ref count
     */
    pitr->second->incRouteUsageCount();
  }
  CHECK_GT(pitr->second->getRouteUsageCount(), 0);
  CHECK_LE(inOutState->nonBackupEcmpGroupsCnt, maxEcmpGroups_);
  if (compressionPenaltyThresholdPct_) {
    if (inserted) {
      /*
       * New group added, compute candidate merges
       * for it
       */
      computeCandidateMerges({idItr->second});
    } else if (pfxInserted) {
      /*
       * New prefix points to existing group
       * update consolidation penalties
       */
      updateConsolidationPenalty(*pitr->second);
    }
  }
}

template <typename AddrT>
void EcmpResourceManager::routeUpdated(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    InputOutputState* inOutState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(oldRoute->isResolved());
  CHECK(oldRoute->isPublished());
  CHECK(newRoute->isResolved());
  CHECK(newRoute->isPublished());
  const auto& oldNHops = oldRoute->getForwardInfo().normalizedNextHops();
  const auto& newNHops = newRoute->getForwardInfo().normalizedNextHops();
  if (oldNHops.size() > 1 && newNHops.size() > 1) {
    routeAddedOrUpdated(rid, oldRoute, newRoute, inOutState);
  } else if (newNHops.size() > 1) {
    // Old route was not pointing to a ECMP group
    // but newRoute is
    XLOG(DBG2) << " Route:" << newRoute->str()
               << " transitioned from single nhop to ECMP";
    routeAdded(rid, newRoute, inOutState);
  } else if (oldNHops.size() > 1) {
    // Old route was pointing to a ECMP group
    // but newRoute is not
    XLOG(DBG2) << " Route:" << newRoute->str()
               << " transitioned from ECMP to single nhop";
    routeDeleted(rid, oldRoute, false /*isUpdate*/, inOutState);
    // Just update deltas, no need to account for update route as a ECMP group
    // This and previous delete still create a single delta since ecmp demand
    // is never exceeded in these 2 steps
    inOutState->addOrUpdateRoute(rid, newRoute, false /*ecmpDemandExceeded*/);
  } else {
    // Neither of the routes point to > 1 nhops. Nothing to do
    CHECK_LE(oldNHops.size(), 1);
    CHECK_LE(newNHops.size(), 1);
    XLOG(DBG2) << " Route:" << newRoute->str()
               << " transitioned from single nhop to a different single nhop";
    // Just update deltas, no need to account for this as a ECMP group
    inOutState->addOrUpdateRoute(rid, newRoute, false /*ecmpDemandExceeded*/);
  }
}

template <typename AddrT>
void EcmpResourceManager::routeAdded(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    InputOutputState* inOutState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(newRoute->isResolved());
  CHECK(newRoute->isPublished());
  if (newRoute->getForwardInfo().normalizedNextHops().size() > 1) {
    routeAddedOrUpdated(
        rid, std::shared_ptr<Route<AddrT>>(), newRoute, inOutState);
  } else {
    // Just update deltas, no need to account for this as a ECMP group
    inOutState->addOrUpdateRoute(rid, newRoute, false /*ecmpDemandExceeded*/);
  }
}
template <typename AddrT>
void EcmpResourceManager::routeDeleted(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& removed,
    bool isUpdate,
    InputOutputState* inOutState) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(removed->isResolved());
  CHECK(removed->isPublished());
  const auto& routeNhops = removed->getForwardInfo().normalizedNextHops();
  if (routeNhops.size() <= 1) {
    // Just update deltas, no need to account for this as a ECMP group
    inOutState->deleteRoute(rid, removed);
    return;
  }
  if (!isUpdate) {
    /*
     * When route is deleted as part of a update we don't need
     * to queue this up as a separate delta in list of deltas
     * to be sent down to HW. The update will just be
     * accounted for in the delta queued by the updateRoute
     * flow
     */
    inOutState->deleteRoute(rid, removed);
  }
  NextHopGroupId groupId{kMinNextHopGroupId - 1};
  {
    auto pitr =
        prefixToGroupInfo_.find({rid, removed->prefix().toCidrNetwork()});
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
                 << inOutState->nonBackupEcmpGroupsCnt
                 << " Group ID: " << groupId << " removed";
    }
    nextHopGroup2Id_.erase(routeNhops);
    nextHopGroupDeleted(groupId);
  } else {
    decRouteUsageCount(*groupInfo);
    XLOG(DBG2) << "Delete route: " << removed->str()
               << " primray ecmp group count unchanged: "
               << inOutState->nonBackupEcmpGroupsCnt << " Group ID: " << groupId
               << " route usage count decremented to: "
               << groupInfo->getRouteUsageCount();
  }
}

void EcmpResourceManager::nextHopGroupDeleted(NextHopGroupId groupId) {
  if (candidateMergeGroups_.empty() && mergedGroups_.empty()) {
    return;
  }
  if (!pruneFromCandidateMerges({groupId})) {
    CHECK(pruneFromMergedGroups(groupId));
  }
}

bool EcmpResourceManager::pruneFromCandidateMerges(
    const NextHopGroupIds& groupIds) {
  bool pruned{false};
  auto citr = candidateMergeGroups_.begin();
  while (citr != candidateMergeGroups_.end()) {
    auto gitr = groupIds.begin();
    // Using search (logN) instead of intersection O(max(M, N)
    // since we expected the passed in groupIds to be
    // a very small set - or even a size of 1. This
    // makes search for individual groupIds more efficient.
    for (; gitr != groupIds.end(); ++gitr) {
      if (citr->first.contains(*gitr)) {
        citr = candidateMergeGroups_.erase(citr);
        pruned = true;
        break;
      }
    }
    if (gitr == groupIds.end()) {
      ++citr;
    }
  }
  return pruned;
}

bool EcmpResourceManager::pruneFromMergedGroups(NextHopGroupId /*groupId*/) {
  XLOG(FATAL) << " Prune API from merged groups is a TODO";
}

void EcmpResourceManager::decRouteUsageCount(NextHopGroupInfo& groupInfo) {
  groupInfo.decRouteUsageCount();
  XLOG(DBG2) << " GID: " << groupInfo.getID()
             << " route ref count decremented to : "
             << groupInfo.getRouteUsageCount();
  CHECK_GT(groupInfo.getRouteUsageCount(), 0);
  updateConsolidationPenalty(groupInfo);
}

void EcmpResourceManager::updateConsolidationPenalty(
    NextHopGroupInfo& groupInfo) {
  if (candidateMergeGroups_.empty() && mergedGroups_.empty()) {
    // Early return if no merged groups exist. Can be due to compression
    // threshold being 0 or if there is a single ECMP groups yet (so
    // nothing to merge with)
    return;
  }
  auto updatePenalty = [&groupInfo](auto& mergedGroups2Info) {
    const auto grpNhopsSize = groupInfo.getNhops().size();
    bool updated{false};
    for (auto& [mergedGroups, info] : mergedGroups2Info) {
      if (!mergedGroups.contains(groupInfo.getID())) {
        continue;
      }
      updated = true;
      auto citr = info.groupId2Penalty.find(groupInfo.getID());
      CHECK(citr != info.groupId2Penalty.end());
      auto nhopsLost = grpNhopsSize - info.mergedNhops.size();
      auto newPenalty = std::ceil((nhopsLost * 100.0) / grpNhopsSize) *
          groupInfo.getRouteUsageCount();
      XLOG(DBG2) << " GID: " << groupInfo.getID()
                 << " merge penalty for: " << folly::join(", ", mergedGroups)
                 << " prev penalty: " << citr->second
                 << " new penalty: " << newPenalty;
      citr->second = newPenalty;
    }
    return updated;
  };
  if (!updatePenalty(candidateMergeGroups_)) {
    XLOG(DBG2)
        << " Group: " << groupInfo.getID()
        << " not part of candidate merged groups, updating penalty in merged groups ";
    CHECK(updatePenalty(mergedGroups_));
  } else {
    XLOG(DBG2) << " Group: " << groupInfo.getID()
               << " penalty updated in candidate merged groups";
    DCHECK(!updatePenalty(mergedGroups_));
  }
}

void EcmpResourceManager::processRouteUpdates(
    const StateDelta& delta,
    InputOutputState* inOutState) {
  processFibsDeltaInHwSwitchOrder(
      delta,
      [this, inOutState](
          RouterID rid, const auto& oldRoute, const auto& newRoute) {
        if (!oldRoute->isResolved() && !newRoute->isResolved()) {
          return;
        }
        if (oldRoute->isResolved() && !newRoute->isResolved()) {
          routeDeleted(rid, oldRoute, false /*isUpdate*/, inOutState);
          return;
        }
        if (!oldRoute->isResolved() && newRoute->isResolved()) {
          routeAdded(rid, newRoute, inOutState);
          return;
        }
        // Both old and new are resolved
        CHECK(oldRoute->isResolved() && newRoute->isResolved());
        if (!routesEqual(oldRoute, newRoute)) {
          routeUpdated(rid, oldRoute, newRoute, inOutState);
        } else {
          // Nexthops and override group type did not change,
          // but the route changed. Just queue it in the delta,
          // no need to reevaluate ECMP resources
          inOutState->addOrUpdateRoute(
              rid, newRoute, false /*ecmpDemandExceeded*/);
        }
      },
      [this, inOutState](RouterID rid, const auto& newRoute) {
        if (newRoute->isResolved()) {
          routeAdded(rid, newRoute, inOutState);
        }
      },
      [this, inOutState](RouterID rid, const auto& oldRoute) {
        if (oldRoute->isResolved()) {
          routeDeleted(rid, oldRoute, false /*isUpdate*/, inOutState);
        }
      });
}

EcmpResourceManager::NextHopGroupId
EcmpResourceManager::findCachedOrNewIdForNhops(
    const RouteNextHopSet& nhops,
    const InputOutputState& inOutState) const {
  auto nitr = inOutState.groupIdCache.nextHopGroup2Id.find(nhops);
  return nitr != inOutState.groupIdCache.nextHopGroup2Id.end()
      ? nitr->second
      : findNextAvailableId();
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
void EcmpResourceManager::updateDone() {
  XLOG(DBG2) << " Update done";
  preUpdateState_.reset();
}

void EcmpResourceManager::updateFailed(
    const std::shared_ptr<SwitchState>& curState) {
  if (!preUpdateState_.has_value()) {
    return;
  }
  XLOG(DBG2) << " Update failed";
  CHECK(preUpdateState_.has_value());
  if (getBackupEcmpSwitchingMode() != preUpdateState_->backupEcmpGroupType) {
    // Throw if we get a failed update involving backup switching mode
    // change. We can make this smarter by
    // - Reverting backupEcmpGroupType_ setting
    // - Asserting that all prefixes in curState with overrideEcmpMode set
    // match the old backupEcmpGroupType
    // However this adds more code for a use case we don't need to support.
    // BackupEcmpType can only change via a config update state delta. And
    // if that fails, we anyways fail the application
    throw FbossError("Update failed with backup switching mode transition");
  }
  reconstructFromSwitchState(curState);
  preUpdateState_.reset();
}

std::optional<EcmpResourceManager::InputOutputState>
EcmpResourceManager::handleFlowletSwitchConfigDelta(const StateDelta& delta) {
  if (delta.getFlowletSwitchingConfigDelta().getOld() ==
      delta.getFlowletSwitchingConfigDelta().getNew()) {
    return std::nullopt;
  }
  std::optional<cfg::SwitchingMode> newMode;
  if (delta.newState()->getFlowletSwitchingConfig()) {
    newMode =
        delta.newState()->getFlowletSwitchingConfig()->getBackupSwitchingMode();
  }
  if (backupEcmpGroupType_.has_value() && !newMode.has_value()) {
    throw FbossError(
        "Cannot change backup ecmp switching mode from non-null to null");
  }
  if (backupEcmpGroupType_ == newMode) {
    return std::nullopt;
  }

  XLOG(DBG2) << "Updating backup switching mode from: "
             << (backupEcmpGroupType_.has_value()
                     ? apache::thrift::util::enumNameSafe(*backupEcmpGroupType_)
                     : "None")
             << " to: " << apache::thrift::util::enumNameSafe(*newMode);

  auto oldBackupEcmpMode = backupEcmpGroupType_;
  validateCfgUpdate(compressionPenaltyThresholdPct_, newMode);
  backupEcmpGroupType_ = newMode;
  if (!oldBackupEcmpMode.has_value()) {
    // No backup ecmp type value for old group.
    // Nothing to do.
    return std::nullopt;
  }
  InputOutputState inOutState(0 /*nonBackupEcmpGroupsCnt*/, delta);
  CHECK_EQ(inOutState.out.size(), 1);
  // Make changes on to current new state (which is essentially,
  // newState with old state's fibs). The first delta we will queue
  // will be the oldState's FIBs route's updated to new backup group.
  auto newState = inOutState.out.back().newState();
  bool changed = false;
  for (const auto& [ridAndPfx, grpInfo] : prefixToGroupInfo_) {
    if (!grpInfo->isBackupEcmpGroupType()) {
      continue;
    }
    // Got a route with backupEcmpType set. Change it.
    changed = true;
    const auto& [rid, pfx] = ridAndPfx;
    auto updateRouteOverridEcmpMode = [this, &inOutState](
                                          RouterID routerId,
                                          const auto& routePrefix,
                                          const auto& fib,
                                          auto groupInfo) mutable {
      auto route = fib->getRouteIf(routePrefix);
      CHECK(route);
      updateForwardingInfoAndInsertDelta(
          routerId,
          route,
          groupInfo,
          false /*ecmpDemandExceeded*/,
          &inOutState);
    };
    if (pfx.first.isV6()) {
      RoutePrefixV6 routePfx(pfx.first.asV6(), pfx.second);
      updateRouteOverridEcmpMode(
          rid,
          routePfx,
          newState->getFibs()->getNode(rid)->getFibV6(),
          grpInfo);
    } else {
      RoutePrefixV4 routePfx(pfx.first.asV4(), pfx.second);
      updateRouteOverridEcmpMode(
          rid,
          routePfx,
          newState->getFibs()->getNode(rid)->getFibV4(),
          grpInfo);
    }
  }
  return changed ? std::move(inOutState) : std::optional<InputOutputState>();
}

void EcmpResourceManager::handleSwitchSettingsDelta(const StateDelta& delta) {
  if (DeltaFunctions::isEmpty(delta.getSwitchSettingsDelta())) {
    return;
  }
  std::optional<int32_t> newEcmpCompressionThresholdPct;
  std::vector<std::optional<int32_t>> newEcmpCompressionThresholdPcts;
  for (const auto& [_, switchSettings] :
       std::as_const(*delta.newState()->getSwitchSettings())) {
    newEcmpCompressionThresholdPcts.emplace_back(
        switchSettings->getEcmpCompressionThresholdPct());
  }
  if (newEcmpCompressionThresholdPcts.size()) {
    newEcmpCompressionThresholdPct = *newEcmpCompressionThresholdPcts.begin();
    std::for_each(
        newEcmpCompressionThresholdPcts.begin(),
        newEcmpCompressionThresholdPcts.end(),
        [&newEcmpCompressionThresholdPct](
            const auto& ecmpCompressionThresholdPct) {
          if (ecmpCompressionThresholdPct != newEcmpCompressionThresholdPct) {
            throw FbossError(
                "All switches must have same ecmp compression threshold value");
          }
        });
  }
  if (newEcmpCompressionThresholdPct.value_or(0) ==
      compressionPenaltyThresholdPct_) {
    return;
  }
  /*
   * compressionPenaltyThresholdPct_ was already non-zero. Changing
   * is not allowed. Otherwise we may now get some routes that are out
   * of compliance
   */
  if (compressionPenaltyThresholdPct_ != 0) {
    throw FbossError(
        "Changing compression penalty threshold on the fly is not supported");
  }
  validateCfgUpdate(
      newEcmpCompressionThresholdPct.value_or(0), backupEcmpGroupType_);
  compressionPenaltyThresholdPct_ = newEcmpCompressionThresholdPct.value_or(0);
  if (compressionPenaltyThresholdPct_) {
    std::vector<NextHopGroupId> grpIds;
    std::for_each(
        nextHopGroupIdToInfo_.begin(),
        nextHopGroupIdToInfo_.end(),
        [&grpIds](const auto& grpIdAndInfo) {
          grpIds.emplace_back(grpIdAndInfo.first);
        });
    computeCandidateMerges(grpIds);
  }
}

void EcmpResourceManager::validateCfgUpdate(
    int32_t compressionPenaltyThresholdPct,
    const std::optional<cfg::SwitchingMode>& backupEcmpGroupType) const {
  if (compressionPenaltyThresholdPct && backupEcmpGroupType.has_value()) {
    throw FbossError(
        "Setting both compression threshold pct and backup ecmp group type is not supported");
  }
}

EcmpResourceManager::ConsolidationInfo
EcmpResourceManager::computeConsolidationInfo(
    const NextHopGroupIds& grpIds) const {
  CHECK_GE(grpIds.size(), 2);
  auto firstGrpInfo = nextHopGroupIdToInfo_.ref(*grpIds.begin());
  CHECK(firstGrpInfo);

  RouteNextHopSet mergedNhops(firstGrpInfo->getNhops());
  for (auto grpIdsItr = ++grpIds.begin(); grpIdsItr != grpIds.end();
       ++grpIdsItr) {
    RouteNextHopSet tmpMergeNhops;
    auto nhopsInfo = nextHopGroupIdToInfo_.ref(*grpIdsItr);
    CHECK(nhopsInfo);
    const auto& grpNhops = nhopsInfo->getNhops();
    std::set_intersection(
        mergedNhops.begin(),
        mergedNhops.end(),
        grpNhops.begin(),
        grpNhops.end(),
        std::inserter(tmpMergeNhops, tmpMergeNhops.begin()));
    mergedNhops = std::move(tmpMergeNhops);
  }
  ConsolidationInfo consolidationInfo;
  XLOG(DBG2) << " Computing consolidation penaties for: "
             << folly::join(", ", grpIds);
  for (auto grpId : grpIds) {
    const auto& grpInfo = nextHopGroupIdToInfo_.ref(grpId);
    CHECK_GE(grpInfo->getNhops().size(), mergedNhops.size());
    auto nhopsLost = grpInfo->getNhops().size() - mergedNhops.size();
    auto nhopsPctLoss =
        std::ceil((nhopsLost * 100.0) / grpInfo->getNhops().size());
    auto penalty = grpInfo->getRouteUsageCount() * nhopsPctLoss;
    XLOG(DBG2) << " For group : " << grpId
               << " orig nhops: " << grpInfo->getNhops().size()
               << " nhops lost: " << nhopsLost << " penalty: " << penalty
               << "%";
    consolidationInfo.groupId2Penalty.insert({grpId, penalty});
  }
  consolidationInfo.mergedNhops = std::move(mergedNhops);
  return consolidationInfo;
}

EcmpResourceManager::GroupIds2ConsolidationInfo
EcmpResourceManager::getConsolidationInfo(NextHopGroupId grpId) const {
  std::map<NextHopGroupIds, ConsolidationInfo> mergedGrps2Info;
  auto addMergedGroups = [&mergedGrps2Info, grpId](const auto& mergedGrpInfo) {
    for (const auto& [mergedGrps, info] : mergedGrpInfo) {
      if (mergedGrps.contains(grpId)) {
        mergedGrps2Info.insert({mergedGrps, info});
      }
    }
  };
  addMergedGroups(candidateMergeGroups_);
  addMergedGroups(mergedGroups_);
  return mergedGrps2Info;
}

void EcmpResourceManager::computeCandidateMerges(
    const std::vector<NextHopGroupId>& groupIds) {
  XLOG(DBG2) << " Will compute candidate merges for : "
             << folly::join(", ", groupIds);
  NextHopGroupIds alreadyMergedGroups;
  std::for_each(
      mergedGroups_.begin(),
      mergedGroups_.end(),
      [&alreadyMergedGroups](const auto& mergedGroupsAndPenalty) {
        alreadyMergedGroups.insert(
            mergedGroupsAndPenalty.first.begin(),
            mergedGroupsAndPenalty.first.end());
      });
  for (auto grpId : groupIds) {
    if (alreadyMergedGroups.contains(grpId)) {
      continue;
    }
    for (const auto& [grpToMergeWith, _] : nextHopGroupIdToInfo_) {
      if (grpToMergeWith == grpId) {
        continue;
      }
      NextHopGroupIds candidateMerge{grpId, grpToMergeWith};
      auto consolidationInfo = computeConsolidationInfo(candidateMerge);
      candidateMergeGroups_.insert(
          {candidateMerge, std::move(consolidationInfo)});
    }
    for (const auto& [grpsToMergeWith, _] : mergedGroups_) {
      DCHECK(!grpsToMergeWith.contains(grpId));
      // TODO: compute consolidation penalty
    }
  }
}

std::map<
    EcmpResourceManager::NextHopGroupId,
    std::set<EcmpResourceManager::Prefix>>
EcmpResourceManager::getGroupIdToPrefix() const {
  std::map<NextHopGroupId, std::set<Prefix>> toRet;
  std::for_each(
      prefixToGroupInfo_.begin(),
      prefixToGroupInfo_.end(),
      [&toRet](const auto& prefixAndGrpInfo) {
        toRet[prefixAndGrpInfo.second->getID()].insert(prefixAndGrpInfo.first);
      });
  return toRet;
}

std::unique_ptr<EcmpResourceManager> makeEcmpResourceManager(
    const std::shared_ptr<SwitchState>& state,
    const HwAsic* asic,
    SwitchStats* stats) {
  std::unique_ptr<EcmpResourceManager> ecmpResourceManager = nullptr;
  auto maxEcmpGroups = FLAGS_flowletSwitchingEnable
      ? asic->getMaxDlbEcmpGroups()
      : asic->getMaxEcmpGroups();
  std::optional<cfg::SwitchingMode> switchingMode;
  std::optional<int32_t> ecmpCompressionPenaltyThresholPct;
  if (auto flowletSwitchingConfig = state->getFlowletSwitchingConfig()) {
    switchingMode = flowletSwitchingConfig->getBackupSwitchingMode();
  }
  if (auto switchId = asic->getSwitchId()) {
    const auto& switchSettings = state->getSwitchSettings()->getSwitchSettings(
        HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(*switchId)})));
    if (switchSettings) {
      ecmpCompressionPenaltyThresholPct =
          switchSettings->getEcmpCompressionThresholdPct();
    }
  }
  if (maxEcmpGroups.has_value()) {
    auto percentage = FLAGS_flowletSwitchingEnable
        ? FLAGS_ars_resource_percentage
        : FLAGS_ecmp_resource_percentage;
    auto maxEcmps =
        std::floor(*maxEcmpGroups * static_cast<double>(percentage) / 100.0);
    XLOG(DBG2) << " Creating ecmp resource manager with max ECMP groups: "
               << maxEcmps << " and backup group type: "
               << (switchingMode.has_value()
                       ? apache::thrift::util::enumNameSafe(*switchingMode)
                       : "None");

    ecmpResourceManager = std::make_unique<EcmpResourceManager>(
        maxEcmps,
        ecmpCompressionPenaltyThresholPct.value_or(0),
        switchingMode,
        stats);
  }
  return ecmpResourceManager;
}

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::ConsolidationInfo& info) {
  std::stringstream ss;
  ss << "Nhops: " << info.mergedNhops << std::endl;
  ss << " Penalties:  " << std::endl;
  for (const auto& [gid, penalty] : info.groupId2Penalty) {
    ss << " gid:  " << gid << " penalty: " << penalty << std::endl;
  }
  os << ss.str();
  return os;
}
} // namespace facebook::fboss
