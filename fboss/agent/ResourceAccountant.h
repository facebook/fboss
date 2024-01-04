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

namespace facebook::fboss {

class ResourceAccountant {
 public:
  explicit ResourceAccountant(const HwAsicTable* asicTable);

  bool isValidRouteUpdate(const StateDelta& delta);
  void stateChanged(const StateDelta& delta);

 private:
  int getMemberCountForEcmpGroup(const RouteNextHopEntry& fwd) const;
  bool checkEcmpResource(bool intermediateState) const;
  bool stateChangedImpl(const StateDelta& delta);

  template <typename AddrT>
  bool checkAndUpdateEcmpResource(
      const std::shared_ptr<Route<AddrT>>& route,
      bool add);

  uint32_t ecmpMemberUsage_{0};
  std::map<RouteNextHopEntry::NextHopSet, uint32_t> ecmpGroupRefMap_;

  const HwAsicTable* asicTable_;
  bool nativeWeightedEcmp_{true};

  FRIEND_TEST(ResourceAccountantTest, getMemberCountForEcmpGroup);
  FRIEND_TEST(ResourceAccountantTest, checkEcmpResource);
  FRIEND_TEST(ResourceAccountantTest, checkAndUpdateEcmpResource);
};
} // namespace facebook::fboss
