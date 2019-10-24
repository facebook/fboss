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
  void processNeighborAdded(VlanID vlan, const NeighborEntryT* addedEntry);
  template <typename NeighborEntryT>
  void processNeighborRemoved(VlanID vlan, const NeighborEntryT* removedEntry);
  template <typename NeighborEntryT>
  void processNeighborChanged(
      VlanID vlan,
      const NeighborEntryT* oldEntry,
      const NeighborEntryT* newEntry);

  void initClassID2Count(std::shared_ptr<Port> port);

  template <typename NeighborEntryT>
  void updateNeighborClassID(VlanID vlan, const NeighborEntryT* newEntry);

  template <typename NeighborEntryT>
  void removeClassIDForPortAndMac(
      VlanID vlan,
      const NeighborEntryT* removedEntry);

  cfg::AclLookupClass getClassIDwithMinimumNeighbors(
      ClassID2Count classID2Count) const;

  template <typename NeighborEntryT>
  void updateStateObserverLocalCache(const NeighborEntryT* newEntry);

  SwSwitch* sw_;
  boost::container::flat_map<PortID, ClassID2Count> port2ClassIDAndCount_;

  using Mac2ClassID =
      boost::container::flat_map<folly::MacAddress, cfg::AclLookupClass>;
  boost::container::flat_map<PortID, Mac2ClassID> port2MacAndClassID_;
};
} // namespace fboss
} // namespace facebook
