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

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/StateDelta.h"

#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>

namespace facebook::fboss {

class LookupClassRouteUpdater : public StateObserver {
 public:
  explicit LookupClassRouteUpdater(SwSwitch* sw) : sw_(sw) {
    sw_->registerStateObserver(this, "LookupClassRouteUpdater");
  }
  ~LookupClassRouteUpdater() override {
    sw_->unregisterStateObserver(this);
  }

  void stateUpdated(const StateDelta& stateDelta) override;

  // Method used by unit tests
  const boost::container::
      flat_map<VlanID, folly::F14FastSet<folly::CIDRNetwork>>&
      getVlan2SubnetCache() const {
    return vlan2SubnetsCache_;
  }
  int getNumPrefixesWithMultiNextHops() const {
    return prefixesWithMultiNextHops_.size();
  }

 private:
  // Helper methods
  void reAddAllRoutes(const StateDelta& stateDelta);

  bool vlanHasOtherPortsWithClassIDs(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<Vlan>& vlan,
      const std::shared_ptr<Port>& removedPort);

  void removeNextHopsForSubnet(
      const StateDelta& stateDelta,
      const folly::CIDRNetwork& subnet,
      const std::shared_ptr<Vlan>& vlan);

  // Methods for dealing with vlan2SubnetsCache_
  bool belongsToSubnetInCache(
      VlanID vlanID,
      const folly::IPAddress& ipToSearch);

  void updateSubnetsCache(
      const StateDelta& stateDelta,
      std::shared_ptr<Port> port,
      bool reAddAllRoutesEnabled);

  std::optional<cfg::AclLookupClass> getClassIDForLinkLocal(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlanID,
      const folly::IPAddressV6& ipAddressV6);

  std::optional<cfg::AclLookupClass> getClassIDForNeighbor(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlanID,
      const folly::IPAddress& ipAddress);

  // Methods for handling port updates

  bool isNeighborReachable(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlanID,
      const folly::IPAddressV6& neighborIP);

  void processPortAdded(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& addedPort,
      bool reAddAllRoutesEnabled);
  void processPortRemovedForVlan(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& removedPort,
      VlanID vlanID);
  void processPortRemoved(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& port);
  void processPortChanged(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  void processPortUpdates(const StateDelta& stateDelta);

  // Methods for handling interface updates
  void processInterfaceAdded(
      const StateDelta& stateDelta,
      const std::shared_ptr<Interface>& addedInterface);
  void processInterfaceRemoved(
      const StateDelta& stateDelta,
      const std::shared_ptr<Interface>& removedInterface);
  void processInterfaceChanged(
      const StateDelta& stateDelta,
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface);

  void processInterfaceUpdates(const StateDelta& stateDelta);

  // Methods for handling neighbor updates
  template <typename AddedNeighborT>
  void processNeighborAdded(
      const StateDelta& stateDelta,
      VlanID vlanID,
      const std::shared_ptr<AddedNeighborT>& addedNeighbor);
  template <typename removedNeighborT>
  void processNeighborRemoved(
      const StateDelta& stateDelta,
      VlanID vlanID,
      const std::shared_ptr<removedNeighborT>& removedNeighbor);
  template <typename ChangedNeighborT>
  void processNeighborChanged(
      const StateDelta& stateDelta,
      VlanID vlanID,
      const std::shared_ptr<ChangedNeighborT>& oldNeighbor,
      const std::shared_ptr<ChangedNeighborT>& newNeighbor);

  template <typename AddrT, typename MapDeltaT>
  void processNeighborUpdates(
      const StateDelta& stateDelta,
      const MapDeltaT& mapDelta);

  // Methods for handling route updates
  template <typename RouteT>
  std::optional<cfg::AclLookupClass> addRouteAndFindClassID(
      const StateDelta& stateDelta,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute,
      std::optional<std::pair<folly::IPAddress, VlanID>> nextHopAndVlanToOmit);

  template <typename RouteT>
  void processRouteAdded(
      const StateDelta& stateDelta,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute);
  template <typename RouteT>
  void processRouteRemoved(
      const StateDelta& stateDelta,
      RouterID rid,
      const std::shared_ptr<RouteT>& removedRoute);
  template <typename RouteT>
  void processRouteChanged(
      const StateDelta& stateDelta,
      RouterID rid,
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);

  template <typename AddrT>
  void processRouteUpdates(const StateDelta& stateDelta);

  using RidAndCidr = std::pair<RouterID, folly::CIDRNetwork>;
  using NextHopAndVlan = std::pair<folly::IPAddress, VlanID>;
  using WithAndWithoutClassIDPrefixes =
      std::pair<std::set<RidAndCidr>, std::set<RidAndCidr>>;

  using RouteAndClassID =
      std::pair<RidAndCidr, std::optional<cfg::AclLookupClass>>;

  template <typename RouteT>
  bool addRouteToMultiNextHopMap(
      const std::shared_ptr<SwitchState>& newState,
      const std::shared_ptr<RouteT>& route,
      std::optional<std::pair<folly::IPAddress, VlanID>> nextHopAndVlanToOmit,
      const RidAndCidr& ridAndCidr);

  // Methods for scheduling state updates
  void updateClassIDsForRoutes(
      const std::vector<RouteAndClassID>& routesAndClassIDs) const;

  // Methods for blocked neighbor processing
  bool isSubnetCachedByLookupClasses(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlanID,
      const folly::CIDRNetwork& addressToSearch) const;

  bool isSubnetCachedByBlockedNeighborIP(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlanID,
      const folly::CIDRNetwork& addressToSearch) const;

  std::optional<folly::CIDRNetwork> getInterfaceSubnetForIPIf(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlanID,
      const folly::IPAddress& ipAddress) const;

  void processBlockNeighborAdded(
      const StateDelta& stateDelta,
      std::vector<std::pair<VlanID, folly::IPAddress>> toBeAddedBlockNeighbors);
  void processBlockNeighborRemoved(
      const StateDelta& stateDelta,
      std::vector<std::pair<VlanID, folly::IPAddress>>
          toBeRemovedBlockNeighbors);
  void processBlockNeighborUpdates(const StateDelta& stateDelta);

  void updatePrefixesWithMultiNextHops(
      const std::set<folly::IPAddress>& neighborsWithClassId,
      const RidAndCidr& ridAndCidr);
  void removePrefixesWithMultiNextHops(
      const std::set<RidAndCidr>& withoutClassIDPrefixes,
      const folly::IPAddress& removedNeighborIP);
  void addPrefixesWithMultiNextHops(
      const std::set<RidAndCidr>& withoutClassIDPrefixes,
      const folly::IPAddress& addedNeighborIP,
      const std::shared_ptr<SwitchState>& newState,
      VlanID vlanID);

  // Methods for blocked MACs processing
  template <typename AddrT>
  bool addBlockedNeighborIPtoSubnetCache(
      VlanID vlanID,
      const folly::MacAddress& blockedNeighborMac,
      const std::shared_ptr<SwitchState>& newState);
  template <typename AddrT>
  void removeBlockedNeighborIPfromSubnetCache(
      VlanID vlanID,
      const folly::MacAddress& blockedNeighborMac,
      const StateDelta& stateDelta);
  void processMacAddrsToBlockAdded(
      const StateDelta& stateDelta,
      const std::vector<std::pair<VlanID, folly::MacAddress>>&
          toBeAddedMacAddrsToBlock);
  void processMacAddrsToBlockRemoved(
      const StateDelta& stateDelta,
      const std::vector<std::pair<VlanID, folly::MacAddress>>&
          toBeRemovedMacAddrsToBlock);
  void processMacAddrsToBlockUpdates(const StateDelta& stateDelta);

  /*
   * We need to maintain nexthop to route mapping so that when a nexthop is
   * resolved (gets classID), the same classID could be associated with
   * corresponding routes. However, we are only interested in nexthops that are
   * part of 'certain' subnets. vlan2SubnetsCache_ maintains that list of
   * subnets.
   *   - ports needing queue-per-host have non-empty lookupClasses list.
   *   - each port belongs to an interface.
   *   - each interface is part of certain subnet.
   *   - nexthop IP that would use such a port for egress, would belong to the
   *     interface subnet.
   * Thus, we discover and maintain a list of subnets for ports that have
   * non-empty lookupClasses list.
   *
   * Today, the list of subnets we need to cache is very small. Thus,
   * std::set is good enough. In future, if we need to cache a large
   * number of subnets, we could use a Radix tree.
   */
  boost::container::flat_map<VlanID, folly::F14FastSet<folly::CIDRNetwork>>
      vlan2SubnetsCache_;

  /*
   * Route inherits classID of one of its reachable next hops.
   *
   * A route entry points to an egress object, which corresponds to a next hop.
   * If a route has multiple next hops:
   *  - route entry points to an ECMP egress object.
   *  - ECMP egress object points to a list of egress objects.
   *  - Each egress object in the list corresponds to one next hop.
   *
   * However, a classID could only be associated with a route entry, and not
   * with an egress object (hardware limitation). Thus, if a route has multiple
   * next hops, we must pick one of the next hops and inherit its classID for
   * the route.
   *
   * Only reachable next hops have classID. Thus, a Route inherits classID of
   * one of its *reachable* next hops. In the current implementation, we pick
   * any one reachable next hop and inherit its classID, in future this could
   * be modified to some other scheme.
   *
   * There are several cases to consider:
   * (1) When a route is resolved:
   *     (1.1) None of its next hops are reachable: No classID for route.
   *     (1.2) At least one next hop is reachable: Inherit its classID.
   * (2) When a route is deleted, clear local data structures.
   * (3) When route's nexthopset changes:
   *         Remove oldRoute and add newRoute.
   * (4) When a next hop's classID is updated:
   *     (4.1) next hop gets classID (neighbor becomes reachable):
   *           For every route that has this next hop, if the route does not
   *           already have a classID, route inherits this classID.
   *     (4.2) next hop loses classID (neighbor becomes unreachable):
   *           For every route that has this next hop, find another next hop
   *           that already has classID, route inherits that classID. If such a
   *           next hop does not exist, route loses classID.
   *
   * Warmboot does not require special handling.
   *  - On warmboot, LookupClassRouteUPdater would observe:
   *    StateDelta(emptyState, warmbootState).
   *  - The resulting neighbor/route/port processing would reinitialize
   *    LookupClassRouteUpdater's local data structures and trigger state
   *    update.
   *  - However, computed route classIDs would be same as programmed, and thus
   *    HwSwitch delta processing would treat this as no-op (no ASIC
   *    programming). Thus, warmboot requires no special handling.
   *  - In future, across warmboot, if we decide to change the scheme (pick
   *    something other than first reachable nexthop), the computed route
   *    classID would not be same as what is programmed on the ASIC. HwSwitch
   *    delta processing would then program the ASIC to associate this new
   *    classID which is the desired behavior anyway.
   *
   *  Thus, we don't require any special handling for warmboot.
   *
   * This is implemented by maintaining following data structures.
   * TODO(skhare) add note on how these data structures handle all cases.
   */

  /*
   * NextHop to prefixes map.
   *
   * In theory, same IP may exist in different Vlans, thus maintain IP + Vlan
   * to prefixes mapping.
   *
   * Today, the list of subnets we need to cache is very small. Thus,
   * std::set is good enough. In future, if we need to cache a large
   * number of subnets, we could use Radix tree.
   *
   * This maintains the list of prefixes that inherit classID from this
   * [nexthop, vlan] separately from the list of prefixes that don't.
   */
  folly::F14FastMap<NextHopAndVlan, WithAndWithoutClassIDPrefixes>
      nextHopAndVlan2Prefixes_;

  /*
   * Set of prefixes with classID (from any [nexthop + vlan]).
   */
  std::set<RidAndCidr> allPrefixesWithClassID_;

  /*
   * Map of prefixes with multiple nexthops associated to class IDs.
   */
  folly::F14FastMap<RidAndCidr, std::set<folly::IPAddress>>
      prefixesWithMultiNextHops_;

  // pending routes with classID to be updated
  std::vector<RouteAndClassID> toUpdateRoutesAndClassIDs_;

  SwSwitch* sw_;

  bool inited_{false};
};

} // namespace facebook::fboss
