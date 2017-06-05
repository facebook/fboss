/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "NexthopToRouteCount.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/NodeMapDelta-defs.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Route.h"

using std::shared_ptr;
using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachRemoved;

namespace facebook { namespace fboss {

void NexthopToRouteCount::stateChanged(const StateDelta& delta) {
   for (auto const& rtDelta : delta.getRouteTablesDelta()) {
      // Do add/changed first so we don't remove next hops due to decrements
      // in ref count via removed routes, only to add them back again if these
      // next hops show up in added/changed routes
     if (rtDelta.getNew()) {
       RouterID id = rtDelta.getNew()->getID();
       forEachChanged(
           rtDelta.getRoutesV4Delta(),
           &NexthopToRouteCount::processChangedRoute<RouteV4>,
           &NexthopToRouteCount::processAddedRoute<RouteV4>,
           [&](NexthopToRouteCount *, RouterID, const shared_ptr<RouteV4>&) {},
           this,
           id);
       forEachChanged(
           rtDelta.getRoutesV6Delta(),
           &NexthopToRouteCount::processChangedRoute<RouteV6>,
           &NexthopToRouteCount::processAddedRoute<RouteV6>,
           [&](NexthopToRouteCount *, RouterID, const shared_ptr<RouteV6>&) {},
           this,
           id);
    }
    // Process removed routes
    if (rtDelta.getOld()) {
      RouterID id = rtDelta.getOld()->getID();
      forEachRemoved(
          rtDelta.getRoutesV4Delta(),
          &NexthopToRouteCount::processRemovedRoute<RouteV4>,
          this,
          id);
      forEachRemoved(
          rtDelta.getRoutesV6Delta(),
          &NexthopToRouteCount::processRemovedRoute<RouteV6>,
          this,
          id);
    }
  }
}

template<typename RouteT>
void NexthopToRouteCount::processChangedRoute(const RouterID rid,
   const shared_ptr<RouteT>& oldRoute, const shared_ptr<RouteT>& newRoute) {
  // We could compute set differences of nexthops from old and new routes,
  // but since we would need to compute 2 set differences (new - old and
  // old - new), it is similar complexity wise to just decrement reference
  // for old next hops and increment reference for new next hops.
  //  Do increment first so we don't need to delete and add nexthops
  //  with ref count 1, which remained unchanged.
  processAddedRoute(rid, newRoute);
  processRemovedRoute(rid, oldRoute);
}

template<typename RouteT>
void NexthopToRouteCount::processAddedRoute(const RouterID rid,
    const shared_ptr<RouteT>& newRoute) {
  const auto &fwd = newRoute->getForwardInfo();
  if (newRoute->isResolved()
      && fwd.getAction() == RouteForwardAction::NEXTHOPS) {
    for (const auto& nhop : fwd.getNextHopSet()) {
      incNexthopReference(rid, nhop);
    }
  }
}

template<typename RouteT>
void NexthopToRouteCount::processRemovedRoute(const RouterID rid,
   const shared_ptr<RouteT>& oldRoute) {
  const auto &fwd = oldRoute->getForwardInfo();
  if (oldRoute->isResolved()
      && fwd.getAction() == RouteForwardAction::NEXTHOPS) {
    for (const auto& nhop : fwd.getNextHopSet()) {
      decNexthopReference(rid, nhop);
    }
  }
}

void NexthopToRouteCount::incNexthopReference(RouterID rid,
    const Nexthop& nhop) {
  auto& nhop2RefCount = rid2nhopRefCounts_[rid];
  auto itr = nhop2RefCount.find(nhop);
  if (itr == nhop2RefCount.end()) {
    nhop2RefCount.emplace(nhop, 1);
  } else {
    DCHECK(itr->second >= 1);
    itr->second++;
  }
}

void NexthopToRouteCount::decNexthopReference(RouterID rid,
    const Nexthop& nhop) {
  auto& nhop2RefCount = rid2nhopRefCounts_[rid];
  auto itr = nhop2RefCount.find(nhop);
  CHECK(itr != nhop2RefCount.end());
  itr->second--;
  DCHECK(itr->second >= 0);
  if (itr->second == 0) {
    nhop2RefCount.erase(itr);
  }
  if (nhop2RefCount.empty()) {
    rid2nhopRefCounts_.erase(rid);
  }
}
}}
