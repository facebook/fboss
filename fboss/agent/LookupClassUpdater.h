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

  template <typename NeighborEntryT>
  bool shouldProcessNeighborEntry(
      const std::shared_ptr<NeighborEntryT>& newEntry) const;
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

  friend class VlanTableDeltaCallbackGenerator;
  bool inited_{false};
};

} // namespace facebook::fboss
