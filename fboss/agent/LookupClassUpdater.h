// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class LookupClassUpdater : public AutoRegisterStateObserver {
 public:
  explicit LookupClassUpdater(SwSwitch* sw)
      : AutoRegisterStateObserver(sw, "LookupClassUpdater"), sw_(sw) {}
  ~LookupClassUpdater() override {}

  void stateUpdated(const StateDelta& stateDelta) override;

  int getRefCnt(
      PortID portID,
      const folly::MacAddress mac,
      VlanID vlanID,
      cfg::AclLookupClass classID);

 private:
  using ClassID2Count = boost::container::flat_map<cfg::AclLookupClass, int>;

  template <typename AddrT>
  void processNeighborUpdates(const StateDelta& stateDelta);

  template <typename AddedEntryT>
  void processNeighborAdded(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<AddedEntryT>& addedEntry);
  template <typename RemovedEntryT>
  void processNeighborRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<RemovedEntryT>& removedEntry);
  template <typename ChangedEntryT>
  void processNeighborChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<ChangedEntryT>& oldEntry,
      const std::shared_ptr<ChangedEntryT>& newEntry);

  bool isInited(PortID portID);
  void initPort(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<Port>& port);

  template <typename NewEntryT>
  void updateNeighborClassID(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<NewEntryT>& newEntry);

  template <typename RemovedEntryT>
  void removeClassIDForPortAndMac(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<RemovedEntryT>& removedEntry);

  cfg::AclLookupClass getClassIDwithMinimumNeighbors(
      ClassID2Count classID2Count) const;

  template <typename RemovedEntryT>
  void removeNeighborFromLocalCacheForEntry(
      const std::shared_ptr<RemovedEntryT>& removedEntry,
      VlanID vlanID);

  template <typename NewEntryT>
  void updateStateObserverLocalCacheForEntry(
      const std::shared_ptr<NewEntryT>& newEntry,
      VlanID vlanID);

  void updateStateObserverLocalCache(
      const std::shared_ptr<SwitchState>& switchState);

  template <typename AddrT>
  void updateStateObserverLocalCacheHelper(
      const std::shared_ptr<Vlan>& vlan,
      const std::shared_ptr<Port>& port);

  template <typename AddrT>
  void clearClassIdsForResolvedNeighbors(
      const std::shared_ptr<SwitchState>& switchState,
      PortID portID);
  template <typename AddrT>
  void repopulateClassIdsForResolvedNeighbors(
      const std::shared_ptr<SwitchState>& switchState,
      PortID portID);

  void processPortUpdates(const StateDelta& stateDelta);

  void processPortAdded(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<Port>& addedPort);
  void processPortRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<Port>& port);
  void processPortChanged(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  template <typename AddrT>
  void validateRemovedPortEntries(
      const std::shared_ptr<Vlan>& vlan,
      PortID portID);

  /*
   * Methods to iterate over MacTable, ArpTable or NdpTable or table deltas.
   */
  template <typename AddrT>
  auto getTable(const std::shared_ptr<Vlan>& vlan);

  template <typename AddrT>
  auto getTableDelta(const VlanDelta& vlanDelta);

  template <typename AddrT>
  void processRouteUpdates(const StateDelta& stateDelta);

  template <typename RouteT>
  void processRouteAdded(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute);
  template <typename RouteT>
  void processRouteRemoved(const std::shared_ptr<RouteT>& removedRoute);
  template <typename RouteT>
  void processRouteChanged(
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);
  void updateRouteClassID(
      const folly::IPAddress& ip,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);

  void updateSubnetsToCache(
      const std::shared_ptr<SwitchState>& switchState,
      std::shared_ptr<Port> port);
  bool belongsToSubnetInCache(const folly::IPAddress& ipToSearch);

  std::optional<cfg::AclLookupClass> getClassIDForIpAndVlan(
      const folly::IPAddress& ip,
      VlanID vlanID);
  void updateIpAndVlan2ClassID(
      const folly::IPAddress& ip,
      VlanID vlanID,
      std::optional<cfg::AclLookupClass> classID);

  SwSwitch* sw_;

  /*
   * Maintains the number of times a classID is used. When a new MAC + vlan
   * requires classID assignment, port2ClassIDAndCount_ is used to determine
   * the least used classID. This is maintained in a map for every port.
   */
  boost::container::flat_map<PortID, ClassID2Count> port2ClassIDAndCount_;

  /*
   * Maintains:
   *  - classID assigned to a (MAC address + vlan).
   *  - refCnt to count the number of times a classID was requested for a
   *    (MAC address + vlan).
   *
   *    For ARP/NDP neighbors:
   *
   *    Multiple neighbor IPs could have same MAC + vlan (e.g. link-local and
   *    global IP etc.). First neighbor with previously unseen MAC + vlan
   *    would cause classID assignment, while subsequent neighbors with same
   *    MAC + vlan would get the same classID but bump up the refCnt.
   *
   *    As the neighbor entries are removed/change to pending, the refCnt is
   *    decremented. When the refCnt drops to 0, the classID is released.
   *
   *  This is also maintained in a map for every port.
   */
  using MacAndVlan2ClassIDAndRefCnt = boost::container::flat_map<
      std::pair<folly::MacAddress, VlanID>,
      std::pair<cfg::AclLookupClass, int>>;
  boost::container::flat_map<PortID, MacAndVlan2ClassIDAndRefCnt>
      port2MacAndVlanEntries_;

  /*
   * In theory, same IP may exist in different Vlans, thus maintain IP & Vlan
   * to classID mapping.
   * A route inherits classID of its nexthop. Maintaining IP to classID mapping
   * allows efficient lookup of nexthop's classID when a route is added.
   */
  boost::container::
      flat_map<std::pair<folly::IPAddress, VlanID>, cfg::AclLookupClass>
          ipAndVlan2ClassID_;

  bool inited_{false};

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
};

} // namespace facebook::fboss
