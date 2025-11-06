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

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"

#include <gtest/gtest.h>

DECLARE_bool(intf_nbr_tables);

namespace facebook::fboss {

class ResourceAccountant {
 public:
  ResourceAccountant(
      const HwAsicTable* asicTable,
      const SwitchIdScopeResolver* scopeResolver);

  bool isValidUpdate(const StateDelta& delta);
  void stateChanged(const StateDelta& delta);

 private:
  size_t getMemberCountForEcmpGroup(const RouteNextHopEntry& fwd) const;
  bool checkEcmpResource(bool intermediateState) const;
  bool checkArsResource(bool intermediateState) const;
  bool routeAndEcmpStateChangedImpl(const StateDelta& delta);
  bool isValidRouteUpdate(const StateDelta& delta);
  bool shouldCheckRouteUpdate() const;
  bool isEcmp(const RouteNextHopEntry& fwd) const;
  size_t computeWeightedEcmpMemberCount(
      const RouteNextHopEntry& fwd,
      const cfg::AsicType& asicType) const;

  template <typename AddrT>
  bool checkAndUpdateGenericEcmpResource(
      const std::shared_ptr<Route<AddrT>>& route,
      bool add);

  template <typename AddrT>
  bool checkAndUpdateArsEcmpResource(
      const std::shared_ptr<Route<AddrT>>& route,
      bool add);

  template <typename AddrT>
  bool checkAndUpdateEcmpResource(
      const std::shared_ptr<Route<AddrT>>& route,
      bool add);

  bool checkAndUpdateRouteResource(bool add);

  bool checkNeighborResource();

  bool l2StateChangedImpl(const StateDelta& delta);

  template <typename TableT>
  void neighborStateChangedImpl(const StateDelta& delta);

  template <typename TableT>
  bool checkNeighborResource(
      SwitchID switchId,
      uint32_t count,
      bool intermediateState);
  template <typename TableT>
  bool shouldCheckNeighborUpdate(SwitchID switchId);
  template <typename TableT>
  std::optional<uint32_t> getMaxAsicNeighborTableSize(
      SwitchID switchId,
      uint8_t resourcePercentage);
  std::optional<uint32_t> getMaxAsicUnifiedNeighborTableSize(
      const SwitchID& switchId,
      uint8_t resourcePercentage);
  template <typename TableT>
  uint32_t getMaxConfiguredNeighborTableSize();
  SwitchID getSwitchIdFromNeighborEntry(
      std::shared_ptr<SwitchState> newState,
      const auto& nbrEntry);
  template <typename TableT>
  std::unordered_map<SwitchID, uint32_t>& getNeighborEntriesMap();

  std::map<RouteNextHopEntry::NextHopSet, uint32_t> ecmpGroupRefMap_;
  std::map<RouteNextHopEntry::NextHopSet, uint32_t> arsEcmpGroupRefMap_;

  const HwAsicTable* asicTable_;
  const SwitchIdScopeResolver* scopeResolver_;

  bool nativeWeightedEcmp_{true};
  bool checkRouteUpdate_;
  uint32_t l2Entries_{0};
  uint32_t ecmpMemberUsage_{0};
  uint32_t routeUsage_{0};
  std::unordered_map<SwitchID, uint32_t> ndpEntriesMap_;
  std::unordered_map<SwitchID, uint32_t> arpEntriesMap_;

  FRIEND_TEST(ResourceAccountantTest, getMemberCountForEcmpGroup);
  FRIEND_TEST(ResourceAccountantTest, checkArsResource);
  FRIEND_TEST(ResourceAccountantTest, checkEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, checkAndUpdateEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, checkEcmpResourceForUcmpWeights);
  FRIEND_TEST(ResourceAccountantTest, checkAndUpdateGenericEcmpResource);
  FRIEND_TEST(
      ResourceAccountantTest,
      checkAndUpdateGenericEcmpResourceForUcmpWeights);
  FRIEND_TEST(ResourceAccountantTest, checkAndUpdateArsEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, computeWeightedEcmpMemberCount);
  FRIEND_TEST(ResourceAccountantTest, checkNeighborResource);
  FRIEND_TEST(ResourceAccountantTest, routeWithAdjustedWeightZero);
  FRIEND_TEST(ResourceAccountantTest, resolvedAndUnresolvedRoutes);
  FRIEND_TEST(MacTableManagerTest, MacLearnedBulkCb);
};
} // namespace facebook::fboss
