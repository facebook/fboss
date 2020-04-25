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

/*
 * TODO(skhare)
 *
 * Current prod queue-per-host implementation includes L2 and L3 host entry
 * portion of the fix. As we roll out, queue-per-host for L3 route entry, the
 * below flag provides us with a mechanism to disable L3 route entry portion of
 * the fix without having to disable L2 and L3 host entry queue-per-host fix.
 *
 * Set this flag to false to disable route entry portion of queue-per-host fix.
 *
 * Once the queue-per-host for L3 route entry is rolled out across the fleet
 * and is stable, remove this flag.
 */
DEFINE_bool(
    queue_per_host_route_fix,
    true,
    "Enable route entry (ip-per-task/VIP) portion of Queue-per-host fix");

namespace facebook::fboss {

void LookupClassRouteUpdater::processUpdates(const StateDelta& stateDelta) {
  if (!FLAGS_queue_per_host_route_fix) {
    return;
  }

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
        processRouteRemoved(
            stateDelta.newState(), oldRouteTable->getID(), oldRoute);
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
        processRouteRemoved(stateDelta.newState(), rid, oldRoute);
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

  auto classID = nextHopsAndRoutesManager_->processRouteAdded(
      switchState, rid, addedRoute);
  if (classID.has_value()) {
    updateClassIDForPrefix(rid, addedRoute->prefix(), classID);
  }
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteRemoved(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteT>& removedRoute) {
  CHECK(removedRoute);

  // ClassID is associated with (and refCnt'ed for) MAC and ARP/NDP neighbor.
  // Route simply inherits classID of its nexthop, so we need not release
  // classID here. Furthermore, the route is already removed, so we don't need
  // to schedule a state update either. Just remove the route from local data
  // structures.
  nextHopsAndRoutesManager_->processRouteRemoved(
      switchState, rid, removedRoute);
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteChanged(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteT>& oldRoute,
    const std::shared_ptr<RouteT>& newRoute) {
  CHECK(oldRoute);
  CHECK(newRoute);

  if (!oldRoute->isResolved() && !newRoute->isResolved()) {
    return;
  } else if (!oldRoute->isResolved() && newRoute->isResolved()) {
    processRouteAdded(switchState, rid, newRoute);
  } else if (oldRoute->isResolved() && !newRoute->isResolved()) {
    processRouteRemoved(switchState, rid, oldRoute);
  } else if (oldRoute->isResolved() && newRoute->isResolved()) {
    auto classID = nextHopsAndRoutesManager_->processRouteChanged(
        switchState, rid, oldRoute, newRoute);
    if (classID.has_value()) {
      updateClassIDForPrefix(rid, newRoute->prefix(), classID);
    }
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
    const std::map<
        std::pair<RouterID, folly::CIDRNetwork>,
        std::optional<cfg::AclLookupClass>>& ridAndCidrToClassID) {
  auto updateRouteClassIDFn =
      [this, ridAndCidrToClassID](const std::shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    auto newState{state};
    auto routeTables = newState->getRouteTables();

    for (const auto& [ridAndCIDRNetwork, classID] : ridAndCidrToClassID) {
      auto& [rid, cidrNetwork] = ridAndCIDRNetwork;
      auto& [ipAddress, mask] = cidrNetwork;
      auto routeTable = routeTables->getRouteTable(rid);

      if (ipAddress.isV6()) {
        RoutePrefix<folly::IPAddressV6> routePrefixV6{ipAddress.asV6(), mask};
        updateClassIDHelper(rid, newState, routeTable, routePrefixV6, classID);
      } else {
        RoutePrefix<folly::IPAddressV4> routePrefixV4{ipAddress.asV4(), mask};
        updateClassIDHelper(rid, newState, routeTable, routePrefixV4, classID);
      }
    }
    return newState;
  };

  sw_->updateState(
      "Update classID for routes", std::move(updateRouteClassIDFn));
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

void LookupClassRouteUpdater::neighborClassIDUpdated(
    const folly::IPAddress& ip,
    VlanID vlanID,
    std::optional<cfg::AclLookupClass> classID) {
  if (!FLAGS_queue_per_host_route_fix) {
    return;
  }

  auto ridAndCidrToClassID =
      nextHopsAndRoutesManager_->neighborClassIDUpdated(ip, vlanID, classID);
  updateClassID(ridAndCidrToClassID);
}

void LookupClassRouteUpdater::initPort(
    const std::shared_ptr<SwitchState>& switchState,
    std::shared_ptr<Port> port) {
  if (!FLAGS_queue_per_host_route_fix) {
    return;
  }

  nextHopsAndRoutesManager_->initPort(switchState, port);
}

void LookupClassRouteUpdater::updateStateObserverLocalCache(
    const std::shared_ptr<SwitchState>& switchState) {
  if (!FLAGS_queue_per_host_route_fix) {
    clearClassIDsForRoutes<folly::IPAddressV6>();
    clearClassIDsForRoutes<folly::IPAddressV4>();
    return;
  }

  // TODO(skhare) initialize local data structures for nexthops and routes
  // association (warmboot case)
}

template <typename AddrT>
void LookupClassRouteUpdater::clearClassIDsForRoutes() const {
  CHECK(!FLAGS_queue_per_host_route_fix);

  auto updateRouteClassIDFn = [](const std::shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    auto newState{state};

    for (const auto& routeTable : *newState->getRouteTables()) {
      auto rid = routeTable->getID();
      auto newRouteTable =
          routeTable->template getRib<AddrT>()->modify(rid, &newState);

      for (const auto& route : *newRouteTable->routes()) {
        if (route->getClassID().has_value()) {
          auto newRoute = route->clone();
          newRoute->updateClassID(std::nullopt);
          newRouteTable->updateRoute(newRoute);
        }
      }
    }

    return newState;
  };

  sw_->updateState(
      "Disable queue-per-host route fix, clear classID for every route ",
      std::move(updateRouteClassIDFn));
}

} // namespace facebook::fboss
