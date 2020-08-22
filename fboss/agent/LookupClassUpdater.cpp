/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/LookupClassUpdater.h"

#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/VlanTableDeltaCallbackGenerator.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

template <typename AddrT>
auto LookupClassUpdater::getTable(const std::shared_ptr<Vlan>& vlan) {
  if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
    return vlan->getMacTable();
  } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
    return vlan->getArpTable();
  } else {
    return vlan->getNdpTable();
  }
}

int LookupClassUpdater::getRefCnt(
    PortID portID,
    const folly::MacAddress mac,
    VlanID vlanID,
    cfg::AclLookupClass classID) {
  auto& macAndVlan2ClassIDAndRefCnt = port2MacAndVlanEntries_[portID];
  auto iter = macAndVlan2ClassIDAndRefCnt.find(std::make_pair(mac, vlanID));
  if (iter == macAndVlan2ClassIDAndRefCnt.end()) {
    return 0;
  } else {
    auto& [_classID, _refCnt] = iter->second;
    CHECK(classID == _classID);
    return _refCnt;
  }
}

cfg::AclLookupClass LookupClassUpdater::getClassIDwithMinimumNeighbors(
    ClassID2Count classID2Count) const {
  auto minItr = std::min_element(
      classID2Count.begin(),
      classID2Count.end(),
      [=](const auto& l, const auto& r) { return l.second < r.second; });

  CHECK(minItr != classID2Count.end());
  return minItr->first;
}

template <typename RemovedEntryT>
void LookupClassUpdater::removeClassIDForPortAndMac(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const std::shared_ptr<RemovedEntryT>& removedEntry) {
  CHECK(removedEntry->getPort().isPhysicalPort());

  auto portID = removedEntry->getPort().phyPortID();
  auto port = switchState->getPorts()->getPortIf(portID);

  if (!port || port->getLookupClassesToDistributeTrafficOn().size() == 0) {
    /*
     * Only downlink ports of Yosemite RSW will have lookupClasses
     * configured. For all other ports, control returns here.
     */
    return;
  }

  removeNeighborFromLocalCacheForEntry(removedEntry, vlan);
  XLOG(DBG2) << "Updating Qos Policy for Removed Neighbor: port: "
             << removedEntry->str() << " classID: None";

  if constexpr (std::is_same_v<RemovedEntryT, MacEntry>) {
    auto removeMacClassIDFn = [vlan, removedEntry](
                                  const std::shared_ptr<SwitchState>& state) {
      return MacTableUtils::removeClassIDForEntry(state, vlan, removedEntry);
    };

    sw_->updateState("remove classID: ", std::move(removeMacClassIDFn));
  } else {
    auto updater = sw_->getNeighborUpdater();
    updater->updateEntryClassID(vlan, removedEntry->getIP());
  }
}

bool LookupClassUpdater::isInited(PortID portID) {
  return port2MacAndVlanEntries_.find(portID) !=
      port2MacAndVlanEntries_.end() &&
      port2ClassIDAndCount_.find(portID) != port2ClassIDAndCount_.end();
}

void LookupClassUpdater::initPort(
    const std::shared_ptr<SwitchState>& switchState,
    const std::shared_ptr<Port>& port) {
  auto& macAndVlan2ClassIDAndRefCnt = port2MacAndVlanEntries_[port->getID()];
  auto& classID2Count = port2ClassIDAndCount_[port->getID()];

  macAndVlan2ClassIDAndRefCnt.clear();
  classID2Count.clear();

  for (auto classID : port->getLookupClassesToDistributeTrafficOn()) {
    classID2Count[classID] = 0;
  }
}

template <typename NewEntryT>
void LookupClassUpdater::updateNeighborClassID(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlanID,
    const std::shared_ptr<NewEntryT>& newEntry) {
  CHECK(newEntry->getPort().isPhysicalPort());

  auto portID = newEntry->getPort().phyPortID();
  auto port = switchState->getPorts()->getPortIf(portID);

  if (!port || port->getLookupClassesToDistributeTrafficOn().size() == 0) {
    /*
     * Only downlink ports of Yosemite RSW will have lookupClasses
     * configured. For all other ports, control returns here.
     */
    return;
  }

  auto mac = newEntry->getMac();
  auto& macAndVlan2ClassIDAndRefCnt = port2MacAndVlanEntries_[port->getID()];
  auto& classID2Count = port2ClassIDAndCount_[port->getID()];

  cfg::AclLookupClass classID;

  auto iter = macAndVlan2ClassIDAndRefCnt.find(std::make_pair(mac, vlanID));
  if (iter == macAndVlan2ClassIDAndRefCnt.end()) {
    classID = getClassIDwithMinimumNeighbors(classID2Count);
    macAndVlan2ClassIDAndRefCnt.insert(std::make_pair(
        std::make_pair(mac, vlanID),
        std::make_pair(classID, 1 /* initialize refCnt */)));
    classID2Count[classID]++;
  } else {
    auto& [_classID, refCnt] = iter->second;
    classID = _classID;
    refCnt++;
  }

  XLOG(DBG2) << "Updating Qos Policy for Updated Neighbor: port: "
             << newEntry->str() << " classID: " << static_cast<int>(classID);

  if constexpr (std::is_same_v<NewEntryT, MacEntry>) {
    auto updateMacClassIDFn =
        [vlanID, newEntry, classID](const std::shared_ptr<SwitchState>& state) {
          return MacTableUtils::updateOrAddEntryWithClassID(
              state, vlanID, newEntry, classID);
        };

    sw_->updateState(
        folly::to<std::string>("configure lookup classID: ", classID),
        std::move(updateMacClassIDFn));
  } else {
    auto updater = sw_->getNeighborUpdater();
    updater->updateEntryClassID(vlanID, newEntry->getIP(), classID);
  }
}

template <typename NeighborEntryT>
bool LookupClassUpdater::shouldProcessNeighborEntry(
    const std::shared_ptr<NeighborEntryT>& entry) const {
  /*
   * At this point in time, queue-per-host fix is needed (and thus
   * supported) for physical link only.
   */
  if (!(entry && entry->getPort().isPhysicalPort())) {
    return false;
  }
  /*
   * If newEntry already has classID populated, don't process.
   * This can happen in two cases:
   *  o Warmboot: prior to warmboot, neighbor entries may have a classID
   *    associated with them. updateStateObserverLocalCache() consumes this
   *    info to populate its local cache, so do nothing here.
   *  o Once LookupClassUpdater chooses classID for a neighbor, it
   *    schedules a state update. After the state update is run, all state
   *    observers are notified. At that time, LookupClassUpdater will
   *    receive a stateDelta that contains classID assigned to new neighbor
   *    entry. Since this will be side-effect of state update that
   *    LookupClassUpdater triggered, there is nothing to do here.
   */
  if (entry->getClassID().has_value()) {
    return false;
  }
  return true;
}

template <typename AddedNeighborEntryT>
void LookupClassUpdater::processAdded(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const std::shared_ptr<AddedNeighborEntryT>& addedEntry) {
  CHECK(addedEntry);
  if (!shouldProcessNeighborEntry(addedEntry)) {
    return;
  }
  CHECK(addedEntry->getPort().isPhysicalPort());
  if constexpr (std::is_same_v<AddedNeighborEntryT, MacEntry>) {
    updateNeighborClassID(switchState, vlan, addedEntry);
  } else {
    if (addedEntry->isReachable()) {
      updateNeighborClassID(switchState, vlan, addedEntry);
    }
  }
}

template <typename RemovedNeighborEntryT>
void LookupClassUpdater::processRemoved(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const std::shared_ptr<RemovedNeighborEntryT>& removedEntry) {
  CHECK(removedEntry);
  /*
   * At this point in time, queue-per-host fix is needed (and thus
   * supported) for physical link only.
   */
  if (!removedEntry->getPort().isPhysicalPort()) {
    return;
  }

  removeClassIDForPortAndMac(switchState, vlan, removedEntry);
}

template <typename ChangedNeighborEntryT>
void LookupClassUpdater::processChanged(
    const StateDelta& stateDelta,
    VlanID vlan,
    const std::shared_ptr<ChangedNeighborEntryT>& oldEntry,
    const std::shared_ptr<ChangedNeighborEntryT>& newEntry) {
  CHECK(oldEntry);
  CHECK(newEntry);
  if (!(shouldProcessNeighborEntry(newEntry) &&
        oldEntry->getPort().isPhysicalPort())) {
    // TODO - ideally we shouldn't care about whether
    // the old port was a non physical port (LAG) or not
    // but unfortunately our logic below assumes that both
    // previous and this new entry ports are physical ports.
    return;
  }
  CHECK(oldEntry->getPort().isPhysicalPort());
  CHECK(newEntry->getPort().isPhysicalPort());

  if constexpr (std::is_same_v<ChangedNeighborEntryT, MacEntry>) {
    /*
     * MAC Move
     */
    if (oldEntry->getPort().phyPortID() != newEntry->getPort().phyPortID()) {
      removeClassIDForPortAndMac(stateDelta.oldState(), vlan, oldEntry);
      updateNeighborClassID(stateDelta.newState(), vlan, newEntry);
    } else {
      /* When a MAC address with class ID is aged out and relearned in a
       * quick succession, the state changes can collapse and lose the
       * class ID. This makes sure that class ID is not lost. (T64576277)
       */
      auto portID = newEntry->getPort().phyPortID();
      auto port = stateDelta.newState()->getPorts()->getPortIf(portID);
      // Only downlink ports of MH-NIC RSW setups will have lookupClasses
      // Thus size of getLookupClassesToDistributeTrafficOn would be > 0
      // For other MH-NIC, this block should not be applied.
      if (oldEntry->getClassID().has_value() &&
          !newEntry->getClassID().has_value() && port &&
          port->getLookupClassesToDistributeTrafficOn().size() != 0) {
        auto classID = oldEntry->getClassID().value();
        auto updateMacClassIDFn =
            [vlan, newEntry, classID](
                const std::shared_ptr<SwitchState>& state) {
              return MacTableUtils::updateOrAddEntryWithClassID(
                  state, vlan, newEntry, classID);
            };

        sw_->updateState(
            folly::to<std::string>("Reconfigure lookup classID: ", classID),
            std::move(updateMacClassIDFn));
      } else {
        updateNeighborClassID(stateDelta.newState(), vlan, newEntry);
      }
    }
  } else {
    CHECK_EQ(oldEntry->getIP(), newEntry->getIP());
    if (!oldEntry->isReachable() && newEntry->isReachable()) {
      updateNeighborClassID(stateDelta.newState(), vlan, newEntry);
    } else if (oldEntry->isReachable() && !newEntry->isReachable()) {
      removeClassIDForPortAndMac(stateDelta.oldState(), vlan, oldEntry);
    }
    /*
     * If the neighbor remains reachable as before but resolves to different
     * MAC/port, remove the classID for oldEntry and add classID for newEntry.
     */
    else if (
        (oldEntry->isReachable() && newEntry->isReachable()) &&
        (oldEntry->getPort().phyPortID() != newEntry->getPort().phyPortID() ||
         oldEntry->getMac() != newEntry->getMac())) {
      removeClassIDForPortAndMac(stateDelta.oldState(), vlan, oldEntry);
      updateNeighborClassID(stateDelta.newState(), vlan, newEntry);
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::clearClassIdsForResolvedNeighbors(
    const std::shared_ptr<SwitchState>& switchState,
    PortID portID) {
  auto port = switchState->getPorts()->getPortIf(portID);
  for (auto vlanMember : port->getVlans()) {
    auto vlanID = vlanMember.first;
    auto vlan = switchState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    for (const auto& entry : *getTable<AddrT>(vlan)) {
      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if (entry->getPort().isPhysicalPort() &&
          entry->getPort().phyPortID() == portID &&
          entry->getClassID().has_value()) {
        removeNeighborFromLocalCacheForEntry(entry, vlanID);
        if constexpr (std::is_same_v<AddrT, MacAddress>) {
          auto removeMacClassIDFn =
              [vlanID, entry](const std::shared_ptr<SwitchState>& state) {
                return MacTableUtils::removeClassIDForEntry(
                    state, vlanID, entry);
              };

          sw_->updateState("remove classID: ", std::move(removeMacClassIDFn));
        } else {
          auto updater = sw_->getNeighborUpdater();
          updater->updateEntryClassID(vlanID, entry.get()->getIP());
        }
      }
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::repopulateClassIdsForResolvedNeighbors(
    const std::shared_ptr<SwitchState>& switchState,
    PortID portID) {
  auto port = switchState->getPorts()->getPortIf(portID);
  for (auto vlanMember : port->getVlans()) {
    auto vlanID = vlanMember.first;
    auto vlan = switchState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    for (const auto& entry : *getTable<AddrT>(vlan)) {
      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if (entry->getPort().isPhysicalPort() &&
          entry->getPort().phyPortID() == portID) {
        updateNeighborClassID(switchState, vlanID, entry);
      }
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::validateRemovedPortEntries(
    const std::shared_ptr<Vlan>& vlan,
    PortID portID) {
  /*
   * At this point in time, queue-per-host fix is needed (and thus
   * supported) for physical link only.
   */

  for (const auto& entry : *getTable<AddrT>(vlan)) {
    if (entry->getPort().isPhysicalPort()) {
      CHECK(entry->getPort().phyPortID() != portID);
    }
  }
}

void LookupClassUpdater::processPortAdded(
    const std::shared_ptr<SwitchState>& switchState,
    const std::shared_ptr<Port>& addedPort) {
  CHECK(addedPort);
  if (addedPort->getLookupClassesToDistributeTrafficOn().size() == 0) {
    /*
     * Only downlink ports of Yosemite RSW will have lookupClasses
     * configured. For all other ports, control returns here.
     */
    return;
  }

  if (!isInited(addedPort->getID())) {
    initPort(switchState, addedPort);
  }
}

void LookupClassUpdater::processPortRemoved(
    const std::shared_ptr<SwitchState>& switchState,
    const std::shared_ptr<Port>& removedPort) {
  CHECK(removedPort);
  auto portID = removedPort->getID();

  /*
   * The port that is being removed should not be next hop for any neighbor.
   * in the new switch state.
   */
  for (auto vlanMember : removedPort->getVlans()) {
    auto vlanID = vlanMember.first;
    auto vlan = switchState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    validateRemovedPortEntries<folly::MacAddress>(vlan, portID);
    validateRemovedPortEntries<folly::IPAddressV6>(vlan, portID);
    validateRemovedPortEntries<folly::IPAddressV4>(vlan, portID);
  }

  port2MacAndVlanEntries_.erase(portID);
  port2ClassIDAndCount_.erase(portID);
}

void LookupClassUpdater::processPortChanged(
    const StateDelta& stateDelta,
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  CHECK(oldPort && newPort);
  CHECK_EQ(oldPort->getID(), newPort->getID());
  /*
   * If the lists of lookupClasses are identical (common case), do nothing.
   * If the lists are different e.g. fewer/more classes (unlikely - but the
   * list of classes can go from > 0 to 0 if this feature is disabled),
   * clear all the ClassIDs associated with every resolved neighbor for this
   * egress port. Then, repopulate classIDs using the newPort's list of
   * lookupClasses.
   * To avoid unncessary shuffling when order of lookup classes changes,
   * (e.g. due to config change), sort and compare. This requires a deep
   * copy and sorting, but in practice, the list of lookup classes should be
   * small (< 10).
   */
  auto oldLookupClasses{oldPort->getLookupClassesToDistributeTrafficOn()};
  auto newLookupClasses{newPort->getLookupClassesToDistributeTrafficOn()};

  sort(oldLookupClasses.begin(), oldLookupClasses.end());
  sort(newLookupClasses.begin(), newLookupClasses.end());

  /*
   * We don't need to handle port down here: on port down,
   *  - ARP/NDP neighbors go to pending and processChanged()
   *    disassociates classID from the neighbor entry.
   *  - L2 entries remain in hardware L2 table and thus we retain classID
   *    assocaited with L2 entries even when port is down. However, L2
   *    entries on down ports could be aged out. This will be processed by
   *    processRemoved() and the classID would be freed up.
   */
  if (oldLookupClasses == newLookupClasses) {
    return;
  }

  clearClassIdsForResolvedNeighbors<folly::MacAddress>(
      stateDelta.oldState(), newPort->getID());
  clearClassIdsForResolvedNeighbors<folly::IPAddressV6>(
      stateDelta.oldState(), newPort->getID());
  clearClassIdsForResolvedNeighbors<folly::IPAddressV4>(
      stateDelta.oldState(), newPort->getID());

  initPort(stateDelta.newState(), newPort);

  repopulateClassIdsForResolvedNeighbors<folly::MacAddress>(
      stateDelta.newState(), newPort->getID());
  repopulateClassIdsForResolvedNeighbors<folly::IPAddressV6>(
      stateDelta.newState(), newPort->getID());
  repopulateClassIdsForResolvedNeighbors<folly::IPAddressV4>(
      stateDelta.newState(), newPort->getID());
}

void LookupClassUpdater::processPortUpdates(const StateDelta& stateDelta) {
  for (const auto& delta : stateDelta.getPortsDelta()) {
    auto oldPort = delta.getOld();
    auto newPort = delta.getNew();

    if (!oldPort && newPort) {
      processPortAdded(stateDelta.newState(), newPort);
    } else if (oldPort && !newPort) {
      /*
       * Asserts port being removed is not next hop for any neighbor.
       * in the newState, thus we need to pass newState to PortRemove.
       */
      processPortRemoved(stateDelta.newState(), oldPort);
    } else {
      processPortChanged(stateDelta, oldPort, newPort);
    }
  }
}

template <typename RemovedEntryT>
void LookupClassUpdater::removeNeighborFromLocalCacheForEntry(
    const std::shared_ptr<RemovedEntryT>& removedEntry,
    VlanID vlanID) {
  CHECK(removedEntry->getPort().isPhysicalPort());

  auto portID = removedEntry->getPort().phyPortID();
  auto mac = removedEntry->getMac();

  auto& macAndVlan2ClassIDAndRefCnt = port2MacAndVlanEntries_[portID];
  auto& [classID, refCnt] =
      macAndVlan2ClassIDAndRefCnt[std::make_pair(mac, vlanID)];
  auto& classID2Count = port2ClassIDAndCount_[portID];

  CHECK_GT(refCnt, 0);

  refCnt--;

  if (refCnt == 0) {
    classID2Count[classID]--;
    CHECK_GE(classID2Count[classID], 0);
    macAndVlan2ClassIDAndRefCnt.erase(std::make_pair(mac, vlanID));
  }
}

template <typename NewEntryT>
void LookupClassUpdater::updateStateObserverLocalCacheForEntry(
    const std::shared_ptr<NewEntryT>& newEntry,
    VlanID vlanID) {
  CHECK(newEntry->getPort().isPhysicalPort());

  auto portID = newEntry->getPort().phyPortID();
  auto& macAndVlan2ClassIDAndRefCnt = port2MacAndVlanEntries_[portID];
  auto& classID2Count = port2ClassIDAndCount_[portID];
  auto classID = newEntry->getClassID().value();
  auto mac = newEntry->getMac();

  auto iter = macAndVlan2ClassIDAndRefCnt.find(std::make_pair(mac, vlanID));
  if (iter == macAndVlan2ClassIDAndRefCnt.end()) {
    macAndVlan2ClassIDAndRefCnt.insert(std::make_pair(
        std::make_pair(mac, vlanID),
        std::make_pair(classID, 1 /* initialize refCnt */)));
    classID2Count[classID]++;
  } else {
    auto& [classID_, refCnt] = iter->second;
    CHECK(classID_ == classID);
    refCnt++;
  }
}

template <typename AddrT>
void LookupClassUpdater::updateStateObserverLocalCacheHelper(
    const std::shared_ptr<Vlan>& vlan,
    const std::shared_ptr<Port>& port) {
  for (const auto& entry : *(getTable<AddrT>(vlan))) {
    if (entry->getPort().isPhysicalPort() &&
        entry->getPort().phyPortID() == port->getID() &&
        entry->getClassID().has_value()) {
      updateStateObserverLocalCacheForEntry(entry, vlan->getID());
    }
  }
}

/*
 * During warmboot, switchState may hold neighbor entries that could already
 * have classIDs assigned prior to the warmboot. This method uses the classID
 * info in switchState to populate mac to classID mappings cached in
 * LookupClassUpdater.
 */
void LookupClassUpdater::updateStateObserverLocalCache(
    const std::shared_ptr<SwitchState>& switchState) {
  CHECK(!inited_);

  for (auto port : *switchState->getPorts()) {
    initPort(switchState, port);
    for (auto vlanMember : port->getVlans()) {
      auto vlanID = vlanMember.first;
      auto vlan = switchState->getVlans()->getVlanIf(vlanID);
      if (!vlan) {
        continue;
      }
      updateStateObserverLocalCacheHelper<folly::MacAddress>(vlan, port);
      updateStateObserverLocalCacheHelper<folly::IPAddressV6>(vlan, port);
      updateStateObserverLocalCacheHelper<folly::IPAddressV4>(vlan, port);
    }
  }
}

void LookupClassUpdater::stateUpdated(const StateDelta& stateDelta) {
  if (!inited_) {
    updateStateObserverLocalCache(stateDelta.newState());
    inited_ = true;
  }

  VlanTableDeltaCallbackGenerator::genCallbacks(stateDelta, *this);
  processPortUpdates(stateDelta);
}

} // namespace facebook::fboss
