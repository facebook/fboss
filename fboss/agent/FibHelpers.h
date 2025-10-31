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

#include "fboss/agent/state/StateDelta.h"

#include <folly/IPAddress.h>

#include <memory>

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

template <typename Func>
void forAllRoutes(const std::shared_ptr<SwitchState>& state, Func func) {
  for (const auto& [_, fibs] : std::as_const(*state->getFibs())) {
    for (const auto& iter : std::as_const(*fibs)) {
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
