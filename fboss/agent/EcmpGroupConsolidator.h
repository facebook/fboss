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

#include <memory>

DECLARE_bool(consolidate_ecmp_groups);

namespace facebook::fboss {
class StateDelta;
class SwitchState;

class EcmpGroupConsolidator {
 public:
  using NextHopGroupId = uint32_t;
  std::shared_ptr<SwitchState> consolidate(const StateDelta& delta);
  const auto& getNhopsToId() const {
    return nextHopGroup2Id_;
  }

 private:
  template <typename AddrT>
  void processRouteUpdates(const StateDelta& delta);
  template <typename AddrT>
  void routeAdded(RouterID rid, const std::shared_ptr<Route<AddrT>>& added);
  template <typename AddrT>
  void routeDeleted(RouterID rid, const std::shared_ptr<Route<AddrT>>& removed);
  static uint32_t constexpr kMinNextHopGroupId = 1;
  NextHopGroupId findNextAvailableId() const;
  std::map<RouteNextHopSet, NextHopGroupId> nextHopGroup2Id_;
};
} // namespace facebook::fboss
