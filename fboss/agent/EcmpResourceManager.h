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
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/PreUpdateStateModifier.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/lib/RefMap.h"

#include <memory>
#include <ostream>

namespace facebook::fboss {
class StateDelta;
class SwitchState;
class SwitchStats;
class NextHopGroupInfo;

class EcmpResourceManagerConfig {
 public:
  EcmpResourceManagerConfig(
      uint32_t maxHwEcmpGroups,
      std::optional<cfg::SwitchingMode> backupEcmpGroupType);
  EcmpResourceManagerConfig(
      uint32_t maxHwEcmpGroups,
      std::optional<uint32_t> maxHwEcmpMembers,
      std::optional<uint32_t> maxEcmpWidth,
      int compressionPenaltyThresholdPct);
  explicit EcmpResourceManagerConfig(uint32_t maxHwEcmpGroups)
      : EcmpResourceManagerConfig(maxHwEcmpGroups, std::nullopt) {}

  bool ecmpLimitReached(uint32_t primaryEcmpGroups, uint32_t ecmpGroupMembers)
      const;

  std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode() const {
    return backupEcmpGroupType_;
  }
  int32_t getEcmpCompressionThresholdPct() const {
    return compressionPenaltyThresholdPct_;
  }
  uint32_t getMaxPrimaryEcmpGroups() const {
    return maxHwEcmpGroups_;
  }

 private:
  static uint32_t computeMaxHwEcmpGroups(uint32_t maxHwEcmpGroups);
  static std::optional<uint32_t> computeMaxHwEcmpMembers(
      std::optional<uint32_t> maxHwEcmpMembers,
      std::optional<uint32_t> maxEcmpWidth);

  const uint32_t maxHwEcmpGroups_;
  const std::optional<uint32_t> maxHwEcmpMembers_;
  const uint32_t compressionPenaltyThresholdPct_{0};
  const std::optional<cfg::SwitchingMode> backupEcmpGroupType_;
};

class EcmpResourceManager : public PreUpdateStateModifier {
 private:
  EcmpResourceManager(
      const EcmpResourceManagerConfig& config,
      SwitchStats* stats);

 public:
  explicit EcmpResourceManager(
      uint32_t maxHwEcmpGroups,
      int compressionPenaltyThresholdPct = 0,
      SwitchStats* stats = nullptr)
      : EcmpResourceManager(
            EcmpResourceManagerConfig(
                maxHwEcmpGroups,
                std::nullopt,
                std::nullopt,
                compressionPenaltyThresholdPct),
            stats) {}

  explicit EcmpResourceManager(
      uint32_t maxHwEcmpGroups,
      std::optional<cfg::SwitchingMode> backupEcmpGroupType = std::nullopt,
      SwitchStats* stats = nullptr)
      : EcmpResourceManager(
            EcmpResourceManagerConfig(maxHwEcmpGroups, backupEcmpGroupType),
            stats) {}
  using NextHopGroupId = uint32_t;
  using NextHopGroupIds = std::set<NextHopGroupId>;
  using NextHops2GroupId = std::map<RouteNextHopSet, NextHopGroupId>;
  using Prefix = std::pair<RouterID, folly::CIDRNetwork>;
  using PrefixToGroupInfo =
      std::unordered_map<Prefix, std::shared_ptr<NextHopGroupInfo>>;

  std::vector<StateDelta> modifyState(
      const std::vector<StateDelta>& deltas) override;
  std::vector<StateDelta> consolidate(const StateDelta& delta);
  std::vector<StateDelta> reconstructFromSwitchState(
      const std::shared_ptr<SwitchState>& curState) override;
  const auto& getNhopsToId() const {
    return nextHopGroup2Id_;
  }
  size_t getRouteUsageCount(NextHopGroupId nhopGrpId) const;
  size_t getCost(NextHopGroupId nhopGrpId) const;
  void updateDone() override;
  void updateFailed(const std::shared_ptr<SwitchState>& curState) override;
  std::optional<cfg::SwitchingMode> getBackupEcmpSwitchingMode() const {
    return backupEcmpGroupType_;
  }
  int32_t getEcmpCompressionThresholdPct() const {
    return compressionPenaltyThresholdPct_;
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
    RouteNextHopSet mergedNhops;
    std::map<NextHopGroupId, int> groupId2Penalty;
  };
  ConsolidationInfo computeConsolidationInfo(
      const NextHopGroupIds& grpIds) const;
  using GroupIds2ConsolidationInfo =
      std::map<NextHopGroupIds, ConsolidationInfo>;
  using GroupIds2ConsolidationInfoItr = GroupIds2ConsolidationInfo::iterator;
  NextHopGroupIds getUnMergedGids() const;
  NextHopGroupIds getMergedGids() const;
  std::vector<NextHopGroupIds> getMergedGroups() const;
  /*
   * Test helper APIs. Used mainly in UTs. Not neccessarily opimized for
   * non test code.
   */
  std::optional<ConsolidationInfo> getMergeGroupConsolidationInfo(
      NextHopGroupId grpId) const;
  GroupIds2ConsolidationInfo getCandidateMergeConsolidationInfo(
      NextHopGroupId grpId) const;
  std::set<NextHopGroupId> getOptimalMergeGroupSet() const;
  std::map<NextHopGroupId, std::set<Prefix>> getGroupIdToPrefix() const;
  const NextHopGroupInfo* getGroupInfo(
      RouterID rid,
      const folly::CIDRNetwork& nw) const;
  /* Test helper API end */

 private:
  friend class BaseEcmpResourceManagerTest;
  std::pair<uint32_t, uint32_t> getPrimaryEcmpAndMemberCounts() const;
  GroupIds2ConsolidationInfoItr fixAndGetMergeGroupItr(
      const NextHopGroupId newMemberGroup,
      const RouteNextHopSet& mergedNhops);
  void fixMergeItreators(
      const NextHopGroupIds& newMergeSet,
      GroupIds2ConsolidationInfoItr mitr,
      const NextHopGroupIds& toIgnore);
  bool pruneFromCandidateMerges(const NextHopGroupIds& groupIds);
  template <typename AddrT>
  bool routeFwdEqual(
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::shared_ptr<Route<AddrT>>& newRoute) const;

  struct PreUpdateState {
    std::map<NextHopGroupIds, ConsolidationInfo> mergedGroups;
    std::map<RouteNextHopSet, NextHopGroupId> nextHopGroup2Id;
    std::optional<cfg::SwitchingMode> backupEcmpGroupType;
  };
  void decRouteUsageCount(NextHopGroupInfo& groupInfo);
  void updateConsolidationPenalty(NextHopGroupInfo& groupInfo);
  struct InputOutputState {
    InputOutputState(
        uint32_t _primaryEcmpGroupsCnt,
        uint32_t ecmpMemberCnt,
        const StateDelta& _in,
        const PreUpdateState& _groupIdCache = PreUpdateState());
    /*
     * addOrUpdateRoute has 2 interesting knobs
     * ecmpDemandExceeded - used for checking that new route now
     * either has overrideEcmpType set or points to a merged group
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
        bool ecmpDemandExceeded,
        bool addNewDelta = false);

    template <typename AddrT>
    void deleteRoute(
        RouterID rid,
        const std::shared_ptr<Route<AddrT>>& delRoute);
    /*
     * StateDelta to use as base state when building the
     * next delta or updating current delta.
     */
    StateDelta getCurrentStateDelta() const {
      CHECK(!out.empty());
      return StateDelta(out.back().oldState(), out.back().newState());
    }
    /*
     * Number of ECMP groups of primary ECMP type. Once these
     * reach the maxEcmpGroups limit, we either compress groups
     * by combining 2 or more groups.
     */
    uint32_t primaryEcmpGroupsCnt{0};
    uint32_t ecmpMemberCnt{0};
    std::vector<StateDelta> out;
    PreUpdateState groupIdCache;
    bool updated{false};
  };
  bool checkPrimaryGroupAndMemberCounts(
      const InputOutputState& inOutState) const;
  std::optional<InputOutputState> handleFlowletSwitchConfigDelta(
      const StateDelta& delta);
  void handleSwitchSettingsDelta(const StateDelta& delta);
  std::vector<StateDelta> consolidateImpl(
      const StateDelta& delta,
      InputOutputState* inOutState);
  std::vector<std::shared_ptr<NextHopGroupInfo>> getGroupsToReclaimOrdered(
      uint32_t canReclaim) const;
  void mergeGroupAndMigratePrefixes(InputOutputState* inOutState);
  void reclaimBackupGroups(
      const std::vector<std::shared_ptr<NextHopGroupInfo>>& toReclaimSorted,
      const NextHopGroupIds& groupIdsToReclaimIn,
      InputOutputState* inOutState);
  void reclaimMergeGroups(
      const std::vector<std::shared_ptr<NextHopGroupInfo>>& toReclaimSorted,
      const NextHopGroupIds& groupIdsToReclaim,
      InputOutputState* inOutState);
  enum class MergeGroupUpdateOp { RECLAIM_GROUPS, DELETE_GROUPS };
  void updateMergedGroups(
      const std::set<NextHopGroupIds>& mergeSetsToUpdate,
      MergeGroupUpdateOp op,
      const NextHopGroupIds& groupIdsToReclaim,
      InputOutputState* inOutState);
  std::unordered_map<NextHopGroupId, std::vector<Prefix>> getGidToPrefixes()
      const;
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
      NextHops2GroupId::iterator nhops2IdItr,
      bool ecmpDemandExceeded,
      InputOutputState* inOutState);
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
      int32_t compressionPenaltyThresholdPct,
      const std::optional<cfg::SwitchingMode>& backupEcmpGroupType) const;
  NextHopGroupId findNextAvailableId() const;
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
  // Cached pre update state, will be used in case of roll back
  // if update fails
  std::optional<PreUpdateState> preUpdateState_;
  int compressionPenaltyThresholdPct_{0};
  std::optional<cfg::SwitchingMode> backupEcmpGroupType_;
  SwitchStats* switchStats_;
  EcmpResourceManagerConfig config_;
};

class NextHopGroupInfo {
 public:
  using NextHopGroupId = EcmpResourceManager::NextHopGroupId;
  using NextHopGroupItr = EcmpResourceManager::NextHops2GroupId::iterator;
  using GroupIds2ConsolidationInfoItr =
      EcmpResourceManager::GroupIds2ConsolidationInfo::iterator;
  NextHopGroupInfo(
      NextHopGroupId id,
      NextHopGroupItr ngItr,
      bool isBackupEcmpGroupType = false,
      std::optional<GroupIds2ConsolidationInfoItr> mergedGroupsToInfoItr =
          std::nullopt)
      : id_(id),
        ngItr_(ngItr),
        isBackupEcmpGroupType_(isBackupEcmpGroupType),
        mergedGroupsToInfoItr_(mergedGroupsToInfoItr) {}
  NextHopGroupId getID() const {
    return id_;
  }
  size_t getRouteUsageCount() const {
    CHECK_GT(routeUsageCount_, 0);
    return routeUsageCount_;
  }
  void incRouteUsageCount() {
    ++routeUsageCount_;
  }
  void decRouteUsageCount() {
    --routeUsageCount_;
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

  std::optional<RouteNextHopSet> getOverrideNextHops() const {
    std::optional<RouteNextHopSet> overrideNhops;
    if (mergedGroupsToInfoItr_.has_value()) {
      overrideNhops = (*mergedGroupsToInfoItr_)->second.mergedNhops;
    }
    return overrideNhops;
  }

 private:
  static constexpr int kInvalidRouteUsageCount = 0;
  NextHopGroupId id_;
  NextHopGroupItr ngItr_;
  bool isBackupEcmpGroupType_{false};
  std::optional<GroupIds2ConsolidationInfoItr> mergedGroupsToInfoItr_;
  int routeUsageCount_{kInvalidRouteUsageCount};
};

std::unique_ptr<EcmpResourceManager> makeEcmpResourceManager(
    const std::shared_ptr<SwitchState>& state,
    const HwAsic* asic,
    SwitchStats* stats = nullptr);

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::ConsolidationInfo& info);

std::ostream& operator<<(
    std::ostream& os,
    const EcmpResourceManager::Prefix& pfx);
} // namespace facebook::fboss
