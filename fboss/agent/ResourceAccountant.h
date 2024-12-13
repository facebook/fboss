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
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"

#include <gtest/gtest.h>

DECLARE_int32(max_l2_entries);
namespace facebook::fboss {

class ResourceAccountant {
 public:
  explicit ResourceAccountant(const HwAsicTable* asicTable);

  bool isValidUpdate(const StateDelta& delta);
  bool isValidRouteUpdate(const StateDelta& delta);
  void stateChanged(const StateDelta& delta);
  void enableDlbResourceCheck(bool enable);

 private:
  int getMemberCountForEcmpGroup(const RouteNextHopEntry& fwd) const;
  bool checkEcmpResource(bool intermediateState) const;
  bool checkDlbResource(uint32_t resourcePercentage) const;
  bool ecmpStateChangedImpl(const StateDelta& delta);
  bool shouldCheckRouteUpdate() const;
  bool isEcmp(const RouteNextHopEntry& fwd) const;
  int computeWeightedEcmpMemberCount(
      const RouteNextHopEntry& fwd,
      const cfg::AsicType& asicType) const;

  template <typename AddrT>
  bool checkAndUpdateEcmpResource(
      const std::shared_ptr<Route<AddrT>>& route,
      bool add);

  bool l2StateChangedImpl(const StateDelta& delta);

  uint32_t ecmpMemberUsage_{0};
  std::map<RouteNextHopEntry::NextHopSet, uint32_t> ecmpGroupRefMap_;

  const HwAsicTable* asicTable_;
  bool nativeWeightedEcmp_{true};
  bool checkRouteUpdate_;
  bool checkDlbResource_{true};
  int32_t l2Entries_{0};

  FRIEND_TEST(ResourceAccountantTest, getMemberCountForEcmpGroup);
  FRIEND_TEST(ResourceAccountantTest, checkDlbResource);
  FRIEND_TEST(ResourceAccountantTest, checkEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, checkAndUpdateEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, computeWeightedEcmpMemberCount);
  FRIEND_TEST(MacTableManagerTest, MacLearnedBulkCb);
};
} // namespace facebook::fboss
