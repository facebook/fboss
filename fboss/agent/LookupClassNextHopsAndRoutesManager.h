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

#include "fboss/agent/LookupClassNextHopsAndRoutesManager.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

class LookupClassNextHopsAndRoutesManager {
 public:
  template <typename RouteT>
  std::optional<cfg::AclLookupClass> processRouteAdded(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute);

  template <typename RouteT>
  void processRouteRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& removedRoute);

  template <typename RouteT>
  std::optional<cfg::AclLookupClass> processRouteChanged(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);

  std::map<
      std::pair<RouterID, folly::CIDRNetwork>,
      std::optional<cfg::AclLookupClass>>
  neighborClassIDUpdated(
      const folly::IPAddress& ip,
      VlanID vlanID,
      std::optional<cfg::AclLookupClass> classID);

  void initPort(
      const std::shared_ptr<SwitchState>& switchState,
      std::shared_ptr<Port> port);

  void updateStateObserverLocalCache(
      const std::shared_ptr<RouteTable> routeTable);

 private:
  bool belongsToSubnetInCache(
      VlanID vlanID,
      const folly::IPAddress& ipToSearch);

  /*
   * We need to maintain nexthop to route mapping so that when a nexthop is
   * resolved (gets classID), the same classID could be associated with
   * corresponding routes. However, we are only interested in nexthops that are
   * part of 'certain' subnets. vlan2SubnetsCache_ maintains this list of
   * subnets.
   *   - ports needing queue-per-host have non-empty lookupClasses list.
   *   - each port belongs to an interface.
   *   - each interface is part of certain subnet.
   *   - nexthop IP that would use such a port for egress, would belong to the
   *     interface subnet.
   * Thus, we discover and maintain a list of subnets for ports that have
   * non-emptry lookupClasses list.
   *
   * Today, the list of subnets we need to cache is very small. Thus, std::set
   * is good enough. In future, if we need to cache a large number of subnets,
   * we could use Radix tree.
   */
  boost::container::flat_map<VlanID, std::set<folly::CIDRNetwork>>
      vlan2SubnetsCache_;
};

} // namespace facebook::fboss
