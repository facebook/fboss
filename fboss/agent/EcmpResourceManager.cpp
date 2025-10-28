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

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"

#include <limits>

namespace facebook::fboss {
namespace {

std::ostream& operator<<(
    std::ostream& os,
    const std::optional<cfg::SwitchingMode>& mode) {
  os << (mode.has_value() ? apache::thrift::util::enumNameSafe(*mode) : "None");
  return os;
}

template <typename AddressT>
void updateRouteOverrides(
    std::shared_ptr<Route<AddressT>>& route,
    std::optional<cfg::SwitchingMode> backupSwitchingMode,
    std::optional<EcmpResourceManager::GroupIds2ConsolidationInfoItr>
        mergeInfoItr) {
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
  XLOG(DBG2) << " Set : " << route->str()
             << " backup switching mode to : " << backupSwitchingMode
             << " override next hops to : "
             << (overrideNhops.has_value()
                     ? folly::to<std::string>(*overrideNhops)
                     : "null");
  route->setResolved(newForwardInfo);
  route->publish();
}

void updateRouteOverrides(
    const EcmpResourceManager::Prefix& ridAndPfx,
    std::shared_ptr<SwitchState>& newState,
    std::optional<cfg::SwitchingMode> backupSwitchingMode = std::nullopt,
    std::optional<EcmpResourceManager::GroupIds2ConsolidationInfoItr>
        mergeInfoItr = std::nullopt) {
  // Cannot have both backup switching mode and merge info set.
  CHECK(!(backupSwitchingMode.has_value() && mergeInfoItr.has_value()));
  auto updateFib = [backupSwitchingMode, mergeInfoItr](
                       const auto& routePfx, auto fib) {
    auto route = fib->exactMatch(routePfx)->clone();
    updateRouteOverrides(route, backupSwitchingMode, mergeInfoItr);
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

std::string toStr(const EcmpResourceManager::NextHopGroupIds& gids) {
  return "[" + folly::join(", ", gids) + "]";
}

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::NextHopGroupIds& gids) {
  os << toStr(gids);
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

EcmpResourceManager::NextHopGroupIds nhopGroupIdsDifference(
    const EcmpResourceManager::NextHopGroupIds& subtractFrom,
    const EcmpResourceManager::NextHopGroupIds& toSubtract) {
  EcmpResourceManager::NextHopGroupIds diff;
  std::set_difference(
      subtractFrom.begin(),
      subtractFrom.end(),
      toSubtract.begin(),
      toSubtract.end(),
      std::inserter(diff, diff.begin()));
  return diff;
}

RouteNextHopSet computeCommonNextHops(
    const std::vector<const RouteNextHopSet*>& in) {
  if (in.empty()) {
    return RouteNextHopSet();
  }
  if (in.size() == 1) {
    return **in.begin();
  }
  std::vector<NextHop> out;
  std::set_intersection(
      in[0]->begin(),
      in[0]->end(),
      in[1]->begin(),
      in[1]->end(),
      std::back_inserter(out));
  for (auto i = 2; i < in.size(); ++i) {
    std::vector<NextHop> tmpCommonNhops;
    std::set_intersection(
        out.begin(),
        out.end(),
        in[i]->begin(),
        in[i]->end(),
        std::back_inserter(tmpCommonNhops));
    std::swap(out, tmpCommonNhops);
  }
  return RouteNextHopSet(out.begin(), out.end());
}

int computePenalty(int numGroupNhops, int numMergedNhops, int routeRefCount) {
  DCHECK_GT(numGroupNhops, 0);
  double nhopsPctLoss{0.0};
  if (numGroupNhops) {
    CHECK_GE(numGroupNhops, numMergedNhops);
    auto nhopsLost = numGroupNhops - numMergedNhops;
    nhopsPctLoss = std::ceil((nhopsLost * 100.0) / numGroupNhops);
  }
  return routeRefCount * nhopsPctLoss;
}

bool checkDeltasPublished(const std::vector<StateDelta>& deltas) {
  bool published{true};
  for (auto i = 0; i < deltas.size() && published; ++i) {
    auto deltaPublished = deltas[i].oldState()->isPublished() &&
        deltas[i].newState()->isPublished();
    XLOG(DBG4) << " Checking delta: " << i << " published: " << deltaPublished;
  }
  return published;
}

std::optional<cfg::SwitchingMode> getBackupSwitchingMode(
    const std::shared_ptr<SwitchState>& state) {
  std::optional<cfg::SwitchingMode> backupMode;
  if (state->getFlowletSwitchingConfig()) {
    backupMode = state->getFlowletSwitchingConfig()->getBackupSwitchingMode();
  }
  return backupMode;
}

} // namespace

EcmpResourceManager::EcmpResourceManager(
    const EcmpResourceManagerConfig& config,
    const SwitchStatsGetter& statsGetter)
    : nextHopGroupIdToInfo_(
          [this](const NextHopGroupId&, const NextHopGroupInfo& grpInfo) {
            nextHopGroup2Id_.erase(grpInfo.getNhops());
          }),
      statsGetter_(statsGetter),
      config_(config) {
  if (auto switchStats = statsGetter_()) {
    switchStats->setPrimaryEcmpGroupsExhausted(false);
    switchStats->setPrimaryEcmpGroupsCount(0);
    switchStats->setBackupEcmpGroupsCount(0);
    switchStats->setMergedEcmpGroupsCount(0);
    switchStats->setMergedEcmpMemberGroupsCount(0);
  }
}

const NextHopGroupInfo* EcmpResourceManager::getGroupInfo(
    RouterID rid,
    const folly::CIDRNetwork& nw) const {
  auto pitr = prefixToGroupInfo_.find({rid, nw});
  return pitr == prefixToGroupInfo_.end() ? nullptr : pitr->second.get();
}

std::vector<StateDelta> EcmpResourceManager::modifyState(
    const std::vector<StateDelta>& deltas) {
  // TODO: Handle list of deltas instead of single delta
  CHECK_EQ(deltas.size(), 1);
  return consolidate(*deltas.begin());
}

std::pair<uint32_t, uint32_t>
EcmpResourceManager::getPrimaryEcmpAndMemberCounts() const {
  uint32_t unmergedGroups{0}, ecmpMemberCnt{0};
  std::set<const ConsolidationInfo*> mergedGroups;
  std::for_each(
      nextHopGroupIdToInfo_.cbegin(),
      nextHopGroupIdToInfo_.cend(),
      [&unmergedGroups, &mergedGroups, &ecmpMemberCnt](const auto& idAndInfo) {
        auto groupInfo = idAndInfo.second.lock();
        unmergedGroups += groupInfo->hasOverrides() ? 0 : 1;
        if (auto gitr = groupInfo->getMergedGroupInfoItr()) {
          auto [_, inserted] = mergedGroups.insert(&(*gitr)->second);
          // Merged groups nhops only count once towards ecmp members
          ecmpMemberCnt += inserted ? (*gitr)->second.mergedNhops.size() : 0;
        } else {
          ecmpMemberCnt += groupInfo->getNhops().size();
        }
      });
  uint32_t primaryEcmpGroupsCnt = unmergedGroups + mergedGroups.size();
  XLOG(DBG2) << " Got primary group count: " << primaryEcmpGroupsCnt << " with "
             << unmergedGroups << " unmerged groups and " << mergedGroups.size()
             << " merged groups. Ecmp member count is: " << ecmpMemberCnt;
  return std::make_pair(primaryEcmpGroupsCnt, ecmpMemberCnt);
}

std::vector<StateDelta> EcmpResourceManager::consolidate(
    const StateDelta& delta,
    bool rollingBack) {
  std::optional<InputOutputState> inOutState;
  StopWatch timeIt("EcmpResourceManager::consolidate", false /*json*/);
  SCOPE_EXIT {
    if (inOutState.has_value()) {
      XLOG(DBG2) << " Updated deltas: " << inOutState->updated;
    } else {
      XLOG(DBG2) << "Returning original delta";
    }
  };
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

  handleSwitchSettingsDelta(delta);
  auto switchingModeChangeResult =
      handleFlowletSwitchConfigDelta(delta, rollingBack);
  if (switchingModeChangeResult) {
    switchingModeChangeResult->publishLastDelta();
  }
  inOutState = std::move(switchingModeChangeResult);
  if (DeltaFunctions::isEmpty(delta.getFibsDelta())) {
    if (inOutState.has_value()) {
      return inOutState->moveDeltas();
    }
    return makeRet(delta);
  }

  auto [primaryEcmpGroupsCnt, ecmpMemberCnt] = getPrimaryEcmpAndMemberCounts();
  if (!inOutState.has_value()) {
    inOutState = InputOutputState(
        primaryEcmpGroupsCnt, ecmpMemberCnt, delta, rollingBack);
  } else {
    inOutState->primaryEcmpGroupsCnt = primaryEcmpGroupsCnt;
    inOutState->ecmpMemberCnt = ecmpMemberCnt;
  }
  XLOG(DBG2) << " Start delta processing, primary group count: "
             << inOutState->primaryEcmpGroupsCnt << " and "
             << " Ecmp member count is: " << inOutState->ecmpMemberCnt;
  auto deltas = consolidateImpl(delta, &(*inOutState));
  XLOG(DBG2) << " Will return : " << deltas.size() << " deltas";
  return deltas;
}

std::vector<StateDelta> EcmpResourceManager::consolidateImpl(
    const StateDelta& delta,
    InputOutputState* inOutState) {
  auto computeAndPublishLastDelta = [inOutState](const auto& computeFunc) {
    computeFunc();
    inOutState->publishLastDelta();
  };
  computeAndPublishLastDelta(
      [&delta, inOutState, this]() { processRouteUpdates(delta, inOutState); });
  computeAndPublishLastDelta(
      [inOutState, this]() { reclaimSingleMemberMergeGroups(inOutState); });
  computeAndPublishLastDelta(
      [inOutState, this]() { reclaimEcmpGroups(inOutState); });
  CHECK_NE(inOutState->numDeltas(), 0);
  if (!inOutState->updated) {
    /*
     * If inOutState was not updated, just return the original delta
     */
    std::ignore = inOutState->moveDeltas();
    inOutState->appendDelta(delta);
  }
  if (auto switchStats = statsGetter_()) {
    switchStats->setPrimaryEcmpGroupsCount(inOutState->primaryEcmpGroupsCnt);
    auto backupEcmpGroupCount = getBackupEcmpSwitchingMode().has_value()
        ? nextHopGroup2Id_.size() - inOutState->primaryEcmpGroupsCnt
        : 0;
    switchStats->setBackupEcmpGroupsCount(backupEcmpGroupCount);
    auto primaryEcmpExhuasted =
        backupEcmpGroupCount > 0 || mergedGroups_.size() > 0;
    switchStats->setPrimaryEcmpGroupsExhausted(primaryEcmpExhuasted);
    switchStats->setMergedEcmpGroupsCount(mergedGroups_.size());
    switchStats->setMergedEcmpMemberGroupsCount(getMergedGids().size());
    if (inOutState->updated && primaryEcmpExhuasted) {
      switchStats->primaryEcmpGroupsExhausted();
    }
  }
  DCHECK(checkPrimaryGroupAndMemberCounts(*inOutState));
  DCHECK(checkNoUnitializedGroups());
  return inOutState->moveDeltas();
}

bool EcmpResourceManager::checkPrimaryGroupAndMemberCounts(
    const EcmpResourceManager::InputOutputState& inOutState) const {
  auto [primaryEcmpGroups, ecmpMemberCnt] = getPrimaryEcmpAndMemberCounts();
  XLOG(DBG2) << " Primary ecmp groups, expected: " << primaryEcmpGroups
             << " computed:  " << inOutState.primaryEcmpGroupsCnt
             << " Ecmp member count, expected: " << ecmpMemberCnt
             << " computed: " << inOutState.ecmpMemberCnt;
  return primaryEcmpGroups == inOutState.primaryEcmpGroupsCnt;
}

bool EcmpResourceManager::checkNoUnitializedGroups() const {
  return std::all_of(
      nextHopGroupIdToInfo_.begin(),
      nextHopGroupIdToInfo_.end(),
      [](const auto& idAndInfo) {
        return !idAndInfo.second.lock()->isUninitialized();
      });
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
  if (getBackupEcmpSwitchingMode()) {
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
      // Merge itr must be set, since we are reclaiming from a merged group
      CHECK(mergeGrpInfoItr.has_value());
      const auto& [mergeSet, _] = *mergeGrpInfoItr.value();
      // Merged group must not be empty. Else it should have already been
      // pruned
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
  auto oldState = inOutState->getCurrentStateDelta().newState();
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
  inOutState->appendDelta(StateDelta(oldState, newState));
  // Increment primaryEcmpGroupsCnt, however no need to increment
  // ecmpMemberCnt since backup ecmp group members already counted
  // towards ecmpMemberCnt
  inOutState->primaryEcmpGroupsCnt += groupIdsToReclaim.size();
  inOutState->updated = true;
  XLOG(DBG2) << "After reclaim, primary ECMP Groups: "
             << inOutState->primaryEcmpGroupsCnt
             << " ECMP member count: " << inOutState->ecmpMemberCnt;
}

EcmpResourceManager::NextHopGroupIdToPrefixes
EcmpResourceManager::getGidToPrefixes() const {
  std::unordered_map<NextHopGroupId, std::vector<Prefix>> gid2Prefix;
  std::for_each(
      prefixToGroupInfo_.begin(),
      prefixToGroupInfo_.end(),
      [&gid2Prefix](const auto& pfxAndGroupInfo) {
        gid2Prefix[pfxAndGroupInfo.second->getID()].push_back(
            pfxAndGroupInfo.first);
      });
  return gid2Prefix;
}

void EcmpResourceManager::updateMergeInfo(
    const NextHopGroupIdToPrefixes& gid2Prefix,
    const NextHopGroupIds& groups2Update,
    std::optional<GroupIds2ConsolidationInfoItr> newMergeInfoItr,
    std::shared_ptr<SwitchState>& newState) {
  for (auto gid : groups2Update) {
    for (const auto& pfx : gid2Prefix.find(gid)->second) {
      updateRouteOverrides(pfx, newState, std::nullopt, newMergeInfoItr);
    }
    if (auto grpInfo = nextHopGroupIdToInfo_.ref(gid)) {
      grpInfo->setMergedGroupInfoItr(newMergeInfoItr);
    }
  }
}
/*
 * Reclaim sub cases
 * i. Reclaim all
 * Input - mergeSetsToUpdate : [[1, 2, 3]], groupIdsToReclaim : [1, 2, 3]
 * newMergeSet: []
 * Extra ECMP groups: 2 - add 1, 2, 3 and delete [1, 2, 3])
 * ii. Partial reclaim with creation of new merge set
 * Input - mergeSetsToUpdate : [[1, 2, 3], groupIdsToReclaim : [1]
 * newMergeSet: [2, 3]
 * Extra ECMP groups: 1 - add 1, [2, 3] and delete [1, 2, 3]
 * iii. Reclaim all but one group in a merge set
 * Input - mergeSetsToUpdate : [[1, 2, 3]], groupIdsToReclaim : [1, 2]
 * newMergeSet: [3] - or simply the unmerged group 3
 * Extra ECMP groups: 2 - add 1, 2, 3 and delete [1, 2, 3].
 *
 * Delete subcases
 * Note delete always happens one group at a time.
 * i. Group size > 2
 * Input - mergeSetsToUpdate : [[1, 2, 3]], groupIdsToReclaim : [1]
 * newMergeSet: [2, 3]
 * Extra ECMP groups: 0 - add [2, 3] added and delete [1, 2, 3]
 * i. Group size = 2
 * Input - mergeSetsToUpdate : [[1, 2]] groupIdsToReclaim : [1]
 * newMergeSet: [2]
 * Extra ECMP groups: 0 - add 2 added and [1, 2]
 */
void EcmpResourceManager::updateMergedGroups(
    const std::set<NextHopGroupIds>& mergeSetsToUpdate,
    MergeGroupUpdateOp op,
    const NextHopGroupIds& groupIdsToReclaimOrPrune,
    InputOutputState* inOutState) {
  XLOG(DBG2) << " Will reclaim the following merge groups: "
             << mergeSetsToUpdate;
  NextHopGroupIds unmergedGroups;
  if (op == MergeGroupUpdateOp::RECLAIM_GROUPS) {
    unmergedGroups = groupIdsToReclaimOrPrune;
  } else if (op == MergeGroupUpdateOp::DELETE_GROUPS) {
    // Groups are deleted one by one
    CHECK_EQ(groupIdsToReclaimOrPrune.size(), 1);
  }

  for (const auto& curMergeSet : mergeSetsToUpdate) {
    CHECK_GE(curMergeSet.size(), 1);
    XLOG(DBG2) << " Updating merged group: " << curMergeSet;
    /*
     * To allow for partial reclaims, we compute a newMergeSet.
     * newMergeSet is the set of groups in curMergeSet which
     * are not to be reclaimed or pruned. These remaining
     * groups will then form a newMergeGroup.
     */
    auto newMergeSet =
        nhopGroupIdsDifference(curMergeSet, groupIdsToReclaimOrPrune);
    auto oldState = inOutState->getCurrentStateDelta().newState();
    auto newState = oldState->clone();
    auto gid2Prefix = getGidToPrefixes();
    if (op == MergeGroupUpdateOp::RECLAIM_GROUPS) {
      /*
       * If we are reclaiming (not deleting)
       * Every thing in curMergeSet but not in newMergeSet is to be reclaimed.
       * For such groups and corresponding prefixes
       * - Clear mergeInfoItr
       * - Clear override nhops for prefixes pointing to such groups
       */
      auto reclaimedGroups = nhopGroupIdsDifference(curMergeSet, newMergeSet);
      updateMergeInfo(gid2Prefix, reclaimedGroups, std::nullopt, newState);
      // Each of these unmerged groups will create 1 primary
      // ECMP group
      inOutState->primaryEcmpGroupsCnt += reclaimedGroups.size();
      // Each of these unmerged groups will contribute ecmp nhops
      for (auto reclaimed : reclaimedGroups) {
        inOutState->ecmpMemberCnt +=
            nextHopGroupIdToInfo_.ref(reclaimed)->getNhops().size();
      }
      XLOG(DBG2) << " From: " << curMergeSet
                 << " will reclaim : " << reclaimedGroups;
    } else {
      XLOG(DBG2) << " From: " << curMergeSet << " will delete : "
                 << nhopGroupIdsDifference(curMergeSet, newMergeSet);
    }
    auto citr = mergedGroups_.find(curMergeSet);
    // curMergeSet must be in the mergedGroups_ map
    CHECK(citr != mergedGroups_.end());
    auto curMergedSetNhops = citr->second.mergedNhops.size();
    // Prune curMergeSet from mergedGroups_
    mergedGroups_.erase(curMergeSet);
    // Prune all candidate merges where we had unmerged groups
    // merged with curMergeSet
    pruneFromCandidateMerges(curMergeSet);
    /*
     * If newMergeSet.size() > 1
     * Create a new merge group and cache its iterator.
     * If newMergeSet.size() == 1
     * Reclaim the lone group remaining in merge group.
     * Update prefix nhops and group's mergeInfo iterators accordingly
     */
    std::optional<GroupIds2ConsolidationInfoItr> newMergeItr;
    if (newMergeSet.size() > 1) {
      // If newMergeSet.size() > 1 we will create one new merged
      // group and then delete the curMergeSet. Transiently we
      // will create one extra ECMP group but at the end of update
      // we will end up with exactly the same number of ECMP groups
      XLOG(DBG2) << " Replacing merge group: " << curMergeSet << std::endl
                 << " with: " << newMergeSet;
      std::tie(newMergeItr, std::ignore) = mergedGroups_.insert(
          {newMergeSet, computeConsolidationInfo(newMergeSet)});
      computeCandidateMergesForNewMergedGroup(newMergeSet);
      auto [mergeGroupInfo, _] =
          getOrCreateGroupInfo((*newMergeItr)->second.mergedNhops, *inOutState);
      mergeGroupInfo->setMergedGroupInfoItr(newMergeItr);
      (*newMergeItr)->second.mergedGroupInfo = mergeGroupInfo;
    } else if (newMergeSet.size() == 1) {
      XLOG(DBG2) << " Reclaiming merge group: " << curMergeSet
                 << " since it has only one member group remaining: "
                 << newMergeSet;
      unmergedGroups.insert(*newMergeSet.begin());
      // If newMergeSet.size() == 1 we will create one new unmerged
      // group and then delete the curMergeSet. Transiently we
      // will create one extra ECMP group but at the end of update
      // we will end up with exactly the same number of ECMP groups
    } else {
      XLOG(DBG2) << " Reclaiming merge group: " << curMergeSet;
      inOutState->primaryEcmpGroupsCnt -= 1;
      inOutState->ecmpMemberCnt -= curMergedSetNhops;
    }
    // Now update the groups and prefixes corresponding to the
    // newMergeSet
    updateMergeInfo(gid2Prefix, newMergeSet, newMergeItr, newState);

    newState->publish();
    // We put each unmerge on a new delta, to ensure that all
    // constitutent groups get unmerged and we reclaim the merged
    // group at the end of this processing.
    inOutState->appendDelta(StateDelta(oldState, newState));
    // Reclaimed curMergeSet.size() groups and deleted all references
    // to the merged group.
    inOutState->updated = true;
  }
  /*
   * Filter out pruned gids before computing candidate merges
   * for reclaimed (but not pruned gids)
   */
  computeCandidateMergesForNewUnmergedGroups(
      unmergedGroups.begin(), unmergedGroups.end());
}
/*
 * During rollback we can have single member merge groups at the end.
 * Consider the following example (P = prefix, G = ECMP group), with
 * a ECMP limit of 5
 * State 1
 * P1 -> G1, P2->G2, P3->G3, P4->G4, P5->G5
 * Rolling back to
 * State 0
 * P0 -> G0, P1, P2 -> [G1,G2], P3->G3, P4->G4, P5->G5,
 * Further consider that merged nhops of [G1, G2] are same as G2
 * NOTE: The guarantee that we provide is that rollback will provide
 * a safe sequence of deltas to not overflow the HW. Due to the sequence
 * dependent nature of merge algo, you might endup with a different
 * set of merge groups at the end.
 * Here is how things pan out for this rollback.
 * Step 0, P0->G0, triggers a ECMP merge. Say we choose merge of P1, P2 -> [G1,
 * G2] Step 1, P1 update This has 2 steps P1 delete (data structure update only
 * and P1 add). During P1 delete we unmerge G1, G2 and make P2->G2 P1 add then
 * remerges these groups to make P1, P2 -> [G1, G2]. We are at ECMP group count
 * of 5. Step 2, P2 update 2.i - P2 delete (data structure update only) This
 * unmerged [G1, G2] and makes P1->G1. 2.ii P2 add with override nhops. This
 * will cause a new groupId to be allocated say G6. This is seen as new group.
 * Since we are already at ECMP limit this triggers a merge. Say we merge [P3,
 * P4] ->[G3, G4]. Following this P2 will get added as P2->[G6]. Note this G6 is
 * considered a merged group, and P2 is considered to have override nhops. Since
 * P2 came in with override nhops, we always start with a merged group. Since we
 * do not know whether there will be subsequent groups that will come and merge
 * with this group. The final state would be P0 -> G0, P1->G1, P2 -> [G6], P3,
 * P4->[G3,G4], P5->G5 This is within ECMP group limit of 5. But has a single
 * member merged group. Reclaim it
 *
 */
void EcmpResourceManager::reclaimSingleMemberMergeGroups(
    InputOutputState* inOutState) {
  if (!getEcmpCompressionThresholdPct()) {
    return;
  }
  std::vector<std::shared_ptr<NextHopGroupInfo>> singleMemberMergedGroups;
  NextHopGroupIds singleMemberGids;
  std::for_each(
      nextHopGroupIdToInfo_.begin(),
      nextHopGroupIdToInfo_.end(),
      [&singleMemberMergedGroups, &singleMemberGids](const auto& idAndGrpRef) {
        auto groupInfo = idAndGrpRef.second.lock();
        auto gmitr = groupInfo->getMergedGroupInfoItr();
        if (gmitr && (*gmitr)->first.size() == 1) {
          singleMemberMergedGroups.push_back(groupInfo);
          singleMemberGids.insert(groupInfo->getID());
        }
      });
  reclaimMergeGroups(singleMemberMergedGroups, singleMemberGids, inOutState);
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
  updateMergedGroups(
      mergeSetsToUpdate,
      MergeGroupUpdateOp::RECLAIM_GROUPS,
      groupIdsToReclaim,
      inOutState);
}

void EcmpResourceManager::reclaimEcmpGroups(InputOutputState* inOutState) {
  DCHECK_LE(
      inOutState->primaryEcmpGroupsCnt, config_.getMaxPrimaryEcmpGroups());
  auto canReclaim =
      config_.getMaxPrimaryEcmpGroups() - inOutState->primaryEcmpGroupsCnt;
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
  if (getBackupEcmpSwitchingMode()) {
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
  if (!getEcmpCompressionThresholdPct()) {
    return {};
  }
  if (candidateMergeGroups_.empty()) {
    throw FbossError("Hit ECMP group limit, but no candidate merge found");
  }
  auto citr = std::min_element(
      candidateMergeGroups_.begin(),
      candidateMergeGroups_.end(),
      [](const auto& groupsAndInfoOne, const auto& groupsAndInfoTwo) {
        return groupsAndInfoOne.second.maxPenalty() <
            groupsAndInfoTwo.second.maxPenalty();
      });

  return citr->first;
}

EcmpResourceManager::NextHopGroupIds EcmpResourceManager::getMergedGids()
    const {
  NextHopGroupIds gids;
  std::for_each(
      mergedGroups_.begin(),
      mergedGroups_.end(),
      [&gids](const auto& groupAndInfo) {
        gids.insert(groupAndInfo.first.begin(), groupAndInfo.first.end());
      });
  return gids;
}

std::vector<EcmpResourceManager::NextHopGroupIds>
EcmpResourceManager::getMergedGroups() const {
  std::vector<NextHopGroupIds> mGroups;
  std::for_each(
      mergedGroups_.begin(),
      mergedGroups_.end(),
      [&mGroups](const auto& gidsAndInfo) {
        mGroups.push_back(gidsAndInfo.first);
      });
  return mGroups;
}

EcmpResourceManager::NextHopGroupIds EcmpResourceManager::getUnMergedGids()
    const {
  NextHopGroupIds gids;
  std::for_each(
      nextHopGroupIdToInfo_.begin(),
      nextHopGroupIdToInfo_.end(),
      [&gids](const auto& gidAndGroup) {
        auto grpInfo = gidAndGroup.second.lock();
        CHECK(grpInfo);
        if (!(grpInfo->isUninitialized() || grpInfo->getMergedGroupInfoItr())) {
          gids.insert(gidAndGroup.first);
        }
      });
  return gids;
}

EcmpResourceManager::InputOutputState::InputOutputState(
    uint32_t _primaryEcmpGroupsCnt,
    uint32_t _ecmpMemberCnt,
    const StateDelta& _in,
    bool rollingBack)
    : primaryEcmpGroupsCnt(_primaryEcmpGroupsCnt),
      ecmpMemberCnt(_ecmpMemberCnt),
      rollingBack_(rollingBack) {
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
    DCHECK(
        DeltaFunctions::isEmpty(
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
  appendDelta(StateDelta(_in.oldState(), newStateWithOldFibs));
}

void EcmpResourceManager::InputOutputState::publishLastDelta() {
  if (out_.size()) {
    DCHECK(out_.back().oldState()->isPublished());
    out_.back().newState()->publish();
  }
  DCHECK(checkDeltasPublished(out_));
}

void EcmpResourceManager::InputOutputState::appendDelta(
    const StateDelta& delta) {
  /*
   * Now that we are starting a new delta. Ensure that the
   * previous delta is published
   */
  publishLastDelta();
  out_.emplace_back(delta.oldState(), delta.newState());
}

void EcmpResourceManager::InputOutputState::replaceLastDelta(
    const StateDelta& delta) {
  CHECK(!out_.empty());
  DCHECK_EQ(out_.back().oldState(), delta.oldState());
  DCHECK(out_.back().oldState()->isPublished());
  out_.pop_back();
  out_.emplace_back(delta.oldState(), delta.newState());
}

template <typename AddrT>
void EcmpResourceManager::InputOutputState::addOrUpdateRoute(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    bool addNewDelta) {
  auto curStateDelta = getCurrentStateDelta();
  DCHECK(curStateDelta.oldState()->isPublished());
  if (addNewDelta) {
    // We are starting a new delta, publish the
    // last one.
    publishLastDelta();
  }
  auto oldState = curStateDelta.newState();
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
    // Still working on the current, replace the current delta.
    oldState = getCurrentStateDelta().oldState();
    replaceLastDelta(StateDelta(oldState, newState));
  } else {
    // add new delta
    appendDelta(StateDelta(oldState, newState));
  }
}

template <typename AddrT>
void EcmpResourceManager::InputOutputState::deleteRoute(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& delRoute) {
  auto curStateDelta = getCurrentStateDelta();
  DCHECK(curStateDelta.oldState()->isPublished());
  auto oldState = curStateDelta.newState();
  auto newState = oldState->clone();
  auto fib = newState->getFibs()->getNode(rid)->getFib<AddrT>()->modify(
      rid, &newState);
  fib->removeNode(delRoute);
  oldState = getCurrentStateDelta().oldState();
  // Still working on the current, replace the current delta.
  replaceLastDelta(StateDelta(oldState, newState));
}

StateDelta EcmpResourceManager::InputOutputState::getCurrentStateDelta() const {
  CHECK(!out_.empty());
  return StateDelta(out_.back().oldState(), out_.back().newState());
}

std::pair<std::shared_ptr<NextHopGroupInfo>, bool>
EcmpResourceManager::getOrCreateGroupInfo(
    const RouteNextHopSet& nhops,
    const InputOutputState& inOutState) {
  auto [nitr, nhopsInserted] = nextHopGroup2Id_.insert(
      {nhops, findCachedOrNewIdForNhops(nhops, inOutState)});
  if (nhopsInserted) {
    CHECK(!nextHopGroupIdToInfo_.ref(nitr->second));
    auto [grpInfo, grpInserted] =
        nextHopGroupIdToInfo_.refOrEmplace(nitr->second, nitr->second, nitr);
    CHECK(grpInserted);
    return {grpInfo, nhopsInserted};
  }
  return {nextHopGroupIdToInfo_.ref(nitr->second), nhopsInserted};
}

void EcmpResourceManager::mergeGroupAndMigratePrefixes(
    InputOutputState* inOutState) {
  auto mergeSet = getOptimalMergeGroupSet();
  mergeGroupAndMigratePrefixes(mergeSet, inOutState);
}

void EcmpResourceManager::mergeGroupAndMigratePrefixes(
    const NextHopGroupIds& mergeSetIn,
    InputOutputState* inOutState) {
  CHECK(!mergeSetIn.empty())
      << "Ecmp overflow, but no candidates available for merge";
  auto mergeSet = mergeSetIn;
  auto citr = candidateMergeGroups_.find(mergeSet);
  // mergeSet must be in candidate merged
  CHECK(citr != candidateMergeGroups_.end());
  auto [newMergeGrpInfo, mergeGrpNhopsInserted] =
      getOrCreateGroupInfo(citr->second.mergedNhops, *inOutState);

  bool mergeSetUpdated{false};
  if (mergeGrpNhopsInserted) {
    // New merge group nothing to do
    XLOG(DBG2) << " Merge set : " << mergeSet
               << " nhops, did not match any existing nhops";
  } else if (!newMergeGrpInfo->hasMergedNhops()) {
    // mergeSet nhops matches a existing unmerged group. Add that
    // to the mergeSet. But check for case where merge set already
    // contained the existing group ID. For e.g. a new merge set
    // [1, 2]'s merged nhops may match group 2's nhops. In which
    // case mergeSet already has the requisite GID.
    XLOG(DBG2) << " Merge set : " << mergeSet
               << " nhops matched existing group : " << newMergeGrpInfo->getID()
               << " next hops";
    std::tie(std::ignore, mergeSetUpdated) =
        mergeSet.insert(newMergeGrpInfo->getID());
    if (mergeSetUpdated) {
      XLOG(DBG2) << " Merge set updated to : " << mergeSet;
    }
    if (newMergeGrpInfo->getMergedGroupInfoItr()) {
      DCHECK_NE(
          (*newMergeGrpInfo->getMergedGroupInfoItr())->second.mergedNhops,
          citr->second.mergedNhops);
    }
  } else {
    // We matched a existing merged group. Merge the 2 merged groups
    // to make a larger group.
    CHECK(newMergeGrpInfo->getMergedGroupInfoItr());
    auto existingMergeGrpItr = *newMergeGrpInfo->getMergedGroupInfoItr();
    XLOG(DBG2) << " Merge set : " << mergeSet
               << " nhops matched existing merged group : "
               << existingMergeGrpItr->first << " next hops";
    mergeSet.insert(
        existingMergeGrpItr->first.begin(), existingMergeGrpItr->first.end());
    mergeSetUpdated = true;
  }
  auto [mitr, mergedGroupsInserted] = mergedGroups_.insert(
      {mergeSet,
       mergeSetUpdated ? computeConsolidationInfo(mergeSet) : citr->second

      });
  CHECK(mergedGroupsInserted);

  // Added to merged groups, no longer a candidate merge.
  pruneFromCandidateMerges(mergeSet);
  // Find out if the new merge set has existing merge
  // sets contained within it.
  std::set<NextHopGroupIds> preExistingMemberMergeSets;
  auto preExistingMergeGrpNhops{0};
  std::for_each(
      mergeSet.begin(),
      mergeSet.end(),
      [this, &preExistingMemberMergeSets, &preExistingMergeGrpNhops](
          auto mgid) {
        auto grpInfo = nextHopGroupIdToInfo_.ref(mgid);
        CHECK(grpInfo);
        if (auto pmitr = grpInfo->getMergedGroupInfoItr()) {
          auto [_, inserted] =
              preExistingMemberMergeSets.insert((*pmitr)->first);
          preExistingMergeGrpNhops +=
              inserted ? (*pmitr)->second.mergedNhops.size() : 0;
        }
      });
  // Prune preExistingMemberMergeSets since we are going to
  // make these part of a larger merge set now
  std::for_each(
      preExistingMemberMergeSets.begin(),
      preExistingMemberMergeSets.end(),
      [this](const auto& mergeSetToPrune) {
        auto removed = mergedGroups_.erase(mergeSetToPrune);
        CHECK(removed);
      });
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
      // decrement primary group count. Note each unmerged
      // group decrements primaryEcmpGroupsCnt only once
      // Since once we encounter this group, we will next
      // update its merge info itr. So for the next pfx
      // pointing to this group, we won't decrement the count
      --inOutState->primaryEcmpGroupsCnt;
      inOutState->ecmpMemberCnt -= pfxGrpInfo->numNhops();
      XLOG(DBG2) << "Primary ecmp group decremented to: "
                 << inOutState->primaryEcmpGroupsCnt;
    }
    pfxGrpInfo->setMergedGroupInfoItr(mitr);
    const auto& [rid, pfx] = ridAndPfx;
    if (pfx.first.isV6()) {
      updateForwardingInfoAndInsertDelta<folly::IPAddressV6>(
          rid, pfx, pfxGrpInfo, inOutState, addNewDelta);

    } else {
      updateForwardingInfoAndInsertDelta<folly::IPAddressV4>(
          rid, pfx, pfxGrpInfo, inOutState, addNewDelta);
    }
    // Only the first prefix update needs to start a new delta.
    // Rest will just queue updates on that same delta
    addNewDelta &= false;
  }
  // Set merge info itr for new merged group
  newMergeGrpInfo->setMergedGroupInfoItr(mitr);
  // Cache the NextHopGroupInfo shared pointer in merged
  // group's consolidation info
  mitr->second.mergedGroupInfo = std::move(newMergeGrpInfo);
  // We added one merged group, so increment primaryEcmpGroupsCnt and
  // ecmpMemberCnt
  ++inOutState->primaryEcmpGroupsCnt;
  inOutState->ecmpMemberCnt += mitr->second.mergedNhops.size();
  // We got rid of any existing merge sets that were part of this larger
  // merge set, discount them from ecmp group and member count
  inOutState->primaryEcmpGroupsCnt -= preExistingMemberMergeSets.size();
  inOutState->ecmpMemberCnt -= preExistingMergeGrpNhops;
  // Compute new candidate merges of merged group + each unmerged group
  computeCandidateMergesForNewMergedGroup(mergeSet);
  XLOG(DBG2) << "Done migrating prefixes to merged group: " << mergeSet
             << ". Incremented primary ecmp group count to : "
             << inOutState->primaryEcmpGroupsCnt
             << " And ecmp member count to: " << inOutState->ecmpMemberCnt;
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
  if (ecmpDemandExceeded) {
    if (getBackupEcmpSwitchingMode().has_value()) {
      grpInfo->setIsBackupEcmpGroupType(true);
    } else {
      CHECK(getEcmpCompressionThresholdPct());
      mergeGroupAndMigratePrefixes(inOutState);
    }
  }
  const auto& curForwardInfo = route->getForwardInfo();
  XLOG(DBG2) << " Will set backup switching mode for : " << route->str()
             << " to: " << getBackupEcmpSwitchingMode();
  auto newForwardInfo = RouteNextHopEntry(
      curForwardInfo.getNextHopSet(),
      curForwardInfo.getAdminDistance(),
      curForwardInfo.getCounterID(),
      curForwardInfo.getClassID(),
      grpInfo->isBackupEcmpGroupType() ? getBackupEcmpSwitchingMode()
                                       : std::optional<cfg::SwitchingMode>(),
      std::optional<RouteNextHopSet>(grpInfo->getOverrideNextHops()));
  auto newRoute = route->clone();
  newRoute->setResolved(std::move(newForwardInfo));
  newRoute->publish();
  inOutState->addOrUpdateRoute(rid, newRoute, addNewDelta);
  inOutState->updated = true;
  return grpInfo;
}

template <typename AddrT>
std::shared_ptr<NextHopGroupInfo>
EcmpResourceManager::updateForwardingInfoAndInsertDelta(
    RouterID rid,
    const folly::CIDRNetwork& pfx,
    std::shared_ptr<NextHopGroupInfo>& pfxGrpInfo,
    InputOutputState* inOutState,
    bool addNewDelta) {
  auto newState = inOutState->getCurrentStateDelta().newState();
  auto fib = newState->getFibs()->getNode(rid)->getFib<AddrT>();
  std::shared_ptr<Route<AddrT>> existingRoute;
  if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
    CHECK(pfx.first.isV6());
    existingRoute = fib->getRouteIf(
        RoutePrefix<folly::IPAddressV6>(pfx.first.asV6(), pfx.second));
  } else {
    CHECK(pfx.first.isV4());
    existingRoute =
        fib->getRouteIf(RoutePrefix<AddrT>(pfx.first.asV4(), pfx.second));
  }
  CHECK(existingRoute);
  return updateForwardingInfoAndInsertDelta(
      rid,
      existingRoute,
      pfxGrpInfo,
      false /*ecmpDemandExceeded*/,
      inOutState,
      addNewDelta);
}

std::vector<StateDelta> EcmpResourceManager::reconstructFromSwitchState(
    const std::shared_ptr<SwitchState>& curState) {
  StopWatch timeIt(
      "EcmpResourceManager::reconstructFromSwitchState", false /*json*/);
  // Clear state which needs to be restored from given state
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
   * ECMP groups == config_.getMaxPrimaryEcmpGroups(). As we run through the
   * routes, we will see a mix of nhop groups, some that are of type primary and
   * others that are of type backupEcmpGroupType. We record them as such in our
   * data structures.  Note that in this replay routes may come in a
   * different order than when we originally got them. However since
   * we are not creating any new backup nhop groups (since
   * num nhop groups of primary type <= maxEcmpGroups) our data structures
   * should simply reflect the state in input state.
   * iii. Have both primary and backup ECMP route nhop groups. And primary
   * ECMP groups < config_.getMaxPrimaryEcmpGroups(). This is similar to ii.
   * except that we will now be able to reclaim some of the backup nhop groups.
   * */
  StateDelta delta(std::make_shared<SwitchState>(), curState);
  InputOutputState inOutState(0, 0, delta);
  auto deltas = consolidateImpl(delta, &inOutState);
  if (!getEcmpCompressionThresholdPct()) {
    // For getBackupEcmpSwitchingMode() reclaim is completed on
    // one delta. So LE 2, since reclaim always starts
    // with a new delta. So orig switch state delta +
    // reclaim.
    // For merge groups - reclaim creates one delta for
    // each merge set being reclaimed. So we can't assert
    // for a precise number.
    DCHECK_LE(deltas.size(), 2);
  }
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
std::pair<std::shared_ptr<NextHopGroupInfo>, bool>
EcmpResourceManager::routeAddedNoCompressionThreshold(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    bool ecmpLimitReached,
    InputOutputState* inOutState) {
  CHECK(!getEcmpCompressionThresholdPct());
  auto nhopSet = newRoute->getForwardInfo().normalizedNextHops();
  auto [grpInfo, grpInserted] = getOrCreateGroupInfo(nhopSet, *inOutState);
  if (grpInserted) {
    DCHECK(grpInfo->isUninitialized());
    XLOG(DBG2) << " Route: " << newRoute->str()
               << " points to new group: " << *grpInfo;
    // Ecmp limit reached and we did not find a existing group,
    // nor did this group group have switching mode set to backup ECMP.
    if (ecmpLimitReached &&
        !newRoute->getForwardInfo().hasOverrideSwitchingMode()) {
      XLOG(DBG2) << " Exceeded ECMP limit for route: " << newRoute->str();
      grpInfo = updateForwardingInfoAndInsertDelta(
          rid, newRoute, grpInfo, ecmpLimitReached, inOutState);
    } else {
      XLOG(DBG2) << "Did not exceed EMCP limit for route: " << newRoute->str();
      grpInfo->setIsBackupEcmpGroupType(newRoute->getForwardInfo()
                                            .getOverrideEcmpSwitchingMode()
                                            .has_value());
      inOutState->addOrUpdateRoute(rid, newRoute);
    }
    inOutState->primaryEcmpGroupsCnt += grpInfo->hasOverrides() ? 0 : 1;
    inOutState->ecmpMemberCnt += grpInfo->numNhops();
  } else {
    XLOG(DBG2) << " Route: " << newRoute->str()
               << " points to existing group: " << *grpInfo;
    if (grpInfo->hasOverrides() !=
        newRoute->getForwardInfo().hasOverrideSwitchingModeOrNhops()) {
      auto existingGrpInfo = updateForwardingInfoAndInsertDelta(
          rid, newRoute, grpInfo, false /*ecmpLimitReached*/, inOutState);
      CHECK_EQ(existingGrpInfo, grpInfo);
    } else {
      // Everything matches just add the route to current delta
      inOutState->addOrUpdateRoute(rid, newRoute);
    }
  }
  return {grpInfo, grpInserted};
}

/*
 * routeAddedNoOverrideNhops algo is as follows
 * Algo here is
 * - Lookup the group for route's normalized nhops
 *    - If this is a new group, we have 3 subcases
 *      i) We are ECMP limit,
 *      - Trigger a merge of some existing groups to make space.
 *      - Add the route with this group's nhops
 *     ii) We are not at ECMP limit
 *      - Add the route with it current nhops.
 *   - If we match a existing group. There are 2 subcases
 *    i) Group has override nhops
 *     We must prefer the group's override
 *     settings. We look at this group's merge itr, for the merge set
 *     membership. If this group ID is not part of that merge
 *     group, add it. Note this is a data structure update only.
 *     We don't need to update any of the other prefixes in
 *     switch state, since none of their nhops were affected.
 *     Now add the route into current delta with group's nhops.
 *   ii) Group does not have override nhops and was already present.
 *   Just update the delta with the current route.
 */
template <typename AddrT>
std::pair<std::shared_ptr<NextHopGroupInfo>, bool>
EcmpResourceManager::routeAddedNoOverrideNhops(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    bool ecmpLimitReached,
    InputOutputState* inOutState) {
  // Should only be called when using EcmpResourceManager for
  // ECMP compression
  CHECK(getEcmpCompressionThresholdPct());
  DCHECK(!newRoute->getForwardInfo().getOverrideNextHops().has_value());
  auto nhopSet = newRoute->getForwardInfo().normalizedNextHops();
  auto [grpInfo, grpInserted] = getOrCreateGroupInfo(nhopSet, *inOutState);
  if (grpInserted) {
    DCHECK(grpInfo->isUninitialized());
    XLOG(DBG2) << " Route: " << newRoute->str()
               << " points to new group: " << *grpInfo;
    // Ecmp limit reached and we did not find a existing group,
    if (ecmpLimitReached) {
      XLOG(DBG2) << " Exceeded ECMP limit for route: " << newRoute->str();
      // This will trigger a merge of 2 or more groups, followed by
      // updating adding the new route to set of deltas in inOutState.
      grpInfo = updateForwardingInfoAndInsertDelta(
          rid, newRoute, grpInfo, ecmpLimitReached, inOutState);
    } else {
      XLOG(DBG4) << "Did not exceed EMCP limit for route: " << newRoute->str();
      // New group did not exceed limits. Just add the route as is. Note
      // that since this is a new group, we don't have to worry about
      // any override nhops being set (for that group must have existed
      // in our data structures before, and would have had to have its
      // override nhops set).
      inOutState->addOrUpdateRoute(rid, newRoute);
    }
    // If grp was made part of a merge group, we would have accounted
    // for it there.
    inOutState->primaryEcmpGroupsCnt += grpInfo->hasOverrides() ? 0 : 1;
    inOutState->ecmpMemberCnt += grpInfo->numNhops();
  } else {
    XLOG(DBG2) << " Route: " << newRoute->str()
               << " points to existing group: " << *grpInfo;
    if (grpInfo->hasOverrideNextHops()) {
      // We know that route did not have override nhops. If group has
      // override next hops we need to reconcile these.
      auto mitr = grpInfo->getMergedGroupInfoItr();
      CHECK(mitr);
      // Not rolling back. Prefer group's override info
      if (!(*mitr)->first.contains(grpInfo->getID())) {
        // This group's nhops point to a merged group, but
        // the group itself is not part of the merged group,
        // then state could only be MERGED_NHOPS_ONLY
        DCHECK(grpInfo->hasMergedNhopsOnly());
        // If group is not part of the merge set pointed to by
        // the merge iterator, make it a part of it and update
        // prefixes accordingly.
        auto newMergeSet = (*mitr)->first;
        newMergeSet.insert(grpInfo->getID());
        mitr = appendToOrCreateMergeGroup(
            newMergeSet, (*mitr)->second.mergedNhops, mitr, *inOutState);
        grpInfo->setMergedGroupInfoItr(mitr);
      }
      auto existingGrpInfo = updateForwardingInfoAndInsertDelta(
          rid, newRoute, grpInfo, false /*ecmpLimitReached*/, inOutState);
      CHECK_EQ(existingGrpInfo, grpInfo);
    } else {
      // No override nhops in group, and group already existed. Just add the
      // route.
      inOutState->addOrUpdateRoute(rid, newRoute);
    }
  }
  return {grpInfo, grpInserted};
}
/*
 * routeAddedWithOverrideNhops can happen in 2 cases
 * i.  Reconstructing the switch state.
 * ii. Rollback
 * Algo here is
 * - Lookup the group for override (normalized nhops).
 *    - If this is a new group and we are ECMP limit, trigger a merge
 *    of some existing groups to make space.
 *  Next we need to create or update a merge group containing the
 *  GID for non-override nhops
 * - Create a merge set consisting of gid for non override nhops for
 *   this prefix. If the override group also points to a non override
 *   group (i.e. matches nhops of a non override group, e.g. consider
 *   P1->G1, P2->G2, and a merge group of [G1, G2] which matches nhops
 *   of G2), include the override group's id in new merge set as well.
 * - Get the override group's merge Itr.
 *    - If this itr matches newMergeSet. Nothing to do
 *    - Else if the newMergeSet.size() > 1
 *    Call mergeGroupAndMigratePrefixes, this creates a new merge groups
 *    updates both internal data structures and the prefix override nhops.
 *    - Else, we just have a single member merge group, corresponding to
 *    the current prefix. Create it, later update to current route will
 *    set the correct override nhops.
 */
template <typename AddrT>
std::pair<std::shared_ptr<NextHopGroupInfo>, bool>
EcmpResourceManager::routeAddedWithOverrideNhops(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    bool ecmpLimitReached,
    InputOutputState* inOutState) {
  CHECK(getEcmpCompressionThresholdPct());
  XLOG(DBG2) << " Processing route with override nhops: " << newRoute->str();
  auto nonOverrideNhops =
      newRoute->getForwardInfo().nonOverrideNormalizedNextHops();
  auto overrideNhops = newRoute->getForwardInfo().normalizedNextHops();
  auto [overrideGrpInfo, overrideGrpInserted] =
      getOrCreateGroupInfo(overrideNhops, *inOutState);
  auto [grpInfo, grpInserted] =
      getOrCreateGroupInfo(nonOverrideNhops, *inOutState);
  grpInserted =
      grpInserted || (grpInfo == overrideGrpInfo && overrideGrpInserted);
  XLOG(DBG2) << " Override group : " << overrideGrpInfo->getID()
             << " created : " << overrideGrpInserted
             << ". Non override grp: " << grpInfo->getID()
             << " created: " << grpInserted;
  if (overrideGrpInserted) {
    if (ecmpLimitReached) {
      DCHECK(overrideGrpInfo->isUninitialized());
      XLOG(DBG2) << " Exceeded ECMP limit for route: " << newRoute->str();
      mergeGroupAndMigratePrefixes(inOutState);
    } else {
      XLOG(DBG2) << "Did not exceed EMCP limit for route: " << newRoute->str();
    }
  }
  NextHopGroupIds newMergeGroupIds{grpInfo->getID()};
  if (overrideGrpInfo->hasUnmergedNhops() || overrideGrpInfo == grpInfo) {
    newMergeGroupIds.insert(overrideGrpInfo->getID());
  }
  auto overrideMitr = overrideGrpInfo->getMergedGroupInfoItr();
  if (overrideMitr && (*overrideMitr)->first == newMergeGroupIds) {
    XLOG(DBG2) << " Existing override group matches: : " << newMergeGroupIds;
  } else if (newMergeGroupIds.size() > 1) {
    addCandidateMerge(newMergeGroupIds);
    mergeGroupAndMigratePrefixes(newMergeGroupIds, inOutState);
  } else {
    auto mergeGrpItr = appendToOrCreateMergeGroup(
        newMergeGroupIds,
        overrideNhops,
        overrideGrpInfo->getMergedGroupInfoItr(),
        *inOutState);
    CHECK(overrideGrpInfo->getMergedGroupInfoItr());
    CHECK(*overrideGrpInfo->getMergedGroupInfoItr() == mergeGrpItr);
  }
  grpInfo->setMergedGroupInfoItr(overrideGrpInfo->getMergedGroupInfoItr());
  grpInfo = updateForwardingInfoAndInsertDelta(
      rid, newRoute, grpInfo, false, inOutState);
  inOutState->primaryEcmpGroupsCnt += overrideGrpInserted ? 1 : 0;
  inOutState->ecmpMemberCnt += overrideGrpInserted
      ? (*overrideGrpInfo->getMergedGroupInfoItr())->second.mergedNhops.size()
      : 0;
  CHECK(overrideGrpInfo->getMergedGroupInfoItr());
  return {grpInfo, grpInserted};
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
  DCHECK_LE(
      inOutState->primaryEcmpGroupsCnt, config_.getMaxPrimaryEcmpGroups());
  bool ecmpLimitReached = config_.ecmpLimitReached(
      inOutState->primaryEcmpGroupsCnt, inOutState->ecmpMemberCnt);
  if (oldRoute) {
    DCHECK(!routeFwdEqual(oldRoute, newRoute));
    /*
     * We compare the non override normalized next hops. Since
     * those represent the original nhop group demand. Further
     * since the new route came in via state update, there are
     * going to be no override (merged) nhops for it
     */
    /*
     * Update internal data structures only if nhops changes.
     * There are other route changes (e.g. classID, counterID)
     * which are no-op to us. If we delete the route here
     * we may endup deleting and recreating nhop group, which
     * is unnecessary.
     */
    routeDeleted(rid, oldRoute, true /*isUpdate*/, inOutState);
  }
  std::shared_ptr<NextHopGroupInfo> grpInfo;
  bool grpInserted{false};
  if (!getEcmpCompressionThresholdPct()) {
    std::tie(grpInfo, grpInserted) = routeAddedNoCompressionThreshold(
        rid, newRoute, ecmpLimitReached, inOutState);
  } else if (!newRoute->getForwardInfo().hasOverrideNextHops()) {
    std::tie(grpInfo, grpInserted) =
        routeAddedNoOverrideNhops(rid, newRoute, ecmpLimitReached, inOutState);
  } else {
    DCHECK(!oldRoute || inOutState->rollingBack())
        << " Routes with override nhops "
           "should only be seen during reconstructFromSwitchState (addRoute) or rollback";
    if (!oldRoute || inOutState->rollingBack()) {
      std::tie(grpInfo, grpInserted) = routeAddedWithOverrideNhops(
          rid, newRoute, ecmpLimitReached, inOutState);
    } else {
      XLOG(ERR)
          << " Ingoring override nhop !!!. Routes with override nhops "
             "should only be seen during reconstructFromSwitchState (addRoute) or rollback";
      auto newerRoute = newRoute->clone();
      updateRouteOverrides(newerRoute, std::nullopt, std::nullopt);
      std::tie(grpInfo, grpInserted) = routeAddedNoOverrideNhops(
          rid, newerRoute, ecmpLimitReached, inOutState);
    }
  }
  CHECK(grpInfo);
  auto [pitr, pfxInserted] = prefixToGroupInfo_.insert(
      {{rid, newRoute->prefix().toCidrNetwork()}, grpInfo});
  if (pfxInserted) {
    /*
     * If a new prefix points to this group increment ref count
     */
    pitr->second->incRouteUsageCount();
  }
  CHECK_GT(pitr->second->getRouteUsageCount(), 0);
  DCHECK_LE(
      inOutState->primaryEcmpGroupsCnt, config_.getMaxPrimaryEcmpGroups());
  if (getEcmpCompressionThresholdPct()) {
    if (pfxInserted) {
      /*
       * New prefix points to existing group
       * update consolidation penalties
       */
      updateConsolidationPenalty(*pitr->second);
    }
    if (grpInserted && !pitr->second->getMergedGroupInfoItr()) {
      /*
       * New unmerged group added, compute candidate merges
       * for it
       */
      computeCandidateMergesForNewUnmergedGroups({grpInfo->getID()});
    }
  }
}

std::optional<EcmpResourceManager::GroupIds2ConsolidationInfoItr>
EcmpResourceManager::getMergeGroupItr(const RouteNextHopSet& mergedNhops) {
  auto nitr = nextHopGroup2Id_.find(mergedNhops);
  if (nitr == nextHopGroup2Id_.end()) {
    return std::nullopt;
  }
  auto grpInfo = nextHopGroupIdToInfo_.ref(nitr->second);
  auto grpState = grpInfo->getState();
  if (grpState == NextHopGroupInfo::NextHopGroupState::MERGED_NHOPS_ONLY ||
      grpState ==
          NextHopGroupInfo::NextHopGroupState::UNMERGED_AND_MERGED_NHOPS) {
    // The above 2 states represent the case where nhops pointed to by
    // group's merge info itr match the indexed nhops (in nextHopGroup2Id_).
    // If so we can return the mergeInfo itr.
    // Its not sufficient to blindly return the mergeGroupInfoItr for group
    // corresponding to these nhops. Since the merge iterator maybe pointing
    // to a different set of merged nhops
    DCHECK(grpInfo->mergedAndUnmergedNhopsMatch());
    return grpInfo->getMergedGroupInfoItr();
  }
  return std::nullopt;
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
 */
EcmpResourceManager::GroupIds2ConsolidationInfoItr
EcmpResourceManager::appendToOrCreateMergeGroup(
    NextHopGroupIds newMemberGroupIds,
    const RouteNextHopSet& mergedNhops,
    std::optional<GroupIds2ConsolidationInfoItr> existingMitr,
    const InputOutputState& inOutState) {
  GroupIds2ConsolidationInfoItr mitr;
  if (!existingMitr) {
    XLOG(DBG2) << " Group ID : " << newMemberGroupIds
               << " merged nhops not found, creating new merged group entry : "
               << newMemberGroupIds;
    DCHECK(!mergedGroups_.contains(newMemberGroupIds));
    pruneFromCandidateMerges(newMemberGroupIds);
    ConsolidationInfo info{mergedNhops, {}};
    std::tie(mitr, std::ignore) =
        mergedGroups_.insert({newMemberGroupIds, std::move(info)});
  } else {
    mitr = *existingMitr;
    std::for_each(
        mitr->first.begin(),
        mitr->first.end(),
        [&newMemberGroupIds](auto existingGid) {
          newMemberGroupIds.erase(existingGid);
        });
    if (newMemberGroupIds.empty()) {
      return mitr;
    }
    XLOG(DBG2) << " Group ID : " << newMemberGroupIds
               << " found existing merged nhops, merging with: " << mitr->first;
    auto info = std::move(mitr->second);
    pruneFromCandidateMerges(mitr->first);
    NextHopGroupIds newMergeSet = mitr->first;
    mergedGroups_.erase(mitr);
    newMergeSet.insert(newMemberGroupIds.begin(), newMemberGroupIds.end());
    bool inserted{false};
    std::tie(mitr, inserted) =
        mergedGroups_.insert({newMergeSet, std::move(info)});
    CHECK(inserted);
    // Fix up iterators
    fixMergeItreators(newMergeSet, mitr, newMemberGroupIds);
  }
  auto [mergeGroupInfo, _] =
      getOrCreateGroupInfo(mitr->second.mergedNhops, inOutState);
  mergeGroupInfo->setMergedGroupInfoItr(mitr);
  mitr->second.mergedGroupInfo = mergeGroupInfo;

  std::for_each(
      newMemberGroupIds.begin(),
      newMemberGroupIds.end(),
      [this, &mitr](auto newMemberGroupId) {
        auto grpInfo = nextHopGroupIdToInfo_.ref(newMemberGroupId);
        CHECK(grpInfo);
        auto penalty = computePenalty(
            grpInfo->getNhops().size(),
            mitr->second.mergedNhops.size(),
            grpInfo->getRouteUsageCount());
        auto [_, insertedPenalty] =
            mitr->second.groupId2Penalty.insert({newMemberGroupId, penalty});
        DCHECK(insertedPenalty)
            << " Group ID: " << newMemberGroupId
            << " already has a computed penalty in: " << mitr->first;
      });
  computeCandidateMergesForNewMergedGroup(mitr->first);
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
    inOutState->addOrUpdateRoute(rid, newRoute);
  } else {
    // Neither of the routes point to > 1 nhops. Nothing to do
    CHECK_LE(oldNHops.size(), 1);
    CHECK_LE(newNHops.size(), 1);
    XLOG(DBG2) << " Route:" << newRoute->str()
               << " transitioned from single nhop to a different single nhop";
    // Just update deltas, no need to account for this as a ECMP group
    inOutState->addOrUpdateRoute(rid, newRoute);
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
    inOutState->addOrUpdateRoute(rid, newRoute);
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
  std::optional<GroupIds2ConsolidationInfoItr> mergeInfoItr;
  uint32_t numGroupNhops{0};
  bool routeHasOverrides;
  {
    auto pitr =
        prefixToGroupInfo_.find({rid, removed->prefix().toCidrNetwork()});
    CHECK(pitr != prefixToGroupInfo_.end());
    groupId = pitr->second->getID();
    // Cache group override info before we remove reference to
    // this group. If this is the last reference and removing
    // it deletes the group, then
    // - For a group w/o overrides, we will need to decrement primary ECMP
    // count. Update candidate merges
    // - If this group was part of a merged set, we will need to prune it
    // from the merged set.
    mergeInfoItr = pitr->second->getMergedGroupInfoItr();
    routeHasOverrides = pitr->second->hasOverrides();
    numGroupNhops = pitr->second->numNhops();
    prefixToGroupInfo_.erase(pitr);
  }
  auto groupInfo = nextHopGroupIdToInfo_.ref(groupId);
  if (!groupInfo) {
    XLOG(DBG2) << "Delete route: " << removed->str() << " all references to "
               << groupId << " are now gone";
    if (!routeHasOverrides) {
      // Last reference to this ECMP group gone, check if this group was
      // of primary ECMP group type
      --inOutState->primaryEcmpGroupsCnt;
      inOutState->ecmpMemberCnt -= numGroupNhops;
      XLOG(DBG2) << "Delete route: " << removed->str()
                 << " primary ecmp group count decremented to: "
                 << inOutState->primaryEcmpGroupsCnt << " Group ID: " << groupId
                 << " removed";
    }
  } else {
    decRouteUsageCount(*groupInfo);
    XLOG(DBG2) << "Delete route: " << removed->str()
               << " primray ecmp group count unchanged: "
               << inOutState->primaryEcmpGroupsCnt << " Group ID: " << groupId
               << " route usage count decremented to: "
               << groupInfo->getRouteUsageCount();
  }
  bool lastRouteRefToGroupRemoved = !groupInfo ||
      groupInfo->getState() ==
          NextHopGroupInfo::NextHopGroupState::MERGED_NHOPS_ONLY;
  if (lastRouteRefToGroupRemoved) {
    if (mergeInfoItr) {
      // If this was part of a merged group, remove it from the
      // merged group
      updateMergedGroups(
          {(*mergeInfoItr)->first},
          MergeGroupUpdateOp::DELETE_GROUPS,
          {groupId},
          inOutState);
    } else {
      // Not part of a merged group and last reference gone
      // remove from candidate merges
      pruneFromCandidateMerges({groupId});
    }
  }
}

bool EcmpResourceManager::pruneFromCandidateMerges(
    const NextHopGroupIds& groupIds) {
  XLOG(DBG2) << " Pruning from candidate merges: " << groupIds;
  return pruneFromMergeGroupsImpl(groupIds, candidateMergeGroups_);
}

void EcmpResourceManager::decRouteUsageCount(NextHopGroupInfo& groupInfo) {
  groupInfo.decRouteUsageCount();
  XLOG(DBG2) << " GID: " << groupInfo.getID()
             << " route ref count decremented to : "
             << groupInfo.getRouteUsageCount();
  if (groupInfo.getState() !=
      NextHopGroupInfo::NextHopGroupState::MERGED_NHOPS_ONLY) {
    updateConsolidationPenalty(groupInfo);
  }
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
      XLOG(DBG4) << " Computing penalty with: " << mergedGroups
                 << " Nhops lost : " << nhopsLost
                 << " Group nhops : " << grpNhopsSize
                 << " Merged nhops: " << info.mergedNhops.size();
      auto newPenalty = std::ceil((nhopsLost * 100.0) / grpNhopsSize) *
          groupInfo.getRouteUsageCount();
      XLOG(DBG4) << " GID: " << groupInfo.getID()
                 << " merge penalty for: " << mergedGroups
                 << " prev penalty: " << citr->second
                 << " new penalty: " << newPenalty;
      citr->second = newPenalty;
    }
    return updated;
  };
  XLOG(DBG2) << " Updating consolidation penalty for : " << groupInfo.getID()
             << " in candidate merges";
  updatePenalty(candidateMergeGroups_);
  XLOG(DBG2) << " Updating consolidation penalty for : " << groupInfo.getID()
             << " in merged groups";
  updatePenalty(mergedGroups_);
}

void EcmpResourceManager::processRouteUpdates(
    const StateDelta& delta,
    InputOutputState* inOutState) {
  processFibsDeltaInHwSwitchOrder(
      delta,
      [this, inOutState](
          RouterID rid, const auto& oldRoute, const auto& newRoute) {
        SCOPE_EXIT {
          DCHECK(checkPrimaryGroupAndMemberCounts(*inOutState));
          DCHECK(checkNoUnitializedGroups());
        };
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
          inOutState->addOrUpdateRoute(rid, newRoute);
        }
      },
      [this, inOutState](RouterID rid, const auto& newRoute) {
        SCOPE_EXIT {
          DCHECK(checkPrimaryGroupAndMemberCounts(*inOutState));
          DCHECK(checkNoUnitializedGroups());
        };
        if (newRoute->isResolved()) {
          routeAdded(rid, newRoute, inOutState);
        }
      },
      [this, inOutState](RouterID rid, const auto& oldRoute) {
        SCOPE_EXIT {
          DCHECK(checkPrimaryGroupAndMemberCounts(*inOutState));
          DCHECK(checkNoUnitializedGroups());
        };
        if (oldRoute->isResolved()) {
          routeDeleted(rid, oldRoute, false /*isUpdate*/, inOutState);
        }
      });
}

EcmpResourceManager::NextHopGroupId
EcmpResourceManager::findCachedOrNewIdForNhops(
    const RouteNextHopSet& nhops,
    const InputOutputState& inOutState) const {
  auto findNextAvailableId = [this]() {
    std::unordered_set<NextHopGroupId> allocatedIds;
    auto fillAllocatedIds = [&allocatedIds](const auto& nhopGroup2Id) {
      for (const auto& [_, id] : nhopGroup2Id) {
        allocatedIds.insert(id);
      }
    };
    fillAllocatedIds(nextHopGroup2Id_);
    for (auto start = kMinNextHopGroupId;
         start < std::numeric_limits<NextHopGroupId>::max();
         ++start) {
      if (allocatedIds.find(start) == allocatedIds.end()) {
        return start;
      }
    }
    throw FbossError("Unable to find id to allocate for new next hop group");
  };
  return findNextAvailableId();
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
}

void EcmpResourceManager::updateFailed(
    const std::shared_ptr<SwitchState>& knownGoodState) {
  XLOG(DBG2) << " Update failed";
  if (getBackupEcmpSwitchingMode() != getBackupSwitchingMode(knownGoodState)) {
    // Throw if we get a failed update involving backup switching mode
    // change. We can make this smarter by
    // - Reverting getBackupEcmpSwitchingMode() setting
    // - Asserting that all prefixes in knownGoodState with overrideEcmpMode set
    // match the old backupEcmpGroupType
    // However this adds more code for a use case we don't need to support.
    // BackupEcmpType can only change via a config update state delta. And
    // if that fails, we anyways fail the application
    std::stringstream ss;
    ss << "Update failed with backup switching mode transition, from: "
       << getBackupEcmpSwitchingMode()
       << " to: " << getBackupSwitchingMode(knownGoodState);
    throw FbossError(ss.str());
  }
  reconstructFromSwitchState(knownGoodState);
}

std::optional<EcmpResourceManager::InputOutputState>
EcmpResourceManager::handleFlowletSwitchConfigDelta(
    const StateDelta& delta,
    bool rollingBack) {
  auto oldBackupEcmpMode = getBackupEcmpSwitchingMode();
  config_.handleFlowletSwitchConfigDelta(delta);
  if (!oldBackupEcmpMode.has_value()) {
    // No backup ecmp type value for old group. Thus
    // we can't have any prefixes with override ECMP
    // mode set. Nothing to do.
    return std::nullopt;
  }
  InputOutputState inOutState(
      0 /*primaryEcmpGroupsCnt*/, 0 /*ecmpMemberCnt*/, delta, rollingBack);
  CHECK_EQ(inOutState.numDeltas(), 1);
  // Make changes on to current new state (which is essentially,
  // newState with old state's fibs). The first delta we will queue
  // will be the oldState's FIBs route's updated to new backup group.
  auto newState = inOutState.getCurrentStateDelta().newState();
  bool changed = false;
  for (const auto& [ridAndPfx, grpInfo] : prefixToGroupInfo_) {
    if (!grpInfo->isBackupEcmpGroupType()) {
      continue;
    } else {
      XLOG(DBG2) << ridAndPfx
                 << " updating backup ecmp mode, from: " << *oldBackupEcmpMode
                 << " to: " << getBackupEcmpSwitchingMode();
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
  inOutState.updated = changed;
  return changed ? std::move(inOutState) : std::optional<InputOutputState>();
}

void EcmpResourceManager::handleSwitchSettingsDelta(const StateDelta& delta) {
  auto compressionPenaltyBefore = getEcmpCompressionThresholdPct();
  config_.handleSwitchSettingsDelta(delta);
  if (!compressionPenaltyBefore && getEcmpCompressionThresholdPct()) {
    std::vector<NextHopGroupId> grpIds;
    std::for_each(
        nextHopGroupIdToInfo_.begin(),
        nextHopGroupIdToInfo_.end(),
        [&grpIds](const auto& grpIdAndInfo) {
          grpIds.emplace_back(grpIdAndInfo.first);
        });
    computeCandidateMergesForNewUnmergedGroups(grpIds);
  }
}

RouteNextHopSet EcmpResourceManager::getCommonNextHops(
    const NextHopGroupIds& grpIds) const {
  std::vector<const RouteNextHopSet*> unmergedNhopSets;
  std::for_each(
      grpIds.begin(), grpIds.end(), [&unmergedNhopSets, this](auto grpId) {
        auto nhopsInfo = nextHopGroupIdToInfo_.ref(grpId);
        CHECK(nhopsInfo);
        unmergedNhopSets.emplace_back(&nhopsInfo->getNhops());
      });

  return computeCommonNextHops(unmergedNhopSets);
}

EcmpResourceManager::ConsolidationInfo
EcmpResourceManager::computeConsolidationInfo(
    const NextHopGroupIds& grpIds) const {
  CHECK_GE(grpIds.size(), 1);
  auto firstGrpInfo = nextHopGroupIdToInfo_.ref(*grpIds.begin());
  CHECK(firstGrpInfo);

  std::vector<const RouteNextHopSet*> unmergedNhopSets;
  std::for_each(
      grpIds.begin(), grpIds.end(), [&unmergedNhopSets, this](auto grpId) {
        auto nhopsInfo = nextHopGroupIdToInfo_.ref(grpId);
        CHECK(nhopsInfo);
        unmergedNhopSets.emplace_back(&nhopsInfo->getNhops());
      });
  XLOG(DBG2) << " Computing consolidation penalties for: " << grpIds;
  ConsolidationInfo consolidationInfo;
  consolidationInfo.mergedNhops = computeCommonNextHops(unmergedNhopSets);
  for (auto grpId : grpIds) {
    const auto& grpInfo = nextHopGroupIdToInfo_.ref(grpId);
    auto penalty = computePenalty(
        grpInfo->getNhops().size(),
        consolidationInfo.mergedNhops.size(),
        grpInfo->getRouteUsageCount());
    XLOG(DBG4) << " For group : " << grpId
               << " orig nhops: " << grpInfo->getNhops().size()
               << " nhops lost: "
               << (grpInfo->getNhops().size() -
                   consolidationInfo.mergedNhops.size())
               << " penalty: " << penalty << "%";
    consolidationInfo.groupId2Penalty.insert({grpId, penalty});
  }
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
void EcmpResourceManager::computeCandidateMergesForNewUnmergedGroups(
    ForwardIt begin,
    ForwardIt end) {
  auto unmergedGroups = getUnMergedGids();
  XLOG(DBG2) << " Will compute candidate merges with unmerged groups : "
             << unmergedGroups;
  while (begin != end) {
    auto grpId = *begin++;
    for (auto grpToMergeWith : unmergedGroups) {
      NextHopGroupIds candidateMerge{grpId, grpToMergeWith};
      if (grpToMergeWith == grpId ||
          candidateMergeGroups_.contains(candidateMerge)) {
        continue;
      }
      addCandidateMerge(candidateMerge);
    }
    for (const auto& [mergedGrp, _] : mergedGroups_) {
      auto candidateMerge{mergedGrp};
      candidateMerge.insert(grpId);
      addCandidateMerge(candidateMerge);
    }
  }
}

void EcmpResourceManager::computeCandidateMergesForNewMergedGroup(
    const NextHopGroupIds& newMergeSet) {
  for (auto grpToMergeWith : getUnMergedGids()) {
    NextHopGroupIds candidateMerge{newMergeSet};
    auto [_, inserted] = candidateMerge.insert(grpToMergeWith);
    if (inserted && !candidateMergeGroups_.contains(candidateMerge)) {
      addCandidateMerge(candidateMerge);
    }
  }
}

void EcmpResourceManager::addCandidateMerge(
    const NextHopGroupIds& candidateMerge) {
  auto consolidationInfo = computeConsolidationInfo(candidateMerge);
  auto [_, inserted] = candidateMergeGroups_.insert(
      {candidateMerge, std::move(consolidationInfo)});
  if (inserted) {
    XLOG(DBG3) << " Added candidate merge group: " << candidateMerge;
  }
}

NextHopGroupInfo::NextHopGroupInfo(
    NextHopGroupId id,
    NextHopGroupItr ngItr,
    bool isBackupEcmpGroupType,
    std::optional<GroupIds2ConsolidationInfoItr> mergedGroupsToInfoItr)
    : id_(id),
      ngItr_(ngItr),
      isBackupEcmpGroupType_(isBackupEcmpGroupType),
      mergedGroupsToInfoItr_(mergedGroupsToInfoItr) {
  if (mergedGroupsToInfoItr_ &&
      (*mergedGroupsToInfoItr_)->second.mergedNhops == getNhops()) {
    state_ = NextHopGroupState::MERGED_NHOPS_ONLY;
  }
}

std::ostream& operator<<(
    std::ostream& os,
    NextHopGroupInfo::NextHopGroupState state) {
  switch (state) {
    case NextHopGroupInfo::NextHopGroupState::UNINITIALIZED:
      os << "UNINITIALIZED";
      break;
    case NextHopGroupInfo::NextHopGroupState::UNMERGED_NHOPS_ONLY:
      os << "UNMERGED_NHOPS_ONLY";
      break;
    case NextHopGroupInfo::NextHopGroupState::MERGED_NHOPS_ONLY:
      os << "MERGED_NHOPS_ONLY";
      break;
    case NextHopGroupInfo::NextHopGroupState::UNMERGED_AND_MERGED_NHOPS:
      os << "UNMERGED_AND_MERGED_NHOPS";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const NextHopGroupInfo& grpInfo) {
  os << "Group ID: " << grpInfo.getID() << " state: " << grpInfo.getState()
     << " override switching mode set: " << grpInfo.isBackupEcmpGroupType()
     << " override nhops points to merge grp: "
     << (grpInfo.getMergedGroupInfoItr()
             ? toStr((*grpInfo.getMergedGroupInfoItr())->first)
             : "none");
  return os;
}

void NextHopGroupInfo::routeUsageCountChanged(
    int prevRouteUsageCount,
    int curRouteUsageCount) {
  XLOG(DBG2) << " Updating state for group: " << getID()
             << " current state: " << state_;
  auto prevState = state_;
  switch (state_) {
    case NextHopGroupState::UNINITIALIZED:
      DCHECK_EQ(prevRouteUsageCount, 0);
      // From 0, routeUsageCount can only transition to 1
      DCHECK_EQ(curRouteUsageCount, 1);
      state_ = NextHopGroupState::UNMERGED_NHOPS_ONLY;
      break;
    case NextHopGroupState::UNMERGED_NHOPS_ONLY:
      // Nothing to do on route usage count change
      // for unmerged group nhops
      break;
    case NextHopGroupState::MERGED_NHOPS_ONLY:
      if (curRouteUsageCount) {
        // New unmerged nhop group has same nhops has a
        // merged nhop group.
        // If current state was MERGED_NHOPS_ONLY, the prior
        // route usage count must be 0
        DCHECK_EQ(prevRouteUsageCount, 0);
        // From 0, routeUsageCount can only transition to 1
        DCHECK_EQ(curRouteUsageCount, 1);
        DCHECK(mergedAndUnmergedNhopsMatch());
        state_ = NextHopGroupState::UNMERGED_AND_MERGED_NHOPS;
      }
      break;
    case NextHopGroupState::UNMERGED_AND_MERGED_NHOPS:
      if (!curRouteUsageCount) {
        state_ = NextHopGroupState::MERGED_NHOPS_ONLY;
      }
      break;
  }
  XLOG(DBG2) << "For group: " << getID()
             << " on route usage count update, state transitioned from: "
             << prevState << " to: " << state_ << ". Merge group points to: "
             << (mergedGroupsToInfoItr_
                     ? toStr((*mergedGroupsToInfoItr_)->first)
                     : " null")
             << ". Has backup ecmp mode set: " << isBackupEcmpGroupType();
}

void NextHopGroupInfo::mergeInfoItrChanged() {
  XLOG(DBG2) << " Updating state for group: " << getID()
             << " current state: " << state_;
  auto prevState = state_;
  switch (state_) {
    case NextHopGroupState::UNINITIALIZED:
      DCHECK_EQ(routeUsageCount_, 0);
      if (mergedAndUnmergedNhopsMatch()) {
        state_ = NextHopGroupState::MERGED_NHOPS_ONLY;
      }
      break;
    case NextHopGroupState::UNMERGED_NHOPS_ONLY:
      if (mergedAndUnmergedNhopsMatch()) {
        state_ = NextHopGroupState::UNMERGED_AND_MERGED_NHOPS;
      }
      break;
    case NextHopGroupState::MERGED_NHOPS_ONLY:
      // mergeItr should always be non null for MERGED_NHOPS_ONLY
      // Such a group can only transition to UNMERGED_AND_MERGED_NHOPS
      // on a route counter update or get deleted.
      // Setting mergeInfoItr to null would imply
      // a dangling nexthopgroup info.
      CHECK(mergedGroupsToInfoItr_.has_value());
      break;
    case NextHopGroupState::UNMERGED_AND_MERGED_NHOPS:
      // In such a state routeUsageCount must always be > 0,
      // since we are just updating the iterator.
      DCHECK_GT(routeUsageCount_, 0);
      if (!mergedAndUnmergedNhopsMatch()) {
        state_ = NextHopGroupState::UNMERGED_NHOPS_ONLY;
      }
      break;
  }
  XLOG(DBG2) << "For group: " << getID()
             << " on merge itr update, state transitioned from: " << prevState
             << " to: " << state_ << ". Merge group points to: "
             << (mergedGroupsToInfoItr_
                     ? toStr((*mergedGroupsToInfoItr_)->first)
                     : " null");
}

std::unique_ptr<EcmpResourceManager> makeEcmpResourceManager(
    const std::shared_ptr<SwitchState>& state,
    const HwAsic* asic,
    const EcmpResourceManager::SwitchStatsGetter& switchStatsGetter) {
  std::unique_ptr<EcmpResourceManager> ecmpResourceManager = nullptr;
  auto maxEcmpGroups = FLAGS_flowletSwitchingEnable ? asic->getMaxArsGroups()
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
  CHECK(
      !(switchingMode.has_value() &&
        ecmpCompressionPenaltyThresholPct.value_or(0)));
  if (maxEcmpGroups.has_value()) {
    auto percentage = FLAGS_flowletSwitchingEnable
        ? FLAGS_ars_resource_percentage
        : FLAGS_ecmp_resource_percentage;
    auto maxEcmps =
        std::floor(*maxEcmpGroups * static_cast<double>(percentage) / 100.0);
    XLOG(DBG2) << " Creating ecmp resource manager with max ECMP groups: "
               << maxEcmps << " and backup group type: " << switchingMode;

    ecmpResourceManager = switchingMode
        ? std::make_unique<EcmpResourceManager>(
              maxEcmps, switchingMode, switchStatsGetter)
        : std::make_unique<EcmpResourceManager>(
              maxEcmps,
              ecmpCompressionPenaltyThresholPct.value_or(0),
              switchStatsGetter);
  }
  return ecmpResourceManager;
}

std::string EcmpResourceManager::ConsolidationInfo::str() const {
  std::stringstream ss;
  ss << " Num Merged Nhops: " << mergedNhops.size() << std::endl;
  ss << " Penalties:  " << std::endl;
  for (const auto& [gid, penalty] : groupId2Penalty) {
    ss << " gid:  " << gid << " penalty: " << penalty << std::endl;
  }
  return ss.str();
}

std::string EcmpResourceManager::ConsolidationInfo::verboseStr() const {
  std::stringstream ss;
  ss << str();
  ss << " Merged Nhops: " << mergedNhops;
  return ss.str();
}

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::ConsolidationInfo& info) {
  os << info.str();
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
