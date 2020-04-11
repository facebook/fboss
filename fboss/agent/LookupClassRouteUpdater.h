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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class LookupClassRouteUpdater {
 public:
  explicit LookupClassRouteUpdater(SwSwitch* sw) : sw_(sw) {}

  void processUpdates(const StateDelta& stateDelta);

  void initPort(
      const std::shared_ptr<SwitchState>& switchState,
      std::shared_ptr<Port> port);

  void neighborClassIDUpdated(
      const folly::IPAddress& ip,
      VlanID vlanID,
      std::optional<cfg::AclLookupClass> classID);

  void neighborClassIDUpdatedLocalCache(
      const folly::IPAddress& ip,
      VlanID vlanID,
      std::optional<cfg::AclLookupClass> classID);

  void updateStateObserverLocalCache(
      const std::shared_ptr<SwitchState>& switchState);

 private:
  template <typename AddrT>
  void processUpdatesHelper(const StateDelta& stateDelta);

  template <typename RouteT>
  void processRouteAdded(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute);
  template <typename RouteT>
  void processRouteRemoved(const std::shared_ptr<RouteT>& removedRoute);
  template <typename RouteT>
  void processRouteChanged(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);

  void updateClassID(
      const folly::IPAddress& nextHop,
      std::optional<cfg::AclLookupClass> classID);

  std::optional<cfg::AclLookupClass> getClassIDForIpAndVlan(
      const folly::IPAddress& ip,
      VlanID vlanID);
  void updateClassIDForIpAndVlan(
      const folly::IPAddress& ip,
      VlanID vlanID,
      std::optional<cfg::AclLookupClass> classID);

  bool belongsToSubnetInCache(const folly::IPAddress& ipToSearch);

  template <typename AddrT>
  void updateClassIDHelper(
      RouterID rid,
      std::shared_ptr<SwitchState>& newState,
      const std::shared_ptr<RouteTable> routeTable,
      const RoutePrefix<AddrT>& routePrefix,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);

  template <typename AddrT>
  void updateClassIDForPrefix(
      RouterID rid,
      const RoutePrefix<AddrT>& routePrefix,
      std::optional<cfg::AclLookupClass> classID);

  template <typename RouteT>
  void cacheNextHopToRoutesMapping(
      RouterID rid,
      const folly::IPAddress& nextHopIp,
      const std::shared_ptr<RouteT>& route);

  template <typename RouteT>
  std::optional<NextHop> getFirstNextHopOfRoute(
      const std::shared_ptr<RouteT>& route);

  template <typename AddrT>
  void updateStateObserverLocalCacheHelper(
      const std::shared_ptr<RouteTable> routeTable);

  /*
   * In theory, same IP may exist in different Vlans, thus maintain IP & Vlan
   * to classID mapping.
   * A route inherits classID of its nexthop. Maintaining IP to classID mapping
   * allows efficient lookup of nexthop's classID when a route is added.
   */
  boost::container::
      flat_map<std::pair<folly::IPAddress, VlanID>, cfg::AclLookupClass>
          ipAndVlan2ClassID_;

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
   */
  boost::container::flat_map<VlanID, std::set<folly::CIDRNetwork>>
      vlan2SubnetsCache_;

  /*
   * Routes inherit classID of its nexthop. When a classID is
   * associated/disassociated with a neighbor entry, this map provides an
   * efficient way to update all routes for that nexthop (neighbor IP) with the
   * same classID.
   */
  using RidAndRoutePrefix = std::pair<RouterID, RoutePrefix<folly::IPAddress>>;
  boost::container::flat_map<folly::IPAddress, std::vector<RidAndRoutePrefix>>
      nextHop2RidAndRoutePrefix_;

  SwSwitch* sw_;
};

} // namespace facebook::fboss
