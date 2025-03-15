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

DECLARE_int32(max_l2_entries);
DECLARE_bool(intf_nbr_tables);

namespace facebook::fboss {

class ResourceAccountant {
 public:
  explicit ResourceAccountant(
      const HwAsicTable* asicTable,
      const SwitchIdScopeResolver* scopeResolver);

  bool isValidUpdate(const StateDelta& delta);
  bool isValidRouteUpdate(const StateDelta& delta);
  void stateChanged(const StateDelta& delta);
  void enableDlbResourceCheck(bool enable);

 private:
  int getMemberCountForEcmpGroup(const RouteNextHopEntry& fwd) const;
  bool checkEcmpResource(bool intermediateState) const;
  bool checkDlbResource(uint32_t resourcePercentage) const;
  bool routeAndEcmpStateChangedImpl(const StateDelta& delta);
  bool shouldCheckRouteUpdate() const;
  bool isEcmp(const RouteNextHopEntry& fwd) const;
  int computeWeightedEcmpMemberCount(
      const RouteNextHopEntry& fwd,
      const cfg::AsicType& asicType) const;

  template <typename AddrT>
  bool checkAndUpdateEcmpResource(
      const std::shared_ptr<Route<AddrT>>& route,
      bool add);

  bool checkAndUpdateRouteResource(bool add);

  bool l2StateChangedImpl(const StateDelta& delta);
  template <typename TableT>
  bool checkNeighborResource(
      SwitchID switchId,
      uint32_t count,
      bool intermediateState);
  template <typename TableT>
  bool shouldCheckNeighborUpdate(SwitchID switchId);
  template <typename TableT>
  bool neighborStateChangedImpl(const StateDelta& delta);
  std::optional<uint32_t> getMaxNdpTableSize(
      SwitchID switchId,
      uint8_t resourcePercentage);
  std::optional<uint32_t> getMaxArpTableSize(
      SwitchID switchId,
      uint8_t resourcePercentage);
  template <typename TableT>
  std::optional<uint32_t> getMaxNeighborTableSize(
      SwitchID switchId,
      uint8_t resourcePercentage);
  SwitchID getSwitchIdFromNeighborEntry(
      std::shared_ptr<SwitchState> newState,
      const auto& nbrEntry);
  template <typename TableT>
  std::unordered_map<SwitchID, uint32_t>& getNeighborEntriesMap();

  std::map<RouteNextHopEntry::NextHopSet, uint32_t> ecmpGroupRefMap_;

  const HwAsicTable* asicTable_;
  const SwitchIdScopeResolver* scopeResolver_;

  bool nativeWeightedEcmp_{true};
  bool checkRouteUpdate_;
  bool checkDlbResource_{true};
  uint32_t l2Entries_{0};
  uint32_t ecmpMemberUsage_{0};
  uint32_t routeUsage_{0};
  std::unordered_map<SwitchID, uint32_t> ndpEntriesMap_;
  std::unordered_map<SwitchID, uint32_t> arpEntriesMap_;

  FRIEND_TEST(ResourceAccountantTest, getMemberCountForEcmpGroup);
  FRIEND_TEST(ResourceAccountantTest, checkDlbResource);
  FRIEND_TEST(ResourceAccountantTest, checkEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, checkAndUpdateEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, computeWeightedEcmpMemberCount);
  FRIEND_TEST(MacTableManagerTest, MacLearnedBulkCb);
};
} // namespace facebook::fboss
