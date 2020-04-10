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

      if (!oldRoute) {
        processRouteAdded(stateDelta.newState(), rid, newRoute);
      } else if (!newRoute) {
        processRouteRemoved(oldRoute);
      } else {
        processRouteChanged(oldRoute, newRoute);
      }
    }
  }
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteAdded(
    const std::shared_ptr<SwitchState>& /*unused*/,
    RouterID /*unused*/,
    const std::shared_ptr<RouteT>& /*unused*/) {
  // TODO (skhare) add support
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteRemoved(
    const std::shared_ptr<RouteT>& /*unused*/) {
  // TODO (skhare) add support
}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteChanged(
    const std::shared_ptr<RouteT>& /*unused*/,
    const std::shared_ptr<RouteT>& /*unused*/) {
  // TODO (skhare) add support
}

void LookupClassRouteUpdater::updateClassID(
    const folly::IPAddress& /*unused*/,
    std::optional<cfg::AclLookupClass> /*unused*/) {
  // TODO (skhare) add support
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

} // namespace facebook::fboss
