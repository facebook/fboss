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

#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseDelta.h"

#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {
template <
    typename AddrT,
    typename ChangedFn,
    typename AddFn,
    typename RemoveFn,
    typename... Args>
void forEachChangedRoute(
    const StateDelta& stateDelta,
    ChangedFn changedFn,
    AddFn addedFn,
    RemoveFn removedFn,
    const Args&... args) {
  auto removeAll = [&](RouterID rid, const auto& routes) {
    for (const auto& iter : routes) {
      removedFn(args..., rid, iter.second);
    }
  };
  auto processRoutesDelta = [&](RouterID rid, const auto& routesDelta) {
    for (auto const& routeDelta : routesDelta) {
      auto const& oldRoute = routeDelta.getOld();
      auto const& newRoute = routeDelta.getNew();

      if (!oldRoute) {
        addedFn(args..., rid, newRoute);
      } else if (!newRoute) {
        removedFn(args..., rid, oldRoute);
      } else {
        changedFn(args..., rid, oldRoute, newRoute);
      }
    }
  };
  for (const auto& fibContainerDelta : stateDelta.getFibsDelta()) {
    auto const& newFibContainer = fibContainerDelta.getNew();
    if (!newFibContainer) {
      auto const& oldFibContainer = fibContainerDelta.getOld();
      removeAll(
          oldFibContainer->getID(),
          std::as_const(*oldFibContainer->template getFib<AddrT>()));
      continue;
    }
    processRoutesDelta(
        newFibContainer->getID(), fibContainerDelta.getFibDelta<AddrT>());
  }
}

template <
    typename ChangedFn,
    typename AddFn,
    typename RemoveFn,
    typename... Args>
void forEachChangedRoute(
    const StateDelta& delta,
    ChangedFn changedFn,
    AddFn addedFn,
    RemoveFn removedFn,
    const Args&... args) {
  forEachChangedRoute<folly::IPAddressV4>(
      delta, changedFn, addedFn, removedFn, args...);
  forEachChangedRoute<folly::IPAddressV6>(
      delta, changedFn, addedFn, removedFn, args...);
}
} // namespace facebook::fboss
