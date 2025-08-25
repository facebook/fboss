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
namespace {

void updateRouteOverrides(
    const EcmpResourceManager::Prefix& ridAndPfx,
    std::shared_ptr<SwitchState>& newState,
    std::optional<cfg::SwitchingMode> backupSwitchingMode = std::nullopt,
    std::optional<EcmpResourceManager::GroupIds2ConsolidationInfoItr>
        mergeInfoItr = std::nullopt) {
  CHECK(!(backupSwitchingMode.has_value() && mergeInfoItr.has_value()));
  auto updateFib = [backupSwitchingMode, mergeInfoItr](
                       const auto& routePfx, auto fib) {
    auto route = fib->exactMatch(routePfx)->clone();
    const auto& curForwardInfo = route->getForwardInfo();
    std::optional<RouteNextHopSet> overrideNhops;
    if (mergeInfoItr) {
      overrideNhops = (*mergeInfoItr)->second.mergedNhops;
    }
    auto newForwardInfo = RouteNextHopEntry(
        curForwardInfo.getNextHopSet(),
        curForwardInfo.getAdminDistance(),
        curForwardInfo.getCounterID(),
        curForwardInfo.getClassID(),
        backupSwitchingMode,
        overrideNhops);
    CHECK(curForwardInfo.hasOverrideSwitchingModeOrNhops());
    XLOG(DBG2) << " Set : " << route->str() << " backup switching mode to : "
               << (backupSwitchingMode.has_value()
                       ? apache::thrift::util::enumNameSafe(
                             *backupSwitchingMode)
                       : "null")
               << " override next hops to : "
               << (overrideNhops.has_value()
                       ? folly::to<std::string>(*overrideNhops)
                       : "null");
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

EcmpResourceManager::GroupIds2ConsolidationInfo getConsolidationInfos(
    const EcmpResourceManager::GroupIds2ConsolidationInfo&
        gids2ConsolidationInfo,
    EcmpResourceManager::NextHopGroupId gid) {
  EcmpResourceManager::GroupIds2ConsolidationInfo mergedGrps2Info;
  for (const auto& [mergedGrps, info] : gids2ConsolidationInfo) {
    if (mergedGrps.contains(gid)) {
      mergedGrps2Info.insert({mergedGrps, info});
    }
  }
  return mergedGrps2Info;
}

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::NextHopGroupIds& gids) {
  os << "[" << folly::join(", ", gids) << "]";
  return os;
}

std::ostream& operator<<(
    std::ostream& os,
    const std::set<EcmpResourceManager::NextHopGroupIds>& gidSets) {
  std::stringstream ss;
  std::for_each(gidSets.begin(), gidSets.end(), [&ss](const auto& gids) {
    ss << gids << " ";
  });
  os << ss.str();
  return os;
}

bool pruneFromMergeGroupsImpl(
    const EcmpResourceManager::NextHopGroupIds& groupIdsToPrune,
    EcmpResourceManager::GroupIds2ConsolidationInfo& pruneFrom) {
  bool pruned{false};
  auto citr = pruneFrom.begin();
  while (citr != pruneFrom.end()) {
    auto gitr = groupIdsToPrune.begin();
    // Using search (logN) instead of intersection O(max(M, N)
    // since we expected the passed in groupIds to be
    // a very small set - or even a size of 1. This
    // makes search for individual groupIds more efficient.
    for (; gitr != groupIdsToPrune.end(); ++gitr) {
      if (citr->first.contains(*gitr)) {
        XLOG(DBG2) << " Pruning: " << citr->first;
        citr = pruneFrom.erase(citr);
        pruned = true;
        break;
      }
    }
    if (gitr == groupIdsToPrune.end()) {
      ++citr;
    }
  }
  return pruned;
}
} // namespace

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

  uint32_t unmergedGroups{0};
  std::set<const ConsolidationInfo*> mergedGroups;
  std::optional<InputOutputState> inOutState(
      std::move(switchingModeChangeResult));
  if (!inOutState.has_value()) {
    inOutState = InputOutputState(unmergedGroups, delta);
  }

  std::for_each(
      nextHopGroupIdToInfo_.cbegin(),
      nextHopGroupIdToInfo_.cend(),
      [&unmergedGroups, &mergedGroups](const auto& idAndInfo) {
        auto groupInfo = idAndInfo.second.lock();
        unmergedGroups += groupInfo->hasOverrides() ? 0 : 1;
        if (auto gitr = groupInfo->getMergedGroupInfoItr()) {
          mergedGroups.insert(&(*gitr)->second);
        }
      });
  inOutState->nonBackupEcmpGroupsCnt = unmergedGroups + mergedGroups.size();
  XLOG(DBG2) << " Start delta processing, primary group count: "
             << inOutState->nonBackupEcmpGroupsCnt << " with " << unmergedGroups
             << " unmerged groups and " << mergedGroups.size()
             << " merged groups";
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

std::vector<std::shared_ptr<NextHopGroupInfo>>
EcmpResourceManager::getGroupsToReclaimOrdered(uint32_t canReclaim) const {
  std::vector<std::shared_ptr<NextHopGroupInfo>> overrideGroupsSorted;
  std::for_each(
      nextHopGroupIdToInfo_.begin(),
      nextHopGroupIdToInfo_.end(),
      [&overrideGroupsSorted](const auto& idAndGrpRef) {
        auto groupInfo = idAndGrpRef.second.lock();
        if (groupInfo->hasOverrides()) {
          overrideGroupsSorted.push_back(groupInfo);
        }
      });
  if (overrideGroupsSorted.empty()) {
    return overrideGroupsSorted;
  }
  XLOG(DBG2) << " Will reclaim : "
             << std::min(
                    canReclaim,
                    static_cast<uint32_t>(overrideGroupsSorted.size()))
             << " non primary groups";
  // Reverse sort groups by cost. So higher cost groups come first
  std::sort(
      overrideGroupsSorted.begin(),
      overrideGroupsSorted.end(),
      [](const auto& lgroup, const auto& rgroup) {
        CHECK_EQ(
            lgroup->isBackupEcmpGroupType(), rgroup->isBackupEcmpGroupType());
        CHECK_EQ(lgroup->hasOverrideNextHops(), rgroup->hasOverrideNextHops());
        return lgroup->cost() > rgroup->cost();
      });
  if (backupEcmpGroupType_) {
    // Reclaiming each backup ECMP group just creates one more primary ECMP
    // group so we can reclaim as many backup groups as we have space for
    // (canReclaim)
    overrideGroupsSorted.resize(
        std::min(static_cast<size_t>(canReclaim), overrideGroupsSorted.size()));
  } else {
    /*
    Logic is
    - Start from the highest cost to lowest cost groups
    - Calculate unmerge cost, if it fits in the available space
    add it to ordered list of groups to unmerge.
    - Find all the other groups that are part of the merge set
    and add them to the ordered list as well. E.g. say we have
    a merge set of groups [1, 3] and [2, 4]. Both of which have
    a cost of say X. Then in our ordered (by cost) group we may see
    the groups in the following order [4, 3, 2, 1]. And say we have
    space to create 2 more groups. If we go in the above order
    order at step where we unmerge 3, we will have prefixes pointing
    to the following groups
    - groups  4, 3 and the original merged groups [1, 3] and [2, 4].
    At this point we already created 2 more groups so we can't proceed.
    - OTOH if we unmerge merged groups together. Viz. we go in the order
    (say) [4, 2, 3, 1] - we will be able to unmerge both and free
    up the merged groups. This also follows that we merge in unmerge
    a merge set together.
    */
    std::vector<std::shared_ptr<NextHopGroupInfo>> reclaimableMergedGroups;
    std::unordered_set<NextHopGroupId> reclaimedGroups;
    auto spaceRemaining = canReclaim;
    for (auto oitr = overrideGroupsSorted.begin();
         oitr != overrideGroupsSorted.end() && spaceRemaining;
         ++oitr) {
      const auto& overrideGroup = *oitr;
      if (reclaimedGroups.contains(overrideGroup->getID())) {
        // Already reclaimed continue
        continue;
      }
      auto mergeGrpInfoItr = (*oitr)->getMergedGroupInfoItr();
      CHECK(mergeGrpInfoItr.has_value());
      const auto& [mergeSet, _] = *mergeGrpInfoItr.value();
      CHECK_GT(mergeSet.size(), 0);
      // Merge set will create mergeSet.size() primary groups primary group and
      // delete one merged group
      auto curGroupDemand = mergeSet.size() - 1;
      if (curGroupDemand > spaceRemaining) {
        XLOG(DBG2) << " Cannot unmerge : " << mergeSet
                   << " demand: " << curGroupDemand
                   << " space remaining: " << spaceRemaining;
        continue;
      }
      spaceRemaining -= curGroupDemand;
      std::for_each(
          mergeSet.begin(),
          mergeSet.end(),
          [&reclaimedGroups, &reclaimableMergedGroups, this](auto gid) {
            reclaimedGroups.insert(gid);
            // We reclaim all groups that are part of a merge set together
            reclaimableMergedGroups.emplace_back(
                nextHopGroupIdToInfo_.ref(gid));
          });
    }
    overrideGroupsSorted = std::move(reclaimableMergedGroups);
  }
  return overrideGroupsSorted;
}

void EcmpResourceManager::reclaimBackupGroups(
    const std::vector<std::shared_ptr<NextHopGroupInfo>>& toReclaimSorted,
    const NextHopGroupIds& groupIdsToReclaim,
    InputOutputState* inOutState) {
  auto oldState = inOutState->out.back().newState();
  auto newState = oldState->clone();
  for (auto& [ridAndPfx, grpInfo] : prefixToGroupInfo_) {
    if (!groupIdsToReclaim.contains(grpInfo->getID())) {
      continue;
    }
    grpInfo->setIsBackupEcmpGroupType(false);
    updateRouteOverrides(ridAndPfx, newState);
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

void EcmpResourceManager::updateMergedGroups(
    const std::set<NextHopGroupIds>& mergeSetsToUpdate,
    const NextHopGroupIds& groupIdsToReclaimOrPruneIn,
    InputOutputState* inOutState) {
  XLOG(DBG2) << " Will reclaim the following merge groups: "
             << mergeSetsToUpdate;
  NextHopGroupIds groupIdsToReclaimOrPrune = groupIdsToReclaimOrPruneIn;
  std::unordered_map<NextHopGroupId, std::vector<Prefix>> gid2Prefix;
  std::for_each(
      prefixToGroupInfo_.begin(),
      prefixToGroupInfo_.end(),
      [&gid2Prefix](const auto& pfxAndGroupInfo) {
        gid2Prefix[pfxAndGroupInfo.second->getID()].push_back(
            pfxAndGroupInfo.first);
      });
  for (const auto& curMergeSet : mergeSetsToUpdate) {
    CHECK_GT(curMergeSet.size(), 1);
    /*
     * To allow for partial reclaims, we compute a newMergeSet.
     * newMergeSet is the set of groups in curMergeSet which
     * are not to be reclaimed or pruned
     */
    NextHopGroupIds newMergeSet;
    std::set_difference(
        curMergeSet.begin(),
        curMergeSet.end(),
        groupIdsToReclaimOrPrune.begin(),
        groupIdsToReclaimOrPrune.end(),
        std::inserter(newMergeSet, newMergeSet.begin()));
    auto oldState = inOutState->out.back().newState();
    auto newState = oldState->clone();
    /*
     * For groups to be reclaimed
     * - Clear mergeInfoItr
     * - Clear override nhops for prefixes pointing to such groups
     */
    for (auto mGid : curMergeSet) {
      if (!newMergeSet.contains(mGid)) {
        for (const auto& pfx : gid2Prefix[mGid]) {
          updateRouteOverrides(pfx, newState);
        }
        if (auto grpInfo = nextHopGroupIdToInfo_.ref(mGid)) {
          CHECK(grpInfo->getMergedGroupInfoItr().has_value());
          grpInfo->setMergedGroupInfoItr(std::nullopt);
        }
      }
    }
    /*
     * If newMergeSet.size() > 1
     * Create a new merge group and cache its iterator.
     * If newMergeSet.size() == 1
     * Reclaim the lone group remaining in merge group.
     * Update prefix nhops and group's mergeInfo iterators accordingly
     */
    std::optional<GroupIds2ConsolidationInfoItr> newMergeItr;
    if (newMergeSet.size() > 1) {
      XLOG(DBG2) << " Replacing merge group: " << curMergeSet << std::endl
                 << " with: " << newMergeSet;
      std::tie(newMergeItr, std::ignore) = mergedGroups_.insert(
          {newMergeSet, computeConsolidationInfo(newMergeSet)});
    } else if (newMergeSet.size() == 1) {
      XLOG(DBG2) << " Reclaiming merge group: " << curMergeSet << std::endl
                 << " since it has only one member group remaining: "
                 << newMergeSet;
      groupIdsToReclaimOrPrune.insert(*newMergeSet.begin());
    } else {
      XLOG(DBG2) << " Reclaiming merge group: " << curMergeSet;
    }
    for (auto mGid : newMergeSet) {
      for (const auto& pfx : gid2Prefix[mGid]) {
        updateRouteOverrides(pfx, newState, std::nullopt, newMergeItr);
      }
      if (auto grpInfo = nextHopGroupIdToInfo_.ref(mGid)) {
        CHECK(grpInfo->getMergedGroupInfoItr().has_value());
        grpInfo->setMergedGroupInfoItr(newMergeItr);
      }
    }

    newState->publish();
    // We put each unmerge on a new delta, to ensure that all
    // constitutent groups get unmerged and we reclaim the merged
    // group at the end of this processing.
    inOutState->out.emplace_back(oldState, newState);
    // Reclaimed curMergeSet.size() groups and deleted all references
    // to the merged group.
    inOutState->nonBackupEcmpGroupsCnt += curMergeSet.size() - 1;
    inOutState->updated = true;
  }
  pruneFromMergeGroupsImpl(groupIdsToReclaimOrPrune, mergedGroups_);
  // TODO remove pruned groups from groupIdsToReclaimOrPrune before
  // computingCandidateMerges
  computeCandidateMerges(
      groupIdsToReclaimOrPrune.begin(), groupIdsToReclaimOrPrune.end());
}

void EcmpResourceManager::reclaimMergeGroups(
    const std::vector<std::shared_ptr<NextHopGroupInfo>>& toReclaimOrdered,
    const NextHopGroupIds& groupIdsToReclaim,
    InputOutputState* inOutState) {
  std::set<NextHopGroupIds> mergeSetsToUpdate;
  std::for_each(
      toReclaimOrdered.begin(),
      toReclaimOrdered.end(),
      [&mergeSetsToUpdate](const auto& grpInfo) {
        auto curMergeGrpItr = grpInfo->getMergedGroupInfoItr();
        CHECK(curMergeGrpItr.has_value());
        const auto& curMergeSet = (*curMergeGrpItr)->first;
        mergeSetsToUpdate.insert(curMergeSet);
      });
  updateMergedGroups(mergeSetsToUpdate, groupIdsToReclaim, inOutState);
}

void EcmpResourceManager::reclaimEcmpGroups(InputOutputState* inOutState) {
  CHECK_LE(inOutState->nonBackupEcmpGroupsCnt, maxEcmpGroups_);
  auto canReclaim = maxEcmpGroups_ - inOutState->nonBackupEcmpGroupsCnt;
  if (!canReclaim) {
    XLOG(DBG2) << " Unable to reclaim any non primary groups";
    return;
  }
  XLOG(DBG2) << " Can reclaim : " << canReclaim << " non primary groups";
  auto overrideGroupsSorted = getGroupsToReclaimOrdered(canReclaim);
  if (overrideGroupsSorted.empty()) {
    XLOG(DBG2) << " No override groups available for reclaim";
    return;
  }
  NextHopGroupIds groupIdsToReclaim;
  std::for_each(
      overrideGroupsSorted.begin(),
      overrideGroupsSorted.end(),
      [&groupIdsToReclaim](
          const std::shared_ptr<const NextHopGroupInfo>& grpInfo) {
        groupIdsToReclaim.insert(grpInfo->getID());
      });
  XLOG(DBG2) << " Will reclaim prefixes pointing to groups: "
             << groupIdsToReclaim;
  if (backupEcmpGroupType_) {
    reclaimBackupGroups(overrideGroupsSorted, groupIdsToReclaim, inOutState);
  } else {
    reclaimMergeGroups(overrideGroupsSorted, groupIdsToReclaim, inOutState);
  }
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
      XLOG(DBG2) << "Merging : " << mergeSet << std::endl
                 << "Consolidation info: " << mitr->second;
      for (auto& [ridAndPfx, pfxGrpInfo] : prefixToGroupInfo_) {
        if (!mergeSet.contains(pfxGrpInfo->getID())) {
          continue;
        }
        XLOG(DBG2) << "Migrating : " << ridAndPfx << " to merged ECMP group";
        if (!pfxGrpInfo->hasOverrideNextHops()) {
          // Converting from primary group to merged group
          // decrement primary group count
          --inOutState->nonBackupEcmpGroupsCnt;
          XLOG(DBG2) << "Primary ecmp group decremented to: "
                     << inOutState->nonBackupEcmpGroupsCnt;
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
      XLOG(DBG2) << "Done migrating prefixes to merged group: " << mergeSet
                 << ". Incremented primary ecmp group count to : "
                 << inOutState->nonBackupEcmpGroupsCnt;
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
      curForwardInfo.getNextHopSet(),
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
  // LE 2, since reclaim always starts with a new delta. Note if
  // we currently don't have a use case of reclaiming merged
  // groups over state restore/WB. If in the future we do,
  // we will need to relax this check. Sine merge group
  // reclaim creates one delta for each merge set being reclaimed.
  CHECK_LE(deltas.size(), 2);
  StateDelta toRet(deltas.front().oldState(), deltas.back().newState());
  deltas.clear();
  deltas.emplace_back(std::move(toRet));
  return deltas;
}

template <typename AddrT>
bool EcmpResourceManager::routeFwdEqual(
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::shared_ptr<Route<AddrT>>& newRoute) const {
  return oldRoute->getForwardInfo().getNextHopSet() ==
      newRoute->getForwardInfo().getNextHopSet() &&
      oldRoute->getForwardInfo().getOverrideEcmpSwitchingMode() ==
      newRoute->getForwardInfo().getOverrideEcmpSwitchingMode() &&
      oldRoute->getForwardInfo().getOverrideNextHops() ==
      newRoute->getForwardInfo().getOverrideNextHops();
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
    DCHECK(!routeFwdEqual(oldRoute, newRoute));
    /*
     * We compare the non override normalized next hops. Since
     * those represent the original nhop group demand. Further
     * since the new route came in via state update, there are
     * going to be no override (merged) nhops for it
     */
    if (oldRoute->getForwardInfo().nonOverrideNormalizedNextHops() !=
        newRoute->getForwardInfo().nonOverrideNormalizedNextHops()) {
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
  auto nhopSet = newRoute->getForwardInfo().nonOverrideNormalizedNextHops();
  auto [idItr, grpInserted] = nextHopGroup2Id_.insert(
      {nhopSet, findCachedOrNewIdForNhops(nhopSet, *inOutState)});
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  bool hasOverrides =
      newRoute->getForwardInfo().hasOverrideSwitchingModeOrNhops();
  if (grpInserted) {
    if (ecmpLimitReached && !hasOverrides) {
      /*
       * If ECMP limit is reached and route does not point to a backup
       * ecmp type nhop group, then update route forwarding info
       */
      XLOG(DBG2) << " Ecmp group demand exceeded available resources on: "
                 << (oldRoute ? "add" : "update")
                 << " route: " << newRoute->str();
      grpInfo = updateForwardingInfoAndInsertDelta(
          rid, newRoute, idItr, ecmpLimitReached, inOutState);
      // If new group does not have override mode or nhops, increment non
      // backup ecmp group count
      inOutState->nonBackupEcmpGroupsCnt += grpInfo->hasOverrides() ? 0 : 1;
      XLOG(DBG2) << " Route  " << (oldRoute ? "update " : "add ")
                 << newRoute->str()
                 << " points to new group: " << grpInfo->getID()
                 << " primray ecmp group count "
                 << (grpInfo->hasOverrides() ? "unchanged: "
                                             : "incremented to: ")
                 << inOutState->nonBackupEcmpGroupsCnt;
    } else {
      std::optional<GroupIds2ConsolidationInfoItr> mergeGrpItr;
      bool newMergeGrpCreated{false};
      if (auto overrideNhops =
              newRoute->getForwardInfo().getOverrideNextHops()) {
        auto numMergeGroupsBefore = mergedGroups_.size();
        mergeGrpItr = fixAndGetMergeGroupItr(idItr->second, *overrideNhops);
        CHECK(
            mergedGroups_.size() == numMergeGroupsBefore ||
            mergedGroups_.size() == numMergeGroupsBefore + 1);
        newMergeGrpCreated = mergedGroups_.size() > numMergeGroupsBefore;
      }

      std::tie(grpInfo, grpInserted) = nextHopGroupIdToInfo_.refOrEmplace(
          idItr->second,
          idItr->second,
          idItr,
          newRoute->getForwardInfo().getOverrideEcmpSwitchingMode().has_value(),
          mergeGrpItr);
      CHECK(grpInserted);
      inOutState->addOrUpdateRoute(
          rid, newRoute, false /* ecmpDemandExceeded*/);
      inOutState->nonBackupEcmpGroupsCnt +=
          hasOverrides && !newMergeGrpCreated ? 0 : 1;
      XLOG(DBG2) << " Route: " << (oldRoute ? "update " : "add ")
                 << newRoute->str()
                 << " points to new group: " << grpInfo->getID()
                 << " primray ecmp group count incremented to: "
                 << inOutState->nonBackupEcmpGroupsCnt;
    }
  } else {
    // Route points to a existing group
    grpInfo = nextHopGroupIdToInfo_.ref(idItr->second);
    if (grpInfo->hasOverrides() !=
        newRoute->getForwardInfo().hasOverrideSwitchingModeOrNhops()) {
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
    if (grpInserted && !pitr->second->getMergedGroupInfoItr()) {
      /*
       * New umerged group added, compute candidate merges
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

/*
 * When restoring from switch state (e.g. warm boot). We may
 * encounter prefixes that already have override nhops. This implies
 * that they should be referencing a merged group. Incrementally
 * build this group out. E.g. if we have such prefixes show up
 *
 * P1 -> [G1, G2], orig G1
 * P2 -> [G1, G2], orig G2
 *
 * Once P1 is added we will notice that it has override nhops. We will
 * look for a matching merge nhop group but not find it. So we will
 * create one. Like so
 * [G1] -> {overrideNhops, {G1-> penalty}}
 *
 * Then when P2 arrives, we will notice that it points to the same
 * merge group. So we will do 2 things
 * 1. Update merge group like so
 * [G1, G2] -> {overrideNhops, {G1-> penalty, G2->penalty}}
 * 2. Update merge group iterator in G1 to point to new position in
 * mergeGroups_ map.
 *
 * FIXME:
 * We compare against existing merge group nhops when selecting a merge
 * group. However we don't do so when deciding to choose a new merge group.
 * So its possible, that reconstruction ends up with a more optimal (lower)
 * set of merge groups than in the forward pass. Will fix this.
 */
EcmpResourceManager::GroupIds2ConsolidationInfoItr
EcmpResourceManager::fixAndGetMergeGroupItr(
    const NextHopGroupId newMemberGroupId,
    const RouteNextHopSet& mergedNhops) {
  auto mitr = mergedGroups_.begin();
  for (; mitr != mergedGroups_.end(); ++mitr) {
    if (mitr->second.mergedNhops == mergedNhops) {
      CHECK(!mitr->first.contains(newMemberGroupId));
      break;
    }
  }
  if (mitr == mergedGroups_.end()) {
    XLOG(DBG2) << " Group ID : " << newMemberGroupId
               << " merged nhops not found, creating new merged group entry";
    ConsolidationInfo info{mergedNhops, {}};
    std::tie(mitr, std::ignore) =
        mergedGroups_.insert({{newMemberGroupId}, std::move(info)});
  } else {
    NextHopGroupIds newMergeSet = mitr->first;
    auto info = mitr->second;
    mergedGroups_.erase(mitr);
    auto [_, inserted] = newMergeSet.insert(newMemberGroupId);
    CHECK(inserted);
    std::tie(mitr, inserted) =
        mergedGroups_.insert({newMergeSet, std::move(info)});
    CHECK(inserted);
    // Fix up iterators
    fixMergeItreators(newMergeSet, mitr, {newMemberGroupId});
  }
  CHECK(!nextHopGroupIdToInfo_.ref(newMemberGroupId));
  auto [_, insertedPenalty] =
      mitr->second.groupId2Penalty.insert({newMemberGroupId, 0});
  CHECK(insertedPenalty);
  return mitr;
}

void EcmpResourceManager::fixMergeItreators(
    const NextHopGroupIds& newMergeSet,
    GroupIds2ConsolidationInfoItr mitr,
    const NextHopGroupIds& toIgnore) {
  std::for_each(
      newMergeSet.begin(),
      newMergeSet.end(),
      [this, &toIgnore, mitr](auto grpId) {
        if (!toIgnore.contains(grpId)) {
          nextHopGroupIdToInfo_.ref(grpId)->setMergedGroupInfoItr(mitr);
        }
      });
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
  const auto& oldNHops =
      oldRoute->getForwardInfo().nonOverrideNormalizedNextHops();
  const auto& newNHops =
      newRoute->getForwardInfo().nonOverrideNormalizedNextHops();
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
  if (newRoute->getForwardInfo().nonOverrideNormalizedNextHops().size() > 1) {
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
  const auto& routeNhops =
      removed->getForwardInfo().nonOverrideNormalizedNextHops();
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
    if (!removed->getForwardInfo().hasOverrideSwitchingModeOrNhops()) {
      // Last reference to this ECMP group gone, check if this group was
      // of primary ECMP group type
      --inOutState->nonBackupEcmpGroupsCnt;
      XLOG(DBG2) << "Delete route: " << removed->str()
                 << " primary ecmp group count decremented to: "
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
    CHECK(pruneFromMergedGroups({groupId}));
  }
}

bool EcmpResourceManager::pruneFromCandidateMerges(
    const NextHopGroupIds& groupIds) {
  XLOG(DBG2) << " Pruning from candidate merges: " << groupIds;
  return pruneFromMergeGroupsImpl(groupIds, candidateMergeGroups_);
}

bool EcmpResourceManager::pruneFromMergedGroups(
    const NextHopGroupIds& groupIds) {
  XLOG(DBG2) << " Pruning from  merged groups: " << groupIds;
  return pruneFromMergeGroupsImpl(groupIds, mergedGroups_);
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
                 << " merge penalty for: " << mergedGroups
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
        if (!routeFwdEqual(oldRoute, newRoute)) {
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

size_t EcmpResourceManager::getCost(NextHopGroupId nhopGrpId) const {
  auto grpInfo = nextHopGroupIdToInfo_.ref(nhopGrpId);
  if (grpInfo) {
    return grpInfo->cost();
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
  XLOG(DBG2) << " Computing consolidation penaties for: " << grpIds;
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

std::optional<EcmpResourceManager::ConsolidationInfo>
EcmpResourceManager::getMergeGroupConsolidationInfo(
    NextHopGroupId grpId) const {
  std::optional<ConsolidationInfo> info;
  auto infos = getConsolidationInfos(mergedGroups_, grpId);
  if (infos.size()) {
    CHECK_EQ(infos.size(), 1);
    info = infos.begin()->second;
  }
  return info;
}

EcmpResourceManager::GroupIds2ConsolidationInfo
EcmpResourceManager::getCandidateMergeConsolidationInfo(
    NextHopGroupId grpId) const {
  return getConsolidationInfos(candidateMergeGroups_, grpId);
}

template <std::forward_iterator ForwardIt>
void EcmpResourceManager::computeCandidateMerges(
    ForwardIt begin,
    ForwardIt end) {
  NextHopGroupIds alreadyMergedGroups;
  std::for_each(
      mergedGroups_.begin(),
      mergedGroups_.end(),
      [&alreadyMergedGroups](const auto& mergedGroupsAndPenalty) {
        alreadyMergedGroups.insert(
            mergedGroupsAndPenalty.first.begin(),
            mergedGroupsAndPenalty.first.end());
      });
  while (begin != end) {
    auto grpId = *begin++;
    CHECK(!alreadyMergedGroups.contains(grpId))
        << "Computing candidate merges for : " << grpId
        << " for already merged group";
    for (const auto& [grpToMergeWith, _] : nextHopGroupIdToInfo_) {
      if (alreadyMergedGroups.contains(grpToMergeWith) ||
          grpToMergeWith == grpId) {
        continue;
      }
      NextHopGroupIds candidateMerge{grpId, grpToMergeWith};
      auto consolidationInfo = computeConsolidationInfo(candidateMerge);
      candidateMergeGroups_.insert(
          {candidateMerge, std::move(consolidationInfo)});
      XLOG(DBG3) << " Added candidate merge group:" << candidateMerge;
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

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::Prefix& ridAndPfx) {
  const auto& [rid, pfx] = ridAndPfx;
  std::stringstream ss;
  ss << " RID: " << rid << " prefix: " << pfx.first << " / " << (int)pfx.second;
  os << ss.str();
  return os;
}
} // namespace facebook::fboss
