/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/LookupClassNextHopsAndRoutesManager.h"

namespace facebook::fboss {

// Tell compiler which instantiations to make while it is compiling this
// template functions file, or else linker will not find right instantations
// and complain about undefined refernce.
template std::optional<cfg::AclLookupClass>
LookupClassNextHopsAndRoutesManager::processRouteAdded(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteV4>& addedRoute);
template std::optional<cfg::AclLookupClass>
LookupClassNextHopsAndRoutesManager::processRouteAdded(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteV6>& addedRoute);

template void LookupClassNextHopsAndRoutesManager::processRouteRemoved<RouteV4>(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteV4>& removedRoute);
template void LookupClassNextHopsAndRoutesManager::processRouteRemoved<RouteV6>(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteV6>& removedRoute);

template std::optional<cfg::AclLookupClass>
LookupClassNextHopsAndRoutesManager::processRouteChanged(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteV4>& oldRoute,
    const std::shared_ptr<RouteV4>& newRoute);
template std::optional<cfg::AclLookupClass>
LookupClassNextHopsAndRoutesManager::processRouteChanged(
    const std::shared_ptr<SwitchState>& switchState,
    RouterID rid,
    const std::shared_ptr<RouteV6>& oldRoute,
    const std::shared_ptr<RouteV6>& newRoute);

template <typename RouteT>
std::optional<cfg::AclLookupClass>
LookupClassNextHopsAndRoutesManager::processRouteAdded(
    const std::shared_ptr<SwitchState>& /* unsued */,
    RouterID /* unused */,
    const std::shared_ptr<RouteT>& /* unused */) {
  // TODO(skhare) update local data structures, get ClassID for this route, if
  // any
  return std::nullopt;
}

template <typename RouteT>
void LookupClassNextHopsAndRoutesManager::processRouteRemoved(
    const std::shared_ptr<SwitchState>& /* unsued */,
    RouterID /* unused */,
    const std::shared_ptr<RouteT>& /* unused */) {
  // TODO(skhare) remove from local data structures
}

template <typename RouteT>
std::optional<cfg::AclLookupClass>
LookupClassNextHopsAndRoutesManager::processRouteChanged(
    const std::shared_ptr<SwitchState>& /* unsued */,
    RouterID /* unused */,
    const std::shared_ptr<RouteT>& /* unused */,
    const std::shared_ptr<RouteT>& /* unused */) {
  // TODO(skhare) get ClassID if modified (if nextHops change, route classID may
  // change).
  return std::nullopt;
}

std::map<
    std::pair<RouterID, folly::CIDRNetwork>,
    std::optional<cfg::AclLookupClass>>
LookupClassNextHopsAndRoutesManager::neighborClassIDUpdated(
    const folly::IPAddress& /* unused */,
    VlanID /* unused */,
    std::optional<cfg::AclLookupClass> /* unused */) {
  std::map<
      std::pair<RouterID, folly::CIDRNetwork>,
      std::optional<cfg::AclLookupClass>>
      ridAndCidrToClassID;
  // TODO (skhare) get all routes whose classID changed
  return ridAndCidrToClassID;
}

void LookupClassNextHopsAndRoutesManager::initPort(
    const std::shared_ptr<SwitchState>& /* unused */,
    std::shared_ptr<Port> /* unused */) {
  // TODO(skhare) initialize vlan 2 subnet cache
}

void LookupClassNextHopsAndRoutesManager::updateStateObserverLocalCache(
    const std::shared_ptr<RouteTable> /* unused */) {
  // TODO(skhare) initialize local data structures for nexthops and routes on
  // warmboot
}

} // namespace facebook::fboss
