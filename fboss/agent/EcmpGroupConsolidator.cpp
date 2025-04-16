/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/EcmpGroupConsolidator.h"

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/StateDelta.h"

#include <gflags/gflags.h>
#include <limits>

DEFINE_bool(
    consolidate_ecmp_groups,
    false,
    "Consolidate ECMP groups when approaching HW limits");

namespace facebook::fboss {

std::shared_ptr<SwitchState> EcmpGroupConsolidator::consolidate(
    const StateDelta& delta) {
  if (!FLAGS_consolidate_ecmp_groups) {
    return delta.newState();
  }
  processRouteUpdates<folly::IPAddressV4>(delta);
  processRouteUpdates<folly::IPAddressV6>(delta);
  return delta.newState();
}

template <typename AddrT>
void EcmpGroupConsolidator::routeAdded(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& added) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(added->isResolved());
  auto nhopSet = added->getForwardInfo().getNextHopSet();
  if (nextHopGroup2Id_.find(nhopSet) == nextHopGroup2Id_.end()) {
    nextHopGroup2Id_.insert({std::move(nhopSet), findNextAvailableId()});
  }
}
template <typename AddrT>
void EcmpGroupConsolidator::routeDeleted(
    RouterID rid,
    const std::shared_ptr<Route<AddrT>>& removed) {
  CHECK_EQ(rid, RouterID(0));
  CHECK(removed->isResolved());
  // TODO we should remove only when the last reference to the
  // nhop set
  nextHopGroup2Id_.erase(removed->getForwardInfo().getNextHopSet());
}

template <typename AddrT>
void EcmpGroupConsolidator::processRouteUpdates(const StateDelta& delta) {
  forEachChangedRoute<AddrT>(
      delta,
      [this](RouterID rid, const auto& oldRoute, const auto& newRoute) {
        if (!oldRoute->isResolved() && !newRoute->isResolved()) {
          return;
        }
        if (oldRoute->isResolved() && !newRoute->isResolved()) {
          routeDeleted(rid, oldRoute);
          return;
        }
        if (!oldRoute->isResolved() && newRoute->isResolved()) {
          routeAdded(rid, newRoute);
          return;
        }
        // Both old and new are resolve
        CHECK(oldRoute->isResolved() && newRoute->isResolved());
        if (oldRoute->getForwardInfo().getNextHopSet() !=
            newRoute->getForwardInfo().getNextHopSet()) {
          routeDeleted(rid, oldRoute);
          routeAdded(rid, newRoute);
        }
      },
      [this](RouterID rid, const auto& newRoute) { routeAdded(rid, newRoute); },
      [this](RouterID rid, const auto& oldRoute) {
        routeDeleted(rid, oldRoute);
      });
}

EcmpGroupConsolidator::NextHopGroupId
EcmpGroupConsolidator::findNextAvailableId() const {
  std::unordered_set<NextHopGroupId> allocatedIds;
  for (const auto& [_, id] : nextHopGroup2Id_) {
    allocatedIds.insert(id);
  }
  for (auto start = kMinNextHopGroupId;
       start < std::numeric_limits<NextHopGroupId>::max();
       ++start) {
    if (allocatedIds.find(start) == allocatedIds.end()) {
      return start;
    }
  }
  throw FbossError("Unable to find id to allocate for new next hop group");
}
} // namespace facebook::fboss
