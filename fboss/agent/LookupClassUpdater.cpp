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
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::isEmpty;

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

template <typename AddrT>
auto LookupClassUpdater::getTableDelta(const VlanDelta& vlanDelta) {
  if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
    return vlanDelta.getMacDelta();
  } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
    return vlanDelta.getArpDelta();
  } else {
    return vlanDelta.getNdpDelta();
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
  XLOG(DBG2) << "Updating Qos Policy for Neighbor:: port: "
             << removedEntry->str() << " classID: None";

  if constexpr (std::is_same_v<RemovedEntryT, MacEntry>) {
    auto removeMacClassIDFn = [vlan, removedEntry](
                                  const std::shared_ptr<SwitchState>& state) {
      return MacTableUtils::removeClassIDForEntry(state, vlan, removedEntry);
    };

    sw_->updateState("remove classID: ", std::move(removeMacClassIDFn));
  } else {
    auto updater = sw_->getNeighborUpdater();
    updateIpAndVlan2ClassID(removedEntry->getIP(), vlan, std::nullopt);
    updater->updateEntryClassID(vlan, removedEntry->getIP());
    updateRouteClassID(removedEntry->getIP());
  }
}

bool LookupClassUpdater::isInited(PortID portID) {
  return port2MacAndVlanEntries_.find(portID) !=
      port2MacAndVlanEntries_.end() &&
      port2ClassIDAndCount_.find(portID) != port2ClassIDAndCount_.end();
}

bool LookupClassUpdater::belongsToSubnetInCache(
    const folly::IPAddress& ipToSearch) {
  /*
   * When a neighbor is resolved, given the egress port, we could tell which
   * vlan(s) it is part of. However, the neighbor may not be resolved
   * yet, thus search if the input ip exists in any of the subnets.
   *
   * In practice, today, there is only one vlan for downlink ports.
   * Even if that changes in future, we should be OK:
   * This function would be used to decide whether or not to store nexthop to
   * route mapping for a given nexthop. At worst, the caller would end up
   * storing nexthop to route mapping for more routes than necessary, but the
   * functionality would still be correct.
   */
  for (const auto& [vlan, subnetsCache] : vlan2SubnetsCache_) {
    for (const auto& [ipAddress, mask] : subnetsCache) {
      if (ipToSearch.inSubnet(ipAddress, mask)) {
        return true;
      }
    }
  }

  return false;
}

void LookupClassUpdater::updateSubnetsToCache(
    const std::shared_ptr<SwitchState>& switchState,
    std::shared_ptr<Port> port) {
  for (const auto& [vlanID, vlanInfo] : port->getVlans()) {
    auto vlan = switchState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    auto& subnetsCache = vlan2SubnetsCache_[vlanID];
    auto interface =
        switchState->getInterfaces()->getInterfaceIf(vlan->getInterfaceID());
    if (interface) {
      for (auto address : interface->getAddresses()) {
        subnetsCache.insert(address);
      }
    }
  }
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

  /*
   * A route inherits classID, if any, of its nexthop. A nexthop would only get
   * classID if it belongs to a subnet of an interface on the port that has
   * non-empty list of lookupClasses.
   */
  if (port->getLookupClassesToDistributeTrafficOn().size() > 0) {
    updateSubnetsToCache(switchState, port);
  }
}

std::optional<cfg::AclLookupClass> LookupClassUpdater::getClassIDForIpAndVlan(
    const folly::IPAddress& ip,
    VlanID vlanID) {
  auto iter = ipAndVlan2ClassID_.find(std::make_pair(ip, vlanID));
  if (iter != ipAndVlan2ClassID_.end()) {
    return iter->second;
  }

  return std::nullopt;
}

void LookupClassUpdater::updateIpAndVlan2ClassID(
    const folly::IPAddress& ip,
    VlanID vlanID,
    std::optional<cfg::AclLookupClass> classID) {
  auto iter = ipAndVlan2ClassID_.find(std::make_pair(ip, vlanID));

  if (classID.has_value()) {
    CHECK(iter == ipAndVlan2ClassID_.end());
    ipAndVlan2ClassID_.insert(
        std::make_pair(std::make_pair(ip, vlanID), classID.value()));
  } else {
    CHECK(iter != ipAndVlan2ClassID_.end());
    ipAndVlan2ClassID_.erase(iter);
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

  XLOG(DBG2) << "Updating Qos Policy for Neighbor:: port: " << newEntry->str()
             << " classID: " << static_cast<int>(classID);

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
    updateIpAndVlan2ClassID(newEntry->getIP(), vlanID, classID);
    updater->updateEntryClassID(vlanID, newEntry->getIP(), classID);
    updateRouteClassID(newEntry->getIP(), classID);
  }
}

void LookupClassUpdater::updateRouteClassID(
    const folly::IPAddress& /*unused*/,
    std::optional<cfg::AclLookupClass> /*unused*/) {
  // TODO (skhare) add support
}

template <typename AddedEntryT>
void LookupClassUpdater::processNeighborAdded(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const std::shared_ptr<AddedEntryT>& addedEntry) {
  CHECK(addedEntry);
  CHECK(addedEntry->getPort().isPhysicalPort());

  if constexpr (std::is_same_v<AddedEntryT, MacEntry>) {
    updateNeighborClassID(switchState, vlan, addedEntry);
  } else {
    if (addedEntry->isReachable()) {
      updateNeighborClassID(switchState, vlan, addedEntry);
    }
  }
}

template <typename RemovedEntryT>
void LookupClassUpdater::processNeighborRemoved(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const std::shared_ptr<RemovedEntryT>& removedEntry) {
  CHECK(removedEntry);
  CHECK(removedEntry->getPort().isPhysicalPort());

  removeClassIDForPortAndMac(switchState, vlan, removedEntry);
}

template <typename ChangedEntryT>
void LookupClassUpdater::processNeighborChanged(
    const StateDelta& stateDelta,
    VlanID vlan,
    const std::shared_ptr<ChangedEntryT>& oldEntry,
    const std::shared_ptr<ChangedEntryT>& newEntry) {
  CHECK(oldEntry);
  CHECK(newEntry);
  CHECK(oldEntry->getPort().isPhysicalPort());
  CHECK(newEntry->getPort().isPhysicalPort());

  if constexpr (std::is_same_v<ChangedEntryT, MacEntry>) {
    /*
     * MAC Move
     */
    if (oldEntry->getPort().phyPortID() != newEntry->getPort().phyPortID()) {
      removeClassIDForPortAndMac(stateDelta.oldState(), vlan, oldEntry);
      updateNeighborClassID(stateDelta.newState(), vlan, newEntry);
    }
  } else {
    CHECK_EQ(oldEntry->getIP(), newEntry->getIP());
    if (!oldEntry->isReachable() && newEntry->isReachable()) {
      updateNeighborClassID(stateDelta.newState(), vlan, newEntry);
    }

    if (oldEntry->isReachable() && !newEntry->isReachable()) {
      removeClassIDForPortAndMac(stateDelta.oldState(), vlan, oldEntry);
    }

    /*
     * If the neighbor remains reachable as before but resolves to different
     * MAC/port, remove the classID for oldEntry and add classID for newEntry.
     */
    if ((oldEntry->isReachable() && newEntry->isReachable()) &&
        (oldEntry->getPort().phyPortID() != newEntry->getPort().phyPortID() ||
         oldEntry->getMac() != newEntry->getMac())) {
      removeClassIDForPortAndMac(stateDelta.oldState(), vlan, oldEntry);
      updateNeighborClassID(stateDelta.newState(), vlan, newEntry);
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::processNeighborUpdates(const StateDelta& stateDelta) {
  for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
    auto newVlan = vlanDelta.getNew();
    if (!newVlan) {
      continue;
    }
    auto vlan = newVlan->getID();

    for (const auto& delta : getTableDelta<AddrT>(vlanDelta)) {
      auto oldEntry = delta.getOld();
      auto newEntry = delta.getNew();
      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if ((oldEntry && !oldEntry->getPort().isPhysicalPort()) ||
          (newEntry && !newEntry->getPort().isPhysicalPort())) {
        continue;
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
      if (newEntry && newEntry->getClassID().has_value()) {
        continue;
      }

      if (!oldEntry) {
        processNeighborAdded(stateDelta.newState(), vlan, newEntry);
      } else if (!newEntry) {
        processNeighborRemoved(stateDelta.oldState(), vlan, oldEntry);
      } else {
        processNeighborChanged(stateDelta, vlan, oldEntry, newEntry);
      }
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
          updateIpAndVlan2ClassID(entry.get()->getIP(), vlanID, std::nullopt);
          updater->updateEntryClassID(vlanID, entry.get()->getIP());
          updateRouteClassID(entry.get()->getIP());
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
   *  - ARP/NDP neighbors go to pending and processNeighborChanged()
   *    disassociates classID from the neighbor entry.
   *  - L2 entries remain in hardware L2 table and thus we retain classID
   *    assocaited with L2 entries even when port is down. However, L2
   *    entries on down ports could be aged out. This will be processed by
   *    processNeighborRemoved() and the classID would be freed up.
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

template <typename RouteT>
void LookupClassUpdater::processRouteAdded(
    const std::shared_ptr<SwitchState>& /*unused*/,
    RouterID /*unused*/,
    const std::shared_ptr<RouteT>& /*unused*/) {
  // TODO (skhare) add support
}

template <typename RouteT>
void LookupClassUpdater::processRouteRemoved(
    const std::shared_ptr<RouteT>& /*unused*/) {
  // TODO (skhare) add support
}

template <typename RouteT>
void LookupClassUpdater::processRouteChanged(
    const std::shared_ptr<RouteT>& /*unused*/,
    const std::shared_ptr<RouteT>& /*unused*/) {
  // TODO (skhare) add support
}

template <typename AddrT>
void LookupClassUpdater::processRouteUpdates(const StateDelta& stateDelta) {
  auto switchState = stateDelta.newState();

  for (auto const& rtDelta : stateDelta.getRouteTablesDelta()) {
    auto const& newRouteTable = rtDelta.getNew();
    if (!newRouteTable) {
      return;
    }

    auto rid = newRouteTable->getID();
    for (auto const& routeDelta : rtDelta.getRoutesDelta<AddrT>()) {
      auto const& oldRoute = routeDelta.getOld();
      auto const& newRoute = routeDelta.getNew();

      if (!oldRoute) {
        processRouteAdded(stateDelta.newState(), rid, newRoute);
      } else if (!newRoute) {
        processRouteRemoved(newRoute);
      } else {
        processRouteChanged(oldRoute, newRoute);
      }
    }
  }
}

void LookupClassUpdater::stateUpdated(const StateDelta& stateDelta) {
  if (!inited_) {
    vlan2SubnetsCache_.clear();
    updateStateObserverLocalCache(stateDelta.newState());
    inited_ = true;
  }

  processNeighborUpdates<folly::MacAddress>(stateDelta);
  processNeighborUpdates<folly::IPAddressV6>(stateDelta);
  processNeighborUpdates<folly::IPAddressV4>(stateDelta);

  processPortUpdates(stateDelta);

  processRouteUpdates<folly::IPAddressV6>(stateDelta);
  processRouteUpdates<folly::IPAddressV4>(stateDelta);
}

} // namespace facebook::fboss
