/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <memory>
#include <ostream>
#include "fboss/agent/EcmpResourceManagerConfig.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

namespace facebook::fboss {
class StateDelta;
class SwitchState;
class SwitchStats;
class NextHopGroupInfo;
class HwAsic;
class Prefix;
class StateDelta;
template <typename AddrT>
class Route;

class EcmpResourceManager {
 public:
  using SwitchStatsGetter = std::function<SwitchStats*()>;

 private:
  EcmpResourceManager(
      const EcmpResourceManagerConfig& config,
      const SwitchStatsGetter& switchStatsGetter);

 public:
  explicit EcmpResourceManager(
      uint32_t maxHwEcmpGroups,
      int compressionPenaltyThresholdPct = 0,
      SwitchStatsGetter statsGetter = []() { return nullptr; })
      : EcmpResourceManager(
            EcmpResourceManagerConfig(
                maxHwEcmpGroups,
                std::nullopt,
                std::nullopt,
                compressionPenaltyThresholdPct),
            statsGetter) {}

  explicit EcmpResourceManager(
      uint32_t maxHwEcmpGroups,
      std::optional<cfg::SwitchingMode> backupEcmpGroupType = std::nullopt,
      SwitchStatsGetter statsGetter = []() { return nullptr; })
      : EcmpResourceManager(
            EcmpResourceManagerConfig(maxHwEcmpGroups, backupEcmpGroupType),
            statsGetter) {}
  using NextHopGroupId = uint64_t;
  using NextHopGroupIds = std::set<NextHopGroupId>;
  using NextHops2GroupId = std::map<RouteNextHopSet, NextHopGroupId>;
  using Prefix = std::pair<RouterID, folly::CIDRNetwork>;
  using PrefixToGroupInfo =
      std::unordered_map<Prefix, std::shared_ptr<NextHopGroupInfo>>;
  using NextHopGroupIdToPrefixes =
      std::unordered_map<NextHopGroupId, std::vector<Prefix>>;

  std::vector<StateDelta> modifyState(const std::vector<StateDelta>& deltas);
  std::vector<StateDelta> consolidate(
      const StateDelta& delta,
      bool rollingBack = false);
  std::vector<StateDelta> reconstructFromSwitchState(
      const std::shared_ptr<SwitchState>& curState);
  const auto& getNhopsToId() const {
    return nextHopGroup2Id_;
  }
  size_t getRouteUsageCount(NextHopGroupId nhopGrpId) const;
  size_t getCost(NextHopGroupId nhopGrpId) const;
  void updateDone();
  void updateFailed(const std::shared_ptr<SwitchState>& knownGoodState);
  std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode() const {
    return config_.getBackupEcmpSwitchingMode();
  }
  int32_t getEcmpCompressionThresholdPct() const {
    return config_.getEcmpCompressionThresholdPct();
  }
  uint32_t getMaxPrimaryEcmpGroups() const {
    return config_.getMaxPrimaryEcmpGroups();
  }

  struct ConsolidationInfo {
    int maxPenalty() const;
    bool operator==(const ConsolidationInfo& other) const {
      return std::tie(mergedNhops, groupId2Penalty) ==
          std::tie(other.mergedNhops, other.groupId2Penalty);
    }
    std::string str() const;
    std::string verboseStr() const;
    RouteNextHopSet mergedNhops;
    std::map<NextHopGroupId, int> groupId2Penalty;
    // mergedGroupInfo is set when the merged nhops get
    // used as part of a merged group
    std::shared_ptr<NextHopGroupInfo> mergedGroupInfo;
  };
  ConsolidationInfo computeConsolidationInfo(
      const NextHopGroupIds& grpIds) const;
  using GroupIds2ConsolidationInfo =
      std::map<NextHopGroupIds, ConsolidationInfo>;
  using GroupIds2ConsolidationInfoItr = GroupIds2ConsolidationInfo::iterator;
  NextHopGroupIds getUnMergedGids() const;
  NextHopGroupIds getMergedGids() const;
  std::vector<NextHopGroupIds> getMergedGroups() const;
  std::pair<uint32_t, uint32_t> getPrimaryEcmpAndMemberCounts() const;
  NextHopGroupIdToPrefixes getGidToPrefixes() const;
  /*
   * Test helper APIs. Used mainly in UTs. Not neccessarily opimized for
   * non test code.
   */
  std::optional<ConsolidationInfo> getMergeGroupConsolidationInfo(
      NextHopGroupId grpId) const;
  GroupIds2ConsolidationInfo getCandidateMergeConsolidationInfo(
      NextHopGroupId grpId) const;
  std::set<NextHopGroupId> getOptimalMergeGroupSet() const;
  const NextHopGroupInfo* getGroupInfo(
      RouterID rid,
      const folly::CIDRNetwork& nw) const;
  const NextHopGroupInfo* getGroupInfo(NextHopGroupId gid) const {
    auto grpInfo = nextHopGroupIdToInfo_.ref(gid);
    return grpInfo ? grpInfo.get() : nullptr;
  }
  /* Test helper API end */

 private:
  RouteNextHopSet getCommonNextHops(const NextHopGroupIds& grpIds) const;
  struct InputOutputState {
    InputOutputState(
        uint32_t _primaryEcmpGroupsCnt,
        uint32_t ecmpMemberCnt,
        const StateDelta& _in,
        bool rollingBack = false);
    /*
     * addOrUpdateRoute has 1 interesting knobs
     * addNewDelta - This route update should be placed on a new
     * delta. This only applies to when we are merging groups at
     * ECMP limit. When doing so we will
     * a. Create a merged group
     * b. Move all routes that point to any members of this merged
     * group to this new group.
     * In transient state this creates 2 extra groups -
     * New merged ECMP group
     * Group that was merged into the new ECMP group
     * This is exactly the make before break buffer we have.
     * We need this update to be applied fully before we process
     * more routes. Since multiple routes maybe migrated to new
     * merged ECMP group and we cannot rely on ordered processing
     * of routes to take care of not tipping over the limit.
     * New delta creation *does not apply* for spillover to backup
     * ECMP type since, on ecmp demand exceeding, we always spillover
     * the new incoming route to backup ecmp type (new group only has
     * one route pointing to it and there is nothing cheaper than a
     * group pointed to by one route).
     */
    template <typename AddrT>
    void addOrUpdateRoute(
        RouterID rid,
        const std::shared_ptr<Route<AddrT>>& newRoute,
        bool addNewDelta = false);

    template <typename AddrT>
    void deleteRoute(
        RouterID rid,
        const std::shared_ptr<Route<AddrT>>& delRoute);
    /*
     * StateDelta to use as base state when building the
     * next delta or updating current delta.
     */
    StateDelta getCurrentStateDelta() const;
    /*
     * We adopt a lazy publish strategy for InputOutputState.out deltas. In
     * that, when a change is made (say route update),
     *  - We clone the state to make this change on
     *  - Make the change.
     *  At this point, we *don't publish* this new state, but just
     *  replace/append the cur delta to list of deltas.
     *  We then publish at a later point
     *  - When appending a new delta, before appending, we publish
     *  the last delta in queue.
     *  - When finishing a functional block, e.g.
     *  finishing route processing, reclaimingEcmpGroups we publish
     *  the last delta in queue.
     * Lazy publishing is much more efficient than a simpler always
     * (eager) publishing approach. Consider the eager publish approach
     * where we publish after every change. Now consider that we
     * get a large route update (say 50K routes). Most of these will
     * just be grafted onto current delta. If we publish after every
     * individual route update, the next update will end up cloning
     * the FIB and copying the prefix->shared_ptr<Route> for FIB
     * for every update. This is a expensive O(N^2) operation.
     */
    /*
     * Publish last delta in queue
     * */
    void publishLastDelta();
    /*
     * Append to current set of queued deltas
     * */
    void appendDelta(const StateDelta& delta);
    /*
     * Replace current delta and back of out_ queue
     * */
    void replaceLastDelta(const StateDelta& delta);
    size_t numDeltas() const {
      return out_.size();
    }

    /*
     * Return and empty out_ deltas
     */
    std::vector<StateDelta> moveDeltas() {
      return std::move(out_);
    }
    bool rollingBack() const {
      return rollingBack_;
    }
    /*
     * Number of ECMP groups of primary ECMP type. Once these
     * reach the maxEcmpGroups limit, we either compress groups
     * by combining 2 or more groups.
     */
    uint32_t primaryEcmpGroupsCnt{0};
    uint32_t ecmpMemberCnt{0};
    bool updated{false};

   private:
    std::vector<StateDelta> out_;
    bool rollingBack_{false};
  };
  std::pair<std::shared_ptr<NextHopGroupInfo>, bool> getOrCreateGroupInfo(
      const RouteNextHopSet& nhops,
      const InputOutputState& inOutState);
  bool pruneFromCandidateMerges(const NextHopGroupIds& groupIds);
  template <typename AddrT>
  bool routeFwdEqual(
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::shared_ptr<Route<AddrT>>& newRoute) const;

  void decRouteUsageCount(NextHopGroupInfo& groupInfo);
  void updateConsolidationPenalty(NextHopGroupInfo& groupInfo);
  bool checkPrimaryGroupAndMemberCounts(
      const InputOutputState& inOutState) const;
  bool checkNoUnitializedGroups() const;
  std::optional<InputOutputState> handleFlowletSwitchConfigDelta(
      const StateDelta& delta,
      bool rollingBack);
  void handleSwitchSettingsDelta(const StateDelta& delta);
  std::vector<StateDelta> consolidateImpl(
      const StateDelta& delta,
      InputOutputState* inOutState);
  std::vector<std::shared_ptr<NextHopGroupInfo>> getGroupsToReclaimOrdered(
      uint32_t canReclaim) const;
  void reclaimBackupGroups(
      const std::vector<std::shared_ptr<NextHopGroupInfo>>& toReclaimSorted,
      const NextHopGroupIds& groupIdsToReclaimIn,
      InputOutputState* inOutState);

  /*
   * Migrate groups and prefixes to new merge info
   */

  void updateMergeInfo(
      const NextHopGroupIdToPrefixes& gid2Prefix,
      const NextHopGroupIds& newMergeSet,
      std::optional<GroupIds2ConsolidationInfoItr> newMergeItr,
      std::shared_ptr<SwitchState>& newState);

  /*
   * Lookup merge iterator for a set of next hops
   */
  std::optional<GroupIds2ConsolidationInfoItr> getMergeGroupItr(
      const RouteNextHopSet& mergedNhops);
  std::optional<GroupIds2ConsolidationInfoItr> getMergeGroupItr(
      const std::optional<RouteNextHopSet>& mergedNhops) {
    return mergedNhops ? getMergeGroupItr(*mergedNhops) : std::nullopt;
  }
  /*
   * Append to or create new merge group and get its iterator.
   * Only updates internal data structures, no delta update
   */
  GroupIds2ConsolidationInfoItr appendToOrCreateMergeGroup(
      NextHopGroupIds newMemberGroups,
      const RouteNextHopSet& mergedNhops,
      std::optional<GroupIds2ConsolidationInfoItr> existingMitr,
      const InputOutputState& inOutState);
  /*
   * Update merge itertor for a set of groups
   */
  void fixMergeItreators(
      const NextHopGroupIds& newMergeSet,
      GroupIds2ConsolidationInfoItr mitr,
      const NextHopGroupIds& toIgnore);
  /*
   * Reclaim any single member merge groups that may be left over
   * during rollbacks
   */
  void reclaimSingleMemberMergeGroups(InputOutputState* inOutState);
  /*
   * Unmerge and reclaim a set of merge groups
   */
  void reclaimMergeGroups(
      const std::vector<std::shared_ptr<NextHopGroupInfo>>& toReclaimSorted,
      const NextHopGroupIds& groupIdsToReclaim,
      InputOutputState* inOutState);
  enum class MergeGroupUpdateOp { RECLAIM_GROUPS, DELETE_GROUPS };
  /*
   * Reclaim or delete a set of groups from existing merged
   * groups.
   */
  void updateMergedGroups(
      const std::set<NextHopGroupIds>& mergeSetsToUpdate,
      MergeGroupUpdateOp op,
      const NextHopGroupIds& groupIdsToReclaim,
      InputOutputState* inOutState);
  /*
   * Merge a set of groups to make a larger set.
   * Update the corresponding prefixes to
   * point to the new merged next hops
   */
  void mergeGroupAndMigratePrefixes(
      const NextHopGroupIds& mergeSet,
      InputOutputState* inOutState);
  /*
   * Pick a optimal set of groups to merge (based on cost).
   * Merge them and update prefixes to point to the new
   * merged next hops
   */
  void mergeGroupAndMigratePrefixes(InputOutputState* inOutState);

  void reclaimEcmpGroups(InputOutputState* inOutState);
  template <typename AddrT>
  std::shared_ptr<NextHopGroupInfo> updateForwardingInfoAndInsertDelta(
      RouterID rid,
      const folly::CIDRNetwork& pfx,
      std::shared_ptr<NextHopGroupInfo>& grpInfo,
      InputOutputState* inOutState,
      bool addNewDelta);
  template <typename AddrT>
  std::shared_ptr<NextHopGroupInfo> updateForwardingInfoAndInsertDelta(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& route,
      std::shared_ptr<NextHopGroupInfo>& grpInfo,
      bool ecmpDemandExceeded,
      InputOutputState* inOutState,
      bool addNewDelta = false);
  template <typename AddrT>
  std::shared_ptr<NextHopGroupInfo> ecmpGroupDemandExceeded(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& route,
      NextHops2GroupId::iterator nhops2IdItr,
      InputOutputState* inOutState);
  void processRouteUpdates(
      const StateDelta& delta,
      InputOutputState* inOutState);
  template <typename AddrT>
  void routeUpdated(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::shared_ptr<Route<AddrT>>& newRoute,
      InputOutputState* inOutState);
  template <typename AddrT>
  void routeAdded(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& newRoute,
      InputOutputState* inOutState);
  template <typename AddrT>
  void routeAddedOrUpdated(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::shared_ptr<Route<AddrT>>& added,
      InputOutputState* inOutState);

  template <typename AddrT>
  std::pair<std::shared_ptr<NextHopGroupInfo>, bool>
  routeAddedNoCompressionThreshold(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& added,
      bool ecmpLimitReached,
      InputOutputState* inOutState);

  template <typename AddrT>
  std::pair<std::shared_ptr<NextHopGroupInfo>, bool> routeAddedNoOverrideNhops(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& added,
      bool ecmpLimitReached,
      InputOutputState* inOutState);

  template <typename AddrT>
  std::pair<std::shared_ptr<NextHopGroupInfo>, bool>
  routeAddedWithOverrideNhops(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& added,
      bool ecmpLimitReached,
      InputOutputState* inOutState);

  template <typename AddrT>
  void routeDeleted(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& removed,
      bool isUpdate,
      InputOutputState* inOutState);
  static uint32_t constexpr kMinNextHopGroupId = 1;
  NextHopGroupId findCachedOrNewIdForNhops(
      const RouteNextHopSet& nhops,
      const InputOutputState& inOutState) const;
  void validateCfgUpdate(
      uint32_t compressionPenaltyThresholdPct,
      const std::optional<cfg::SwitchingMode>& backupEcmpGroupType) const;
  template <std::forward_iterator ForwardIt>
  void computeCandidateMergesForNewUnmergedGroups(
      ForwardIt begin,
      ForwardIt end);
  void computeCandidateMergesForNewUnmergedGroups(
      const std::vector<NextHopGroupId>& gids) {
    computeCandidateMergesForNewUnmergedGroups(gids.begin(), gids.end());
  }
  void computeCandidateMergesForNewMergedGroup(
      const NextHopGroupIds& newMergeSet);
  void addCandidateMerge(const NextHopGroupIds& candidateMerge);

  NextHops2GroupId nextHopGroup2Id_;
  StdRefMap<NextHopGroupId, NextHopGroupInfo> nextHopGroupIdToInfo_;
  PrefixToGroupInfo prefixToGroupInfo_;
  std::map<NextHopGroupIds, ConsolidationInfo> mergedGroups_;
  std::map<NextHopGroupIds, ConsolidationInfo> candidateMergeGroups_;
  SwitchStatsGetter statsGetter_;
  EcmpResourceManagerConfig config_;
};

class NextHopGroupInfo {
 public:
  using NextHopGroupId = EcmpResourceManager::NextHopGroupId;
  using NextHopGroupItr = EcmpResourceManager::NextHops2GroupId::iterator;
  using GroupIds2ConsolidationInfoItr =
      EcmpResourceManager::GroupIds2ConsolidationInfo::iterator;

  enum class NextHopGroupState {
    UNINITIALIZED,
    UNMERGED_NHOPS_ONLY,
    MERGED_NHOPS_ONLY,
    // Matching MERGED and UMERGED NHOPS group
    UNMERGED_AND_MERGED_NHOPS,
  };
  NextHopGroupInfo(
      NextHopGroupId id,
      NextHopGroupItr ngItr,
      bool isBackupEcmpGroupType = false,
      std::optional<GroupIds2ConsolidationInfoItr> mergedGroupsToInfoItr =
          std::nullopt);

  bool isUninitialized() const {
    return state_ == NextHopGroupState::UNINITIALIZED;
  }
  bool hasUnmergedNhops() const {
    return state_ == NextHopGroupState::UNMERGED_NHOPS_ONLY ||
        state_ == NextHopGroupState::UNMERGED_AND_MERGED_NHOPS;
  }
  bool hasMergedNhops() const {
    return state_ == NextHopGroupState::MERGED_NHOPS_ONLY ||
        state_ == NextHopGroupState::UNMERGED_AND_MERGED_NHOPS;
  }
  bool hasMergedNhopsOnly() const {
    return state_ == NextHopGroupState::MERGED_NHOPS_ONLY;
  }
  NextHopGroupId getID() const {
    return id_;
  }
  size_t getRouteUsageCount() const {
    CHECK_GE(routeUsageCount_, 0);
    return routeUsageCount_;
  }
  void incRouteUsageCount() {
    CHECK_GE(routeUsageCount_, 0);
    ++routeUsageCount_;
    routeUsageCountChanged(routeUsageCount_ - 1, routeUsageCount_);
  }
  void decRouteUsageCount() {
    CHECK_GT(routeUsageCount_, 0);
    --routeUsageCount_;
    routeUsageCountChanged(routeUsageCount_ + 1, routeUsageCount_);
  }
  bool isBackupEcmpGroupType() const {
    return isBackupEcmpGroupType_;
  }
  void setIsBackupEcmpGroupType(bool isBackupEcmp) {
    isBackupEcmpGroupType_ = isBackupEcmp;
  }
  void setMergedGroupInfoItr(
      std::optional<GroupIds2ConsolidationInfoItr> gitr) {
    mergedGroupsToInfoItr_ = gitr;
    mergeInfoItrChanged();
  }
  std::optional<GroupIds2ConsolidationInfoItr> getMergedGroupInfoItr() const {
    return mergedGroupsToInfoItr_;
  }
  const RouteNextHopSet& getNhops() const {
    return ngItr_->first;
  }
  size_t numNhops() const {
    return getNhops().size();
  }
  bool hasOverrideNextHops() const {
    return mergedGroupsToInfoItr_.has_value();
  }
  bool hasOverrides() const {
    return isBackupEcmpGroupType() || hasOverrideNextHops();
  }

  int cost() const {
    return mergedGroupsToInfoItr_.has_value()
        ? (*mergedGroupsToInfoItr_)->second.maxPenalty()
        : routeUsageCount_;
  }

  NextHopGroupState getState() const {
    return state_;
  }
  std::optional<RouteNextHopSet> getOverrideNextHops() const {
    std::optional<RouteNextHopSet> overrideNhops;
    if (mergedGroupsToInfoItr_.has_value()) {
      overrideNhops = (*mergedGroupsToInfoItr_)->second.mergedNhops;
    }
    return overrideNhops;
  }

  bool mergedAndUnmergedNhopsMatch() const {
    return mergedGroupsToInfoItr_ &&
        (*mergedGroupsToInfoItr_)->second.mergedNhops == getNhops();
  }

 private:
  void routeUsageCountChanged(int prevRouteUsageCount, int curRouteUsageCount);
  void mergeInfoItrChanged();
  NextHopGroupId id_;
  NextHopGroupItr ngItr_;
  bool isBackupEcmpGroupType_{false};
  std::optional<GroupIds2ConsolidationInfoItr> mergedGroupsToInfoItr_;
  int routeUsageCount_{0};
  NextHopGroupState state_{NextHopGroupState::UNINITIALIZED};
};

std::unique_ptr<EcmpResourceManager> makeEcmpResourceManager(
    const std::shared_ptr<SwitchState>& state,
    const HwAsic* asic,
    const EcmpResourceManager::SwitchStatsGetter& switchStatsGetter = []() {
      return nullptr;
    });

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::ConsolidationInfo& info);

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::Prefix& pfx);

std::ostream& operator<<(
    std::ostream& os,
    NextHopGroupInfo::NextHopGroupState state);

std::ostream& operator<<(std::ostream& os, const NextHopGroupInfo& grpInfo);
} // namespace facebook::fboss
