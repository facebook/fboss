// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook {
namespace fboss {

class LookupClassUpdater : public AutoRegisterStateObserver {
 public:
  explicit LookupClassUpdater(SwSwitch* sw)
      : AutoRegisterStateObserver(sw, "LookupClassUpdater"), sw_(sw) {}
  ~LookupClassUpdater() override {}

  void stateUpdated(const StateDelta& stateDelta) override;

  int getRefCnt(
      PortID portID,
      const folly::MacAddress& mac,
      cfg::AclLookupClass classID);

 private:
  using ClassID2Count = boost::container::flat_map<cfg::AclLookupClass, int>;

  template <typename AddrT>
  void processNeighborUpdates(const StateDelta& stateDelta);

  template <typename AddedEntryT>
  void processNeighborAdded(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const AddedEntryT* addedEntry);
  template <typename RemovedEntryT>
  void processNeighborRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const RemovedEntryT* removedEntry);
  template <typename ChangedEntryT>
  void processNeighborChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const ChangedEntryT* oldEntry,
      const ChangedEntryT* newEntry);

  bool isInited(PortID portID);
  void initPort(const std::shared_ptr<Port>& port);

  template <typename NewEntryT>
  void updateNeighborClassID(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const NewEntryT* newEntry);

  template <typename RemovedEntryT>
  void removeClassIDForPortAndMac(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const RemovedEntryT* removedEntry);

  cfg::AclLookupClass getClassIDwithMinimumNeighbors(
      ClassID2Count classID2Count) const;

  template <typename RemovedEntryT>
  void removeNeighborFromLocalCacheForEntry(const RemovedEntryT* removedEntry);

  template <typename NewEntryT>
  void updateStateObserverLocalCacheForEntry(const NewEntryT* newEntry);

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
      std::shared_ptr<Port> addedPort);
  void processPortRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      std::shared_ptr<Port> port);
  void processPortChanged(
      const StateDelta& stateDelta,
      std::shared_ptr<Port> oldPort,
      std::shared_ptr<Port> newPort);

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

  SwSwitch* sw_;

  /*
   * Maintains the number of times a classID is used. When a new MAC requires
   * classID assignment, port2ClassIDAndCount_ is used to determine the least
   * used classID. This is maintained in a map for every port.
   */
  boost::container::flat_map<PortID, ClassID2Count> port2ClassIDAndCount_;

  /*
   * Maintains:
   *  - classID assigned to a MAC address.
   *  - refCnt to count the number of times a classID was requested for a MAC.
   *    Multiple neighbor IPs could have same MAC (e.g. link-local and global
   *    IP etc.). First neighbor with previously unseen MAC would cause classID
   *    assignment, while subsequent neighbors with same MAC would get the same
   *    classID but bump up the refCnt. As the neighbor entries are
   *    removed/change to pending, the refCnt is decremented. When the refCnt
   *    drops to 0, the classID is released.
   *
   *  This is also maintained in a map for every port.
   */
  using Mac2ClassIDAndRefCnt = boost::container::
      flat_map<folly::MacAddress, std::pair<cfg::AclLookupClass, int>>;
  boost::container::flat_map<PortID, Mac2ClassIDAndRefCnt> port2MacEntries_;

  bool inited_{false};
};
} // namespace fboss
} // namespace facebook
