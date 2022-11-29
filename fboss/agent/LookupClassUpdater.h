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
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {
class SwSwitch;

namespace detail {
template <typename AddrT>
struct EntryType;

template <>
struct EntryType<folly::IPAddressV4> {
  using type = std::shared_ptr<ArpEntry>;
};

template <>
struct EntryType<folly::IPAddressV6> {
  using type = std::shared_ptr<NdpEntry>;
};

template <>
struct EntryType<folly::MacAddress> {
  using type = std::shared_ptr<MacEntry>;
};
} // namespace detail

class LookupClassUpdater : public StateObserver {
 public:
  explicit LookupClassUpdater(SwSwitch* sw);
  ~LookupClassUpdater() override;

  void stateUpdated(const StateDelta& stateDelta) override;

  int getRefCnt(
      PortID portID,
      const folly::MacAddress mac,
      VlanID vlanID,
      cfg::AclLookupClass classID);

 private:
  using ClassID2Count = boost::container::flat_map<cfg::AclLookupClass, int>;

  bool portHasClassID(const std::shared_ptr<Port>& port);

  template <typename NeighborEntryT>
  bool shouldProcessNeighborEntry(
      const std::shared_ptr<NeighborEntryT>& newEntry,
      bool added) const;
  template <typename AddedNeighborEntryT>
  void processAdded(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<AddedNeighborEntryT>& addedEntry);
  template <typename RemovedNeighborEntryT>
  void processRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<RemovedNeighborEntryT>& removedEntry);
  template <typename ChangedNeighborEntryT>
  void processChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<ChangedNeighborEntryT>& oldEntry,
      const std::shared_ptr<ChangedNeighborEntryT>& newEntry);

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

  template <typename NewEntryT>
  bool isPresentInBlockList(
      VlanID vlanID,
      const std::shared_ptr<NewEntryT>& newEntry);

  template <typename AddrT>
  void processBlockNeighborUpdatesHelper(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<Vlan>& vlan,
      const AddrT& ipAddress);

  void processBlockNeighborUpdates(const StateDelta& stateDelta);

  template <typename NewEntryT>
  bool isPresentInMacAddrsBlockList(
      VlanID vlanID,
      const std::shared_ptr<NewEntryT>& newEntry);

  template <typename AddrT>
  void updateClassIDForEveryNeighborForMac(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<Vlan>& vlan,
      folly::MacAddress macAddress);
  template <typename AddrT>
  void removeClassIDForEveryNeighborForMac(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<Vlan>& vlan,
      folly::MacAddress macAddress);

  void processMacAddrsToBlockUpdates(const StateDelta& stateDelta);

  /*
   * Methods to iterate over MacTable, ArpTable or NdpTable or table deltas.
   */
  template <typename AddrT>
  auto getTable(const std::shared_ptr<Vlan>& vlan);

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
   * Some use cases (e.g. DR) requires blocking traffic to specific neighbors.
   * This solution has two parts viz.:
   *
   * static configuration:
   *  ACL:: matcher: CLASS_DROP action: Drop. One each for L2, neighbor and
   * route.
   *
   * dynamic configuration:
   *  Dynamically associate/disassociate CLASS_DROP with provided neighbors to
   *  block/unblock traffic egress to them.
   *  LookupClassUpdater implements this.
   *
   *  blockedNeighbors_ maintains the current set of neighbors to block traffic
   *  to.
   */
  std::set<std::pair<VlanID, folly::IPAddress>> blockedNeighbors_;

  /*
   * The approach to use IPs to block traffic to a server has few challenges
   * when multiple IPs (say IP1 and IP2) share the same MAC1.
   *     - Blocking traffic to MAC1 is necessary to block the L2 traffic to
   *       IP1, but that inadvertently blocks the L2 traffic to IP2 as well,
   *       while leaving L3 traffic to IP2 unblocked.
   *     - DR use case requires blocking ALL the traffic to a specific server,
   *       and thus wants to block ALL the traffic to IP1 as well as IP2.
   *       However, that puts additional requirement on the DR test to
   *       discover (or compute e.g. IPv6 auto configured addr) ALL the IPs
   *       (IP1, IP2 etc.) for a server.
   *     - Given an IP-to-block, FBOSS could compute/track all the IPs that
   *       share the same MAC with IP-to-block, but a design with implicit
   *       configuration is hard to reason/explain/likely bug-prone.
   *     - ALL the traffic to a server (L2 link local or otherwise, Neighbor,
   *       Routed) uses the same destination MAC.
   *
   * Given all the above, to address the use case of "blocking ALL the traffic
   * to a server" by implemeting "blocking ALL the traffic to a specified MAC
   * address".
   *
   * Similar to blocking traffic to Neighbor IP solution, this solution has two
   * parts viz.:
   *
   * static configuration:
   *     ACL:: matcher: CLASS_DROP action: Drop.
   *     One such ACL each for L2, neighbor and route.
   *
   * dynamic configuration:
   *  Dynamically associate/disassociate CLASS_DROP with provided MAC addrs to
   *  block/unblock traffic egress to them.
   *  LookupClassUpdater implements this.
   *
   *  macAddrsToBlock_ maintains the current set of MAC addrs to block the
   *  traffic to.
   */
  std::set<std::pair<VlanID, folly::MacAddress>> macAddrsToBlock_;

  friend class VlanTableDeltaCallbackGenerator;
  bool inited_{false};
};

} // namespace facebook::fboss
