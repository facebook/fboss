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

 private:
  using ClassID2Count = boost::container::flat_map<cfg::AclLookupClass, int>;

  template <typename AddrT>
  void processNeighborUpdates(const StateDelta& stateDelta);

  template <typename NeighborEntryT>
  void processNeighborAdded(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const NeighborEntryT* addedEntry);
  template <typename NeighborEntryT>
  void processNeighborRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const NeighborEntryT* removedEntry);
  template <typename NeighborEntryT>
  void processNeighborChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const NeighborEntryT* oldEntry,
      const NeighborEntryT* newEntry);

  bool isInited(PortID portID);
  void initPort(const std::shared_ptr<Port>& port);

  template <typename NeighborEntryT>
  void updateNeighborClassID(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const NeighborEntryT* newEntry);

  template <typename NeighborEntryT>
  void removeClassIDForPortAndMac(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const NeighborEntryT* removedEntry);

  cfg::AclLookupClass getClassIDwithMinimumNeighbors(
      ClassID2Count classID2Count) const;

  template <typename NeighborEntryT>
  void removeNeighborFromLocalCacheForEntry(const NeighborEntryT* removedEntry);

  template <typename NeighborEntryT>
  void updateStateObserverLocalCacheForEntry(const NeighborEntryT* newEntry);

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

  SwSwitch* sw_;
  boost::container::flat_map<PortID, ClassID2Count> port2ClassIDAndCount_;

  using Mac2ClassID =
      boost::container::flat_map<folly::MacAddress, cfg::AclLookupClass>;
  boost::container::flat_map<PortID, Mac2ClassID> port2MacAndClassID_;

  bool inited_{false};
};
} // namespace fboss
} // namespace facebook
