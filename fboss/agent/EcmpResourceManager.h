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
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/lib/RefMap.h"

#include <boost/container/flat_set.hpp>
#include <memory>

namespace facebook::fboss {
class StateDelta;
class SwitchState;

class NextHopGroupInfo {
 public:
  using NextHopGroupId = uint32_t;
  using NextHopGroupItr = std::map<RouteNextHopSet, NextHopGroupId>::iterator;
  NextHopGroupInfo(
      NextHopGroupId id,
      NextHopGroupItr ngItr,
      bool isBackupEcmpGroupType = false)
      : id_(id), ngItr_(ngItr), isBackupEcmpGroupType_(isBackupEcmpGroupType) {}
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

 private:
  static constexpr int kInvalidRouteUsageCount = 0;
  NextHopGroupId id_;
  NextHopGroupItr ngItr_;
  bool isBackupEcmpGroupType_{false};
  int routeUsageCount_{kInvalidRouteUsageCount};
};

class EcmpResourceManager {
  // Keep some buffer from HW limit for make before break
  // nature of ECMP.
  static auto constexpr kEcmpMakeBeforeBreakBuffer = 2;

 public:
  explicit EcmpResourceManager(
      uint32_t maxHwEcmpGroups,
      int compressionPenaltyThresholdPct = 0,
      std::optional<cfg::SwitchingMode> backupEcmpGroupType = std::nullopt)
      // We keep a buffer of 2 for transient increment in ECMP groups when
      // pushing updates down to HW
      : maxEcmpGroups_(maxHwEcmpGroups - kEcmpMakeBeforeBreakBuffer),
        compressionPenaltyThresholdPct_(compressionPenaltyThresholdPct),
        backupEcmpGroupType_(backupEcmpGroupType) {
    CHECK_GT(maxHwEcmpGroups, kEcmpMakeBeforeBreakBuffer);
    CHECK_EQ(compressionPenaltyThresholdPct_, 0)
        << " Group compression algo is WIP";
  }
  using NextHopGroupId = uint32_t;
  using NextHopGroupIds = boost::container::flat_set<NextHopGroupId>;
  using NextHops2GroupId = std::map<RouteNextHopSet, NextHopGroupId>;

  std::vector<StateDelta> consolidate(const StateDelta& delta);
  const auto& getNhopsToId() const {
    return nextHopGroup2Id_;
  }
  size_t getRouteUsageCount(NextHopGroupId nhopGrpId) const;
  void updateDone(const StateDelta& delta);
  void updateFailed(const StateDelta& delta);

 private:
  struct ConsolidationPenalty {
    int maxPenalty() const;
    int avgPenalty() const;
    std::map<NextHopGroupId, int> groupId2Penalty;
  };
  struct PreUpdateState {
    std::map<NextHopGroupIds, ConsolidationPenalty> mergedGroups;
    std::map<RouteNextHopSet, NextHopGroupId> nextHopGroup2Id;
  };
  struct InputOutputState {
    InputOutputState(uint32_t _nonBackupEcmpGroupsCnt, const StateDelta& _in)
        : nonBackupEcmpGroupsCnt(_nonBackupEcmpGroupsCnt) {
      /*
       * Note that for first StateDelta we push in.oldState() for both
       * old and new state in the first StateDelta, since we will process
       * and add/update/delete routes on top of the old state.
       */
      out.emplace_back(_in.oldState(), _in.oldState());
    }
    template <typename AddrT>
    void addOrUpdateRoute(
        RouterID rid,
        const std::shared_ptr<Route<AddrT>>& newRoute,
        bool ecmpDemandExceeded);

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
    uint32_t nonBackupEcmpGroupsCnt;
    std::vector<StateDelta> out;
  };
  std::set<NextHopGroupId> createOptimalMergeGroupSet();
  template <typename AddrT>
  std::shared_ptr<NextHopGroupInfo> ecmpGroupDemandExceeded(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& route,
      NextHops2GroupId::iterator nhops2IdItr,
      InputOutputState* inOutState);
  template <typename AddrT>
  void processRouteUpdates(
      const StateDelta& delta,
      InputOutputState* inOutState);
  template <typename AddrT>
  void routeAdded(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& added,
      InputOutputState* inOutState);
  template <typename AddrT>
  void routeDeleted(
      RouterID rid,
      const std::shared_ptr<Route<AddrT>>& removed,
      InputOutputState* inOutState);
  static uint32_t constexpr kMinNextHopGroupId = 1;
  NextHopGroupId findNextAvailableId() const;
  NextHops2GroupId nextHopGroup2Id_;
  StdRefMap<NextHopGroupId, NextHopGroupInfo> nextHopGroupIdToInfo_;
  std::unordered_map<folly::CIDRNetwork, std::shared_ptr<NextHopGroupInfo>>
      prefixToGroupInfo_;
  std::map<NextHopGroupIds, ConsolidationPenalty> mergedGroups_;
  std::map<NextHopGroupIds, ConsolidationPenalty> candidateMergeGroups_;
  // Cached pre update state, will be used in case of roll back
  // if update fails
  std::optional<PreUpdateState> preUpdateState_;
  // Knobs to control resource mgt policy
  uint32_t maxEcmpGroups_{0};
  int compressionPenaltyThresholdPct_{0};
  std::optional<cfg::SwitchingMode> backupEcmpGroupType_;
};
} // namespace facebook::fboss
