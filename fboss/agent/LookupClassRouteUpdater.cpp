/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/LookupClassRouteUpdater.h"

#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

void LookupClassRouteUpdater::processUpdates(const StateDelta& stateDelta) {
  processUpdatesHelper<folly::IPAddressV6>(stateDelta);
  processUpdatesHelper<folly::IPAddressV4>(stateDelta);
}

template <typename AddrT>
void LookupClassRouteUpdater::processUpdatesHelper(
    const StateDelta& stateDelta) {
  for (auto const& routeTableDelta : stateDelta.getRouteTablesDelta()) {
    auto const& newRouteTable = routeTableDelta.getNew();
    if (!newRouteTable) {
      auto const& oldRouteTable = routeTableDelta.getOld();
      for (const auto& oldRoute :
           *(oldRouteTable->template getRib<AddrT>()->routes())) {
        processRouteRemoved(oldRoute);
      }
      continue;
    }

    auto rid = newRouteTable->getID();
    for (auto const& routeDelta : routeTableDelta.getRoutesDelta<AddrT>()) {
      auto const& oldRoute = routeDelta.getOld();
      auto const& newRoute = routeDelta.getNew();

      /*
       * If newRoute already has classID populated, don't process.
       * This can happen in two cases:
       *  o Warmboot: prior to warmboot, route entries may have a classID
       *    associated with them. Routes inherit the classID of their nexthop.
       *    The nexthop (neighbor entry) classID is consumed by
       *    updateStateObserverLocalCache() to populate its local cache.
       *  o Once LookupClassUpdater chooses classID for a route, it
       *    schedules a state update. After the state update is run, all state
       *    observers are notified. At that time, LookupClassUpdater will
       *    receive a stateDelta that contains classID assigned to new route
       *    entry. Since this will be side-effect of state update that
       *    LookupClassUpdater triggered, there is nothing to do here.
       */
      if (newRoute && newRoute->getClassID().has_value()) {
        continue;
      }

      if (!oldRoute) {
        processRouteAdded(stateDelta.newState(), rid, newRoute);
      } else if (!newRoute) {
        processRouteRemoved(oldRoute);
      } else {
        processRouteChanged(stateDelta.newState(), rid, oldRoute, newRoute);
      }
    }
  }
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteAdded(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteT>& addedRoute) {
  CHECK(addedRoute);

  // Non-resolved routes are not programmed in HW.
  if (!addedRoute->isResolved()) {
    return;
  }

  auto firstNextHop = getFirstNextHopOfRoute(addedRoute);
  if (firstNextHop.has_value() &&
      belongsToSubnetInCache(firstNextHop.value().addr())) {
    auto firstNextHopIp = firstNextHop.value().addr();
    auto vlanID = switchState->getInterfaces()
                      ->getInterfaceIf(firstNextHop->intf())
                      ->getVlanID();
    cacheNextHopToRoutesMapping(rid, firstNextHopIp, addedRoute);
    auto classID = getClassIDForIpAndVlan(firstNextHopIp, vlanID);
    if (classID.has_value()) {
      updateClassIDForPrefix(rid, addedRoute->prefix(), classID);
    }
  }
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteRemoved(
    const std::shared_ptr<RouteT>& removedRoute) {
  CHECK(removedRoute);

  auto firstNextHop = getFirstNextHopOfRoute(removedRoute);
  // ClassID is associated with (and refCnt'ed for) MAC and ARP/NDP neighbor.
  // Route simply inherits classID of its nexthop, so we need not release
  // classID here. Furthermore, the route is already removed, so we don't need
  // to schedule a state update either. Just remove the route from local
  // nextHop to route cache.
  if (firstNextHop.has_value()) {
    nextHop2RidAndRoutePrefix_.erase(firstNextHop.value().addr());
  }
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteChanged(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteT>& oldRoute,
    const std::shared_ptr<RouteT>& newRoute) {
  CHECK(oldRoute);
  CHECK(newRoute);

  if (!oldRoute->isResolved() && newRoute->isResolved()) {
    processRouteAdded(switchState, rid, newRoute);
  }

  if (oldRoute->isResolved() && !newRoute->isResolved()) {
    processRouteRemoved(oldRoute);
  }

  if ((oldRoute->isResolved() && newRoute->isResolved()) &&
      (getFirstNextHopOfRoute(oldRoute) != getFirstNextHopOfRoute(newRoute))) {
    processRouteRemoved(oldRoute);
    processRouteAdded(switchState, rid, newRoute);
  }
}

template <typename AddrT>
void LookupClassRouteUpdater::updateClassIDHelper(
    RouterID rid,
    std::shared_ptr<SwitchState>& newState,
    const std::shared_ptr<RouteTable> routeTable,
    const RoutePrefix<AddrT>& routePrefix,
    std::optional<cfg::AclLookupClass> classID) {
  auto routeTableRib =
      routeTable->template getRib<AddrT>()->modify(rid, &newState);

  auto route = routeTableRib->routes()->getRouteIf(routePrefix);
  if (route) {
    route = route->clone();
    route->updateClassID(classID);
    routeTableRib->updateRoute(route);
  }
}

void LookupClassRouteUpdater::updateClassID(
    const folly::IPAddress& nextHop,
    std::optional<cfg::AclLookupClass> classID) {
  auto updateRouteClassIDFn =
      [this, nextHop, classID](const std::shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    auto newState{state};
    /*
     * Route inherits classID of its nexthop. Thus, for every route whose
     * nexthop is this ip, update the route's classID.
     */
    auto iter = nextHop2RidAndRoutePrefix_.find(nextHop);
    if (iter != nextHop2RidAndRoutePrefix_.end()) {
      auto routeTables = newState->getRouteTables();
      auto ridAndRoutePrefixes = iter->second;

      for (const auto& [rid, routePrefix] : ridAndRoutePrefixes) {
        auto routeTable = routeTables->getRouteTable(rid);
        if (routePrefix.network.isV6()) {
          RoutePrefix<folly::IPAddressV6> routePrefixV6{
              routePrefix.network.asV6(), routePrefix.mask};

          updateClassIDHelper(
              rid, newState, routeTable, routePrefixV6, classID);
        } else {
          RoutePrefix<folly::IPAddressV4> routePrefixV4{
              routePrefix.network.asV4(), routePrefix.mask};

          updateClassIDHelper(
              rid, newState, routeTable, routePrefixV4, classID);
        }
      }
    }
    return newState;
  };

  auto classIDStr = classID.has_value()
      ? folly::to<std::string>(static_cast<int>(classID.value()))
      : "None";
  sw_->updateState(
      folly::to<std::string>("Update classID for routes: ", classIDStr),
      std::move(updateRouteClassIDFn));
}

template <typename AddrT>
void LookupClassRouteUpdater::updateClassIDForPrefix(
    RouterID rid,
    const RoutePrefix<AddrT>& routePrefix,
    std::optional<cfg::AclLookupClass> classID) {
  auto updateRouteClassIDFn = [this, rid, routePrefix, classID](
                                  const std::shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    auto newState{state};
    auto routeTable = newState->getRouteTables()->getRouteTable(rid);
    updateClassIDHelper(rid, newState, routeTable, routePrefix, classID);
    return newState;
  };

  auto classIDStr = classID.has_value()
      ? folly::to<std::string>(static_cast<int>(classID.value()))
      : "None";
  sw_->updateState(
      folly::to<std::string>("Update classID for route prefix: ", classIDStr),
      std::move(updateRouteClassIDFn));
}

std::optional<cfg::AclLookupClass>
LookupClassRouteUpdater::getClassIDForIpAndVlan(
    const folly::IPAddress& ip,
    VlanID vlanID) {
  auto iter = ipAndVlan2ClassID_.find(std::make_pair(ip, vlanID));
  if (iter != ipAndVlan2ClassID_.end()) {
    return iter->second;
  }

  return std::nullopt;
}

void LookupClassRouteUpdater::updateClassIDForIpAndVlan(
    const folly::IPAddress& ip,
    VlanID vlanID,
    std::optional<cfg::AclLookupClass> classID) {
  auto iter = ipAndVlan2ClassID_.find(std::make_pair(ip, vlanID));

  if (classID.has_value()) {
    // If a neighbor goes from classID x to classID y, the entry would exist,
    // overwrite it.
    ipAndVlan2ClassID_[std::make_pair(ip, vlanID)] = classID.value();
  } else {
    CHECK(iter != ipAndVlan2ClassID_.end());
    ipAndVlan2ClassID_.erase(iter);
  }
}

void LookupClassRouteUpdater::neighborClassIDUpdated(
    const folly::IPAddress& ip,
    VlanID vlanID,
    std::optional<cfg::AclLookupClass> classID) {
  updateClassIDForIpAndVlan(ip, vlanID, classID);
  updateClassID(ip, classID);
}

bool LookupClassRouteUpdater::belongsToSubnetInCache(
    const folly::IPAddress& ipToSearch) {
  /*
   * When a neighbor is resolved, given the egress port, we could tell which
   * vlan(s) it is part of. However, the neighbor may not be resolved
   * yet, thus search if the input ip exists in any of the subnets.
   *
   * In practice, today, there is only one vlan for downlink ports.
   * Even if that changes in future, we should be OK:
   * This function would be used to decide whether or not to store nexthop to
   * route mapping for a given nexthop. At worst, the caller would end up
   * storing nexthop to route mapping for more routes than necessary, but the
   * functionality would still be correct.
   */
  for (const auto& [vlan, subnetsCache] : vlan2SubnetsCache_) {
    for (const auto& [ipAddress, mask] : subnetsCache) {
      if (ipToSearch.inSubnet(ipAddress, mask)) {
        return true;
      }
    }
  }

  return false;
}

void LookupClassRouteUpdater::initPort(
    const std::shared_ptr<SwitchState>& switchState,
    std::shared_ptr<Port> port) {
  for (const auto& [vlanID, vlanInfo] : port->getVlans()) {
    auto vlan = switchState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    auto& subnetsCache = vlan2SubnetsCache_[vlanID];
    auto interface =
        switchState->getInterfaces()->getInterfaceIf(vlan->getInterfaceID());
    if (interface) {
      for (auto address : interface->getAddresses()) {
        subnetsCache.insert(address);
      }
    }
  }
}

template <typename RouteT>
void LookupClassRouteUpdater::cacheNextHopToRoutesMapping(
    RouterID rid,
    const folly::IPAddress& nextHopIp,
    const std::shared_ptr<RouteT>& route) {
  auto& ridAndRoutePrefixes = nextHop2RidAndRoutePrefix_[nextHopIp];

  folly::IPAddress addr{route->prefix().network};
  auto mask = route->prefix().mask;
  RoutePrefix<folly::IPAddress> prefix{addr, mask};
  ridAndRoutePrefixes.push_back(std::make_pair(rid, prefix));
}

/*
 * On Broadcom:
 *
 * If a route has multiple next hops, they are grouped together in an ECMP
 * group. Such a route entry will point to an ECMP egress object. The members
 * of this ECMP egress object are egress objects for the next hops.
 *
 * A classID could be associated with a route entry, and thus, for a route
 * with multiple next hops, we must pick one of the nexthop and inherit its
 * classID for a given route. Current implementation chooses to inherit from
 * first nexthop. If this becomes a problem in practice, this could be
 * modified to pick some other scheme e.g. pick a random nth next hop instead
 * of always picking the first.
 *
 * This is a hardware limitation.
 */
template <typename RouteT>
std::optional<NextHop> LookupClassRouteUpdater::getFirstNextHopOfRoute(
    const std::shared_ptr<RouteT>& route) {
  auto fwdInfo = route->getForwardInfo();
  auto nextHopIter = fwdInfo.getNextHopSet().begin();
  if (nextHopIter != fwdInfo.getNextHopSet().end()) {
    return *nextHopIter;
  }
  return std::nullopt;
}

} // namespace facebook::fboss
