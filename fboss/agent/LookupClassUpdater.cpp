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
#include <type_traits>
#include <utility>

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/VlanTableDeltaCallbackGenerator.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

LookupClassUpdater::LookupClassUpdater(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "LookupClassUpdater");
}
LookupClassUpdater::~LookupClassUpdater() {
  sw_->unregisterStateObserver(this);
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
  CHECK(!isNoHostRoute(removedEntry));

  auto portID = removedEntry->getPort().phyPortID();
  auto port = switchState->getPorts()->getPortIf(portID);

  bool isDropClassID = removedEntry->getClassID().has_value() &&
      removedEntry->getClassID().value() == cfg::AclLookupClass::CLASS_DROP;

  if (!portHasClassID(port) && !isDropClassID) {
    /*
     * Only downlink ports of Yosemite RSW will have lookupClasses
     * configured. For all other ports, control returns here.
     *
     * There is one exception to this: if a traffic to a neighbor is blocked,
     * it has CLASS_DROP associated with it on MH-NIC as well as non-MH-NIC
     * setup. Thus, continue to process CLASS_DROP.
     */
    return;
  }

  removeNeighborFromLocalCacheForEntry(removedEntry, vlan);

  if constexpr (std::is_same_v<RemovedEntryT, MacEntry>) {
    auto removeMacClassIDFn = [vlan, removedEntry](
                                  const std::shared_ptr<SwitchState>& state) {
      return MacTableUtils::removeClassIDForEntry(state, vlan, removedEntry);
    };

    sw_->updateState(
        "remove classID: " + removedEntry->str(),
        std::move(removeMacClassIDFn));
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
bool LookupClassUpdater::isPresentInBlockList(
    VlanID vlanID,
    const std::shared_ptr<NewEntryT>& newEntry) {
  if constexpr (std::is_same_v<NewEntryT, MacEntry>) {
    // neighbor block list comprises of IPs, no MACs.
    // processBlockNeighborUpdatesHelper handles update to MAC addresses by
    // removing classID for neighbor IP and MAC before adding new classID for
    // neighbor as well as MAC.
    return false;
  } else {
    auto it = blockedNeighbors_.find(
        std::make_pair(vlanID, folly::IPAddress(newEntry->getIP())));
    return it != blockedNeighbors_.end();
  }
}

template <typename NewEntryT>
bool LookupClassUpdater::isPresentInMacAddrsBlockList(
    VlanID vlanID,
    const std::shared_ptr<NewEntryT>& newEntry) {
  // Check if macAddress for the newEntry (macEntry or neighibor entry) is
  // present in the block list.
  auto it = macAddrsToBlock_.find(std::make_pair(vlanID, newEntry->getMac()));
  return it != macAddrsToBlock_.end();
}

template <typename NewEntryT>
void LookupClassUpdater::updateNeighborClassID(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlanID,
    const std::shared_ptr<NewEntryT>& newEntry) {
  CHECK(newEntry->getPort().isPhysicalPort());

  auto portID = newEntry->getPort().phyPortID();
  auto port = switchState->getPorts()->getPortIf(portID);
  if (!port) {
    return;
  }

  auto noLookupClasses = !portHasClassID(port);

  auto setDropClassID = isPresentInBlockList(vlanID, newEntry) ||
      isPresentInMacAddrsBlockList(vlanID, newEntry);

  auto mac = newEntry->getMac();
  auto& macAndVlan2ClassIDAndRefCnt = port2MacAndVlanEntries_[port->getID()];
  auto& classID2Count = port2ClassIDAndCount_[port->getID()];

  cfg::AclLookupClass classID;

  auto iter = macAndVlan2ClassIDAndRefCnt.find(std::make_pair(mac, vlanID));
  if (iter == macAndVlan2ClassIDAndRefCnt.end()) {
    if (noLookupClasses && !setDropClassID) {
      // For noLookupClasses (non-MH-NIC) only process if CLASS_DROP
      XLOG(DBG2) << "No lookup class and not set drop classID for vlanID "
                 << vlanID << " macAddress " << mac.toString();
      return;
    }

    classID = setDropClassID ? cfg::AclLookupClass::CLASS_DROP
                             : getClassIDwithMinimumNeighbors(classID2Count);
    macAndVlan2ClassIDAndRefCnt.insert(std::make_pair(
        std::make_pair(mac, vlanID),
        std::make_pair(classID, 1 /* initialize refCnt */)));
    if (!setDropClassID) {
      // classID2Count is maitanined to compute getClassIDwithMinimumNeighbors
      // CLASS_DROP is not part of that computation.
      classID2Count[classID]++;
    }
  } else {
    auto& [_classID, refCnt] = iter->second;
    XLOG(DBG2) << "ClassID and ref count from map" << static_cast<int>(_classID)
               << " " << refCnt << " for vlanID " << vlanID << " macAddress "
               << mac.toString();
    if (noLookupClasses && _classID != cfg::AclLookupClass::CLASS_DROP) {
      // For noLookupClasses (non-MH-NIC) only process if CLASS_DROP
      return;
    }

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
        folly::to<std::string>(
            "configure lookup classID: ",
            classID,
            " for entry " + newEntry->str()),
        std::move(updateMacClassIDFn));
  } else {
    auto updater = sw_->getNeighborUpdater();
    updater->updateEntryClassID(vlanID, newEntry->getIP(), classID);
  }
}

template <typename NeighborEntryT>
bool LookupClassUpdater::shouldProcessNeighborEntry(
    const std::shared_ptr<NeighborEntryT>& entry,
    bool added) const {
  /*
   * At this point in time, queue-per-host fix is needed (and thus
   * supported) for physical link only.
   */
  if (!(entry && entry->getPort().isPhysicalPort())) {
    return false;
  }
  /*
   * If newEntry already has classID populated
   * This can happen in three cases, only process third case:
   *  o Warmboot: prior to warmboot, neighbor entries may have a classID
   *    associated with them. updateStateObserverLocalCache() consumes this
   *    info to populate its local cache, so do nothing here.
   *  o Once LookupClassUpdater chooses classID for a neighbor, it
   *    schedules a state update. After the state update is run, all state
   *    observers are notified. At that time, LookupClassUpdater will
   *    receive a stateDelta that contains classID assigned to new neighbor
   *    entry. Since this will be side-effect of state update that
   *    LookupClassUpdater triggered, there is nothing to do here.
   *  o Racing scenarios: might happen when neighbor entry flaps.
   *    LookupClassUpdater assigns class id to mac entry and trigger mac
   *    entry state updateOrAdd(). However, this mac entry might be removed
   *    shortly by StaticL2ForNeighborUpdate. As a result, another new mac
   *    entry with class id is created when processing mac entry updateOrAdd().
   *    We still need to process this new mac entry to keep track of refCnt,
   *    although this entry woud be removed later.
   */
  if (entry->getClassID().has_value() &&
      (!inited_ /* case 1 */ || !added /*case 2*/)) {
    return false;
  }

  if (isNoHostRoute(entry)) {
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
  if (!shouldProcessNeighborEntry(addedEntry, true)) {
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
   * Entries with no Host Route do not have classID.
   */
  if (!removedEntry->getPort().isPhysicalPort() ||
      isNoHostRoute(removedEntry)) {
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
  if (!(shouldProcessNeighborEntry(newEntry, false) &&
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

bool LookupClassUpdater::portHasClassID(const std::shared_ptr<Port>& port) {
  return port && port->getLookupClassesToDistributeTrafficOn().size();
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

    using EntryType = typename detail::EntryType<AddrT>::type;
    for (auto iter : std::as_const(
             *VlanTableDeltaCallbackGenerator::getTable<AddrT>(vlan))) {
      EntryType entry = nullptr;
      entry = iter.second;
      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if (entry->getPort().isPhysicalPort() &&
          entry->getPort().phyPortID() == portID &&
          entry->getClassID().has_value() && !isNoHostRoute(entry)) {
        removeNeighborFromLocalCacheForEntry(entry, vlanID);
        if constexpr (std::is_same_v<AddrT, MacAddress>) {
          auto removeMacClassIDFn =
              [vlanID, entry](const std::shared_ptr<SwitchState>& state) {
                return MacTableUtils::removeClassIDForEntry(
                    state, vlanID, entry);
              };

          sw_->updateState(
              "remove classID: " + entry->str(), std::move(removeMacClassIDFn));
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

    using EntryType = typename detail::EntryType<AddrT>::type;
    for (auto iter : std::as_const(
             *VlanTableDeltaCallbackGenerator::getTable<AddrT>(vlan))) {
      EntryType entry = nullptr;
      entry = iter.second;
      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if (entry->getPort().isPhysicalPort() &&
          entry->getPort().phyPortID() == portID && !isNoHostRoute(entry)) {
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

  using EntryType = typename detail::EntryType<AddrT>::type;
  for (auto iter :
       std::as_const(*VlanTableDeltaCallbackGenerator::getTable<AddrT>(vlan))) {
    EntryType entry = nullptr;
    entry = iter.second;
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
  CHECK(!isNoHostRoute(removedEntry));

  auto portID = removedEntry->getPort().phyPortID();
  auto mac = removedEntry->getMac();

  XLOG(DBG2) << "Remove neighbor entry. Port: " << portID
             << " , mac: " << mac.toString() << ", vlan: " << vlanID;
  if (!isInited(portID)) {
    XLOG(ERR) << "LookupClass not even inited for port: " << portID
              << " , mac: " << mac.toString();
    return;
  }

  auto& macAndVlan2ClassIDAndRefCnt = port2MacAndVlanEntries_[portID];
  auto& [classID, refCnt] =
      macAndVlan2ClassIDAndRefCnt[std::make_pair(mac, vlanID)];
  auto& classID2Count = port2ClassIDAndCount_[portID];

  XLOG(DBG2) << "Reference count: " << (int)refCnt
             << " for classID: " << (int)classID;
  CHECK_GT(refCnt, 0);

  refCnt--;

  bool isDropClassID = removedEntry->getClassID().has_value() &&
      removedEntry->getClassID().value() == cfg::AclLookupClass::CLASS_DROP;

  if (refCnt == 0) {
    if (!isDropClassID) {
      // classID2Count is maitanined to compute getClassIDwithMinimumNeighbors
      // CLASS_DROP is not part of that computation.
      classID2Count[classID]--;
      CHECK_GE(classID2Count[classID], 0);
    }

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
  auto isDropClassID = (classID == cfg::AclLookupClass::CLASS_DROP);
  auto mac = newEntry->getMac();

  auto iter = macAndVlan2ClassIDAndRefCnt.find(std::make_pair(mac, vlanID));
  if (iter == macAndVlan2ClassIDAndRefCnt.end()) {
    macAndVlan2ClassIDAndRefCnt.insert(std::make_pair(
        std::make_pair(mac, vlanID),
        std::make_pair(classID, 1 /* initialize refCnt */)));

    if (!isDropClassID) {
      // classID2Count is maitanined to compute getClassIDwithMinimumNeighbors
      // CLASS_DROP is not part of that computation.
      classID2Count[classID]++;
    }

    XLOG(DBG2) << "Create neighbor entry for port: " << portID
               << " , mac: " << mac.toString() << " , vlan: " << vlanID
               << ", classId: " << static_cast<int>(classID);
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
  using EntryType = typename detail::EntryType<AddrT>::type;
  for (auto iter : std::as_const(
           *(VlanTableDeltaCallbackGenerator::getTable<AddrT>(vlan)))) {
    EntryType entry = nullptr;
    entry = iter.second;
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

  for (auto port : std::as_const(*switchState->getPorts())) {
    initPort(switchState, port.second);
    // THRIFT_COPY
    for (auto vlanMember : port.second->getVlans()) {
      auto vlanID = vlanMember.first;
      auto vlan = switchState->getVlans()->getVlanIf(vlanID);
      if (!vlan) {
        continue;
      }
      updateStateObserverLocalCacheHelper<folly::MacAddress>(vlan, port.second);
      updateStateObserverLocalCacheHelper<folly::IPAddressV6>(
          vlan, port.second);
      updateStateObserverLocalCacheHelper<folly::IPAddressV4>(
          vlan, port.second);
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::processBlockNeighborUpdatesHelper(
    const std::shared_ptr<SwitchState>& switchState,
    const std::shared_ptr<Vlan>& vlan,
    const AddrT& ipAddress) {
  using NeighborEntryT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpEntry,
      NdpEntry>;
  std::shared_ptr<NeighborEntryT> neighborEntry;

  neighborEntry =
      VlanTableDeltaCallbackGenerator::getTable<AddrT>(vlan)->getEntryIf(
          ipAddress);
  if (!neighborEntry) {
    return;
  }

  // Remove classID for neighbor entry and corresponding MAC
  removeClassIDForPortAndMac(switchState, vlan->getID(), neighborEntry);

  std::shared_ptr<MacEntry> macEntry = nullptr;
  if (neighborEntry->isReachable()) {
    macEntry = vlan->getMacTable()->getMacIf(neighborEntry->getMac());
    if (macEntry) {
      removeClassIDForPortAndMac(switchState, vlan->getID(), macEntry);
    }
  }

  // Re-Add classID for neighbor entry and corresponding MAC
  updateNeighborClassID(switchState, vlan->getID(), neighborEntry);

  if (macEntry) {
    updateNeighborClassID(switchState, vlan->getID(), macEntry);
  }
}

void LookupClassUpdater::processBlockNeighborUpdates(
    const StateDelta& stateDelta) {
  auto newState = stateDelta.newState();
  /*
   * Set of currently blocked neighbors: set A
   * New set of neighbors to block: set B
   *
   * A - B: set of neighbors to unblock
   * B - A: set of neighbors to block
   *
   * In this case:
   *  A is blockedNeighbors_
   *  B is newState->getSwitchSettings()->getBlockNeighbors()
   *
   *  Approach:
   *   - Compute symmetric difference (disjunctive union) i.e. set of elements
   *     which are in either A or B, but not in their intersection.
   *   - Update blockedNeighbors_ to newState's blockNeighbors.
   *   - For every neighbor in the symmetric difference, remove classID
   *     associated with it and re-add (update) classID.
   *
   *  Note:
   *   - macAndVlan2ClassIDAndRefCnt stores (MAC + VLAN) to (classID + refCnt)
   *     mapping. It does not store the neighbor IP address.
   *   - macAndVlan2ClassIDAndRefCnt typically has refCnt = 3 for any MAC. One
   *     for each of the below: Neighbor Entry, L2 Entry, Link local IP.
   *   - Neighbor blocked list comprises of IPs, no MACs.
   *
   *  Thus, to remove and re-add classID:
   *   - Remove classID associated with the neighbor entry.
   *   - Derive link local IP from the neighbor entry, and remove its classID.
   *   - Remove classID associated with corresponding mac entry.
   *   - Re-add classID for neighbor entry.
   *   - Re-add classID for link local IP.
   *   - Re-add classID for corresponding MAC entry.
   *
   *  We re-add classID for neighbor entry first (so its gets CLASS_DROP or
   *  queue-per-host classID as appropriate).
   *
   *  Neighbor Entry, Link local IP and MAC Entry all reference the same
   *  MacAndVlan2ClassIDAndRefCnt entry, and thus they inherit the classID
   *  chosen for neighbor entry (DROP or queue-per-host).
   *
   *  Since blockedNeighbors_ is updated before calling remove/re-add, if a
   *  neighbor is being blocked, remove will remove queue-per-host classID
   *  and re-add will associate CLASS_DROP.
   *  Similarly, if a negihbor is being unblocked, remove will remove
   *  CLASS_DROP and re-add will associate queue-per-hot classID.
   */

  std::set<std::pair<VlanID, folly::IPAddress>> newBlockedNeighbors;
  for (const auto& iter :
       *(newState->getSwitchSettings()->getBlockNeighbors())) {
    auto vlanID = VlanID(
        iter->cref<switch_state_tags::blockNeighborVlanID>()->toThrift());
    auto ipAddress = network::toIPAddress(
        iter->cref<switch_state_tags::blockNeighborIP>()->toThrift());
    newBlockedNeighbors.insert(std::make_pair(vlanID, ipAddress));
  }

  std::vector<std::pair<VlanID, folly::IPAddress>> toBeUpdatedBlockNeighbors;
  std::set_symmetric_difference(
      blockedNeighbors_.begin(),
      blockedNeighbors_.end(),
      newBlockedNeighbors.begin(),
      newBlockedNeighbors.end(),
      std::inserter(
          toBeUpdatedBlockNeighbors, toBeUpdatedBlockNeighbors.end()));

  blockedNeighbors_ = newBlockedNeighbors;
  for (const auto& [vlanID, ipAddress] : toBeUpdatedBlockNeighbors) {
    auto vlan = newState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    if (ipAddress.isV4()) {
      processBlockNeighborUpdatesHelper<folly::IPAddressV4>(
          newState, vlan, ipAddress.asV4());
    } else {
      processBlockNeighborUpdatesHelper<folly::IPAddressV6>(
          newState, vlan, ipAddress.asV6());
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::updateClassIDForEveryNeighborForMac(
    const std::shared_ptr<SwitchState>& switchState,
    const std::shared_ptr<Vlan>& vlan,
    folly::MacAddress macAddress) {
  for (auto iter :
       std::as_const(*VlanTableDeltaCallbackGenerator::getTable<AddrT>(vlan))) {
    auto neighborEntry = iter.second;
    if (!isNoHostRoute(neighborEntry) && neighborEntry->isReachable() &&
        neighborEntry->getMac() == macAddress) {
      updateNeighborClassID(switchState, vlan->getID(), neighborEntry);
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::removeClassIDForEveryNeighborForMac(
    const std::shared_ptr<SwitchState>& switchState,
    const std::shared_ptr<Vlan>& vlan,
    folly::MacAddress macAddress) {
  for (auto iter :
       std::as_const(*VlanTableDeltaCallbackGenerator::getTable<AddrT>(vlan))) {
    auto neighborEntry = iter.second;
    if (!isNoHostRoute(neighborEntry) && neighborEntry->isReachable() &&
        neighborEntry->getMac() == macAddress) {
      removeClassIDForPortAndMac(switchState, vlan->getID(), neighborEntry);
    }
  }
}

void LookupClassUpdater::processMacAddrsToBlockUpdates(
    const StateDelta& stateDelta) {
  auto newState = stateDelta.newState();
  /*
   * Set of currently blocked MACs: set A
   * New set of MACs to block: set B
   *
   * A - B: set of MACs to unblock
   * B - A: set of MACs to block
   *
   * In this case:
   *  A is macAddrsToBlock_
   *  B is newState->getSwitchSettings()->getMacAddrsToBlock()
   *
   * Approach:
   *   - Compute symmetric difference (disjunctive union) i.e. set of elements
   *     which are in either A or B, but not in their intersection.
   *   - Update macAddrsToBlock_ to newState's macAddrsToBlock
   *   - For every MAC in the symmetric difference:
   *   -    Remove classID associated with the MAC.
   *   -    Remove classID associated with every neighbor using the MAC.
   *   -    Re-add classID for the MAC.
   *   -    Re-add classID for every neighbor using the MAC.
   *
   *   Since macAddrsToBlock_ is updated before calling remove/re-add, if a
   *   MAC address is blocked, Remove will remove queue-per-host/NONE classID
   *   and Re-add will assoicate CLASS_DROP.
   *   Conversely, if a MAC addr is being unblocked, remove will remove
   *   CLASS_DROP and re-add will associate queue-per-host/NONE classID.
   */

  std::set<std::pair<VlanID, folly::MacAddress>> newMacAddrsToBlock;
  for (const auto& iter :
       *(newState->getSwitchSettings()->getMacAddrsToBlock())) {
    auto macStr =
        iter->cref<switch_state_tags::macAddrToBlockAddr>()->toThrift();
    auto vlanID = VlanID(
        iter->cref<switch_state_tags::macAddrToBlockVlanID>()->toThrift());
    auto macAddress = folly::MacAddress(macStr);

    newMacAddrsToBlock.insert(std::make_pair(vlanID, macAddress));
    XLOG(DBG2) << "New blocked mac address " << macStr;
  }

  std::vector<std::pair<VlanID, folly::MacAddress>> toBeUpdatedMacAddrsToBlock;
  std::set_symmetric_difference(
      macAddrsToBlock_.begin(),
      macAddrsToBlock_.end(),
      newMacAddrsToBlock.begin(),
      newMacAddrsToBlock.end(),
      std::inserter(
          toBeUpdatedMacAddrsToBlock, toBeUpdatedMacAddrsToBlock.end()));

  macAddrsToBlock_ = newMacAddrsToBlock;
  for (const auto& [vlanID, macAddress] : toBeUpdatedMacAddrsToBlock) {
    auto vlan = newState->getVlans()->getVlanIf(vlanID);
    XLOG(DBG2) << "Starting to Processing mac address "
               << macAddress.toString();
    if (!vlan) {
      XLOG(DBG2) << "No vlan found for vlanID " << vlanID << " macAddress "
                 << macAddress.toString()
                 << ". Skip processing the blocked mac entry.";
      continue;
    }

    auto macEntry = vlan->getMacTable()->getMacIf(macAddress);
    if (!macEntry) {
      XLOG(DBG2) << "No mac entry found for in mac table for vlanID " << vlanID
                 << " macAddress " << macAddress.toString()
                 << ". Skip processing the blocked mac entry.";
      continue;
    }

    removeClassIDForPortAndMac(newState, vlan->getID(), macEntry);
    removeClassIDForEveryNeighborForMac<folly::IPAddressV4>(
        newState, vlan, macAddress);
    removeClassIDForEveryNeighborForMac<folly::IPAddressV6>(
        newState, vlan, macAddress);

    updateNeighborClassID(newState, vlan->getID(), macEntry);
    updateClassIDForEveryNeighborForMac<folly::IPAddressV4>(
        newState, vlan, macAddress);
    updateClassIDForEveryNeighborForMac<folly::IPAddressV6>(
        newState, vlan, macAddress);
  }
}

void LookupClassUpdater::stateUpdated(const StateDelta& stateDelta) {
  if (!inited_) {
    updateStateObserverLocalCache(stateDelta.newState());
  }

  VlanTableDeltaCallbackGenerator::genCallbacks(stateDelta, *this);
  processPortUpdates(stateDelta);
  processBlockNeighborUpdates(stateDelta);
  processMacAddrsToBlockUpdates(stateDelta);
  inited_ = true;
}

} // namespace facebook::fboss
