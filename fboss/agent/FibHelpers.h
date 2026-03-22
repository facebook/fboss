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

#include "fboss/agent/state/FibDeltaHelpers.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseDelta.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"

#include <folly/IPAddress.h>

#include <memory>
#include <vector>

namespace facebook::fboss {

class SwitchState;
class RoutingInformationBase;

template <typename NeighborEntryT>
bool isNoHostRoute(const std::shared_ptr<NeighborEntryT>& entry);

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findRoute(
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state);

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findLongestMatchRoute(
    const RoutingInformationBase* rib,
    RouterID rid,
    const AddrT& addr,
    const std::shared_ptr<SwitchState>& state);

std::pair<uint64_t, uint64_t> getRouteCount(
    const std::shared_ptr<SwitchState>& state);

std::vector<NextHop> getNextHops(
    const std::shared_ptr<FibInfo>& fibInfo,
    NextHopSetId id);

std::vector<NextHop> getNextHops(
    const std::shared_ptr<SwitchState>& state,
    NextHopSetId id);

// Resolve nexthops from RouteNextHopEntry.
// When FLAGS_resolve_nexthops_from_id is on, resolves via resolvedNextHopSetID.
// When off, falls back to entry.getNextHopSet().
RouteNextHopSet getNextHops(
    const std::shared_ptr<SwitchState>& state,
    const RouteNextHopEntry& entry);

// Resolve non-override normalized nexthops from RouteNextHopEntry.
// When FLAGS_resolve_nexthops_from_id is on, resolves via
// normalizedResolvedNextHopSetID.
// When off, falls back to entry.nonOverrideNormalizedNextHops().
RouteNextHopSet getNonOverrideNormalizedNextHops(
    const std::shared_ptr<SwitchState>& state,
    const RouteNextHopEntry& entry);

/*
 * Build a new SwitchState that has newState's non-FIB data but
 * oldState's FIB routes (and ID maps when FLAGS_enable_nexthop_id_manager
 * is on). When oldState has no FibsInfoMap (warmboot/rollback),
 * creates empty FIBs matching newState's structure.
 */
std::shared_ptr<SwitchState> getNewStateWithOldFibInfo(
    const std::shared_ptr<SwitchState>& oldState,
    const std::shared_ptr<SwitchState>& newState);

template <typename Func>
void forAllRoutes(const std::shared_ptr<SwitchState>& state, Func func) {
  for (const auto& [_, fibInfo] : std::as_const(*state->getFibsInfoMap())) {
    auto fibsMap = fibInfo->getfibsMap();
    if (!fibsMap) {
      continue;
    }

    // Iterate through all FIB containers
    for (const auto& iter : std::as_const(*fibsMap)) {
      const auto& fibContainer = iter.second;
      auto rid = fibContainer->getID();
      for (const auto& route : std::as_const(*(fibContainer->getFibV6()))) {
        func(rid, route.second);
      }
      for (const auto& route : std::as_const(*(fibContainer->getFibV4()))) {
        func(rid, route.second);
      }
    }
  }
}

/*
 * Helper function to process FIBs delta in the same order that HwSwitch will
 */
template <
    typename ChangedFn,
    typename AddFn,
    typename RemoveFn,
    typename... Args>
void processFibsDeltaInHwSwitchOrder(
    const StateDelta& delta,
    ChangedFn changedFn,
    AddFn addedFn,
    RemoveFn removedFn,
    const Args&... args) {
  // First handle removed routes
  forEachChangedRoute(
      delta,
      [](RouterID /*rid*/,
         const auto&
         /*oldRoute*/,
         const auto& /*newRoute*/) {},
      [](RouterID /*id*/, const auto& /*addedRoute*/) {},
      [&](RouterID rid, const auto& deleted) { removedFn(rid, deleted); },
      args...);

  // Now handle added, changed routes
  forEachChangedRoute(
      delta,
      [&](RouterID rid, const auto& oldRoute, const auto& newRoute) {
        changedFn(rid, oldRoute, newRoute);
      },
      [&](RouterID rid, const auto& addedRoute) { addedFn(rid, addedRoute); },
      [](RouterID /*rid*/, const auto& /*deleted*/) {},
      args...);
}
} // namespace facebook::fboss
