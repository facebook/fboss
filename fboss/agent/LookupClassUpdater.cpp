// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/LookupClassUpdater.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::isEmpty;

namespace facebook {
namespace fboss {

cfg::AclLookupClass LookupClassUpdater::getClassIDwithMinimumNeighbors(
    ClassID2Count classID2Count) const {
  auto minItr = std::min_element(
      classID2Count.begin(),
      classID2Count.end(),
      [=](const auto& l, const auto& r) { return l.second < r.second; });

  CHECK(minItr != classID2Count.end());
  return minItr->first;
}

template <typename NeighborEntryT>
void LookupClassUpdater::removeNeighborFromLocalCache(
    const NeighborEntryT* removedEntry) {
  CHECK(removedEntry->getPort().isPhysicalPort());

  auto portID = removedEntry->getPort().phyPortID();
  auto mac = removedEntry->getMac();
  auto& mac2ClassID = port2MacAndClassID_[portID];
  auto& classID2Count = port2ClassIDAndCount_[portID];
  auto classID = mac2ClassID[mac];

  mac2ClassID.erase(mac);
  classID2Count[classID]--;
  CHECK_GE(classID2Count[classID], 0);
}

template <typename NeighborEntryT>
void LookupClassUpdater::removeClassIDForPortAndMac(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const NeighborEntryT* removedEntry) {
  CHECK(removedEntry->getPort().isPhysicalPort());

  auto portID = removedEntry->getPort().phyPortID();
  auto port = switchState->getPorts()->getPortIf(portID);
  auto updater = sw_->getNeighborUpdater();

  if (!port || port->getLookupClassesToDistributeTrafficOn().size() == 0) {
    /*
     * Only downlink ports of Yosemite RSW will have lookupClasses
     * configured. For all other ports, control returns here.
     */
    return;
  }

  removeNeighborFromLocalCache(removedEntry);
  XLOG(DBG2) << "Updating Qos Policy for Neighbor:: port: " << port->getID()
             << " name: " << port->getName()
             << " MAC: " << folly::to<std::string>(removedEntry->getMac())
             << " IP: " << folly::to<std::string>(removedEntry->getIP())
             << " classID: None";

  updater->updateEntryClassID(vlan, removedEntry->getIP());
}

void LookupClassUpdater::initPort(std::shared_ptr<Port> port) {
  auto& mac2ClassID = port2MacAndClassID_[port->getID()];
  auto& classID2Count = port2ClassIDAndCount_[port->getID()];

  mac2ClassID.clear();
  classID2Count.clear();

  for (auto classID : port->getLookupClassesToDistributeTrafficOn()) {
    classID2Count[classID] = 0;
  }
}

template <typename NeighborEntryT>
void LookupClassUpdater::updateNeighborClassID(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const NeighborEntryT* newEntry) {
  CHECK(newEntry->getPort().isPhysicalPort());

  auto portID = newEntry->getPort().phyPortID();
  auto port = switchState->getPorts()->getPortIf(portID);
  auto updater = sw_->getNeighborUpdater();

  if (!port || port->getLookupClassesToDistributeTrafficOn().size() == 0) {
    /*
     * Only downlink ports of Yosemite RSW will have lookupClasses
     * configured. For all other ports, control returns here.
     */
    return;
  }

  auto mac = newEntry->getMac();
  auto& mac2ClassID = port2MacAndClassID_[port->getID()];
  auto& classID2Count = port2ClassIDAndCount_[port->getID()];

  cfg::AclLookupClass classID;
  auto iter = mac2ClassID.find(mac);
  if (iter == mac2ClassID.end()) {
    classID = getClassIDwithMinimumNeighbors(classID2Count);
    mac2ClassID.insert(std::make_pair(mac, classID));
    classID2Count[classID]++;
  } else {
    classID = iter->second;
  }

  XLOG(DBG2) << "Updating Qos Policy for Neighbor:: port: " << port->getID()
             << " name: " << port->getName()
             << " MAC: " << folly::to<std::string>(newEntry->getMac())
             << " IP: " << folly::to<std::string>(newEntry->getIP())
             << " classID: " << static_cast<int>(classID);

  updater->updateEntryClassID(vlan, newEntry->getIP(), classID);
}

template <typename NeighborEntryT>
void LookupClassUpdater::processNeighborAdded(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const NeighborEntryT* addedEntry) {
  CHECK(addedEntry);
  CHECK(addedEntry->getPort().isPhysicalPort());

  if (addedEntry->isReachable()) {
    updateNeighborClassID(switchState, vlan, addedEntry);
  }
}

template <typename NeighborEntryT>
void LookupClassUpdater::processNeighborRemoved(
    const std::shared_ptr<SwitchState>& switchState,
    VlanID vlan,
    const NeighborEntryT* removedEntry) {
  CHECK(removedEntry);
  CHECK(removedEntry->getPort().isPhysicalPort());

  removeClassIDForPortAndMac(switchState, vlan, removedEntry);
}

template <typename NeighborEntryT>
void LookupClassUpdater::processNeighborChanged(
    const StateDelta& stateDelta,
    VlanID vlan,
    const NeighborEntryT* oldEntry,
    const NeighborEntryT* newEntry) {
  CHECK(oldEntry);
  CHECK(newEntry);
  CHECK_EQ(oldEntry->getIP(), newEntry->getIP());
  CHECK(oldEntry->getPort().isPhysicalPort());
  CHECK(newEntry->getPort().isPhysicalPort());

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

template <typename NeighborEntryT>
void LookupClassUpdater::updateStateObserverLocalCache(
    const std::shared_ptr<SwitchState>& switchState,
    const NeighborEntryT* newEntry) {
  CHECK(newEntry->getPort().isPhysicalPort());

  auto portID = newEntry->getPort().phyPortID();
  auto port = switchState->getPorts()->getPortIf(portID);
  auto& mac2ClassID = port2MacAndClassID_[portID];
  auto& classID2Count = port2ClassIDAndCount_[portID];
  auto classID = newEntry->getClassID().value();
  auto mac = newEntry->getMac();

  auto iter = mac2ClassID.find(mac);
  if (iter == mac2ClassID.end()) {
    mac2ClassID.insert(std::make_pair(mac, classID));
    classID2Count[classID]++;
  } else {
    iter->second = classID;
  }
}

template <typename AddrT>
void LookupClassUpdater::processNeighborUpdates(const StateDelta& stateDelta) {
  using NeighborTableT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
    auto newVlan = vlanDelta.getNew();
    if (!newVlan) {
      continue;
    }
    auto vlan = newVlan->getID();

    for (const auto& delta :
         vlanDelta.template getNeighborDelta<NeighborTableT>()) {
      const auto* oldEntry = delta.getOld().get();
      const auto* newEntry = delta.getNew().get();

      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if ((oldEntry && !oldEntry->getPort().isPhysicalPort()) ||
          (newEntry && !newEntry->getPort().isPhysicalPort())) {
        continue;
      }

      /*
       * If newEntry already has classID populated (e.g. post warmboot), then
       * the classID would be already programmed in the ASIC. Just use the
       * classID to populate this state observer's local cached value.
       */
      if (newEntry && newEntry->getClassID().has_value()) {
        updateStateObserverLocalCache(stateDelta.newState(), newEntry);
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
  using NeighborTableT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  auto updater = sw_->getNeighborUpdater();
  auto port = switchState->getPorts()->getPortIf(portID);
  for (auto vlanMember : port->getVlans()) {
    auto vlanID = vlanMember.first;
    auto vlan = switchState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    for (const auto& entry :
         *(vlan->template getNeighborTable<NeighborTableT>())) {
      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if (entry->getPort().isPhysicalPort() &&
          entry->getPort().phyPortID() == portID &&
          entry->getClassID().has_value()) {
        removeNeighborFromLocalCache(entry.get());
        updater->updateEntryClassID(vlanID, entry.get()->getIP());
      }
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::repopulateClassIdsForResolvedNeighbors(
    const std::shared_ptr<SwitchState>& switchState,
    PortID portID) {
  using NeighborTableT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  auto port = switchState->getPorts()->getPortIf(portID);
  for (auto vlanMember : port->getVlans()) {
    auto vlanID = vlanMember.first;
    auto vlan = switchState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    for (const auto& entry :
         *(vlan->template getNeighborTable<NeighborTableT>())) {
      /*
       * At this point in time, queue-per-host fix is needed (and thus
       * supported) for physical link only.
       */
      if (entry->getPort().isPhysicalPort() &&
          entry->getPort().phyPortID() == portID) {
        updateNeighborClassID(switchState, vlanID, entry.get());
      }
    }
  }
}

void LookupClassUpdater::processPortAdded(
    const std::shared_ptr<SwitchState>& /* unused */,
    std::shared_ptr<Port> addedPort) {
  CHECK(addedPort);
  if (addedPort->getLookupClassesToDistributeTrafficOn().size() == 0) {
    /*
     * Only downlink ports of Yosemite RSW will have lookupClasses
     * configured. For all other ports, control returns here.
     */
    return;
  }

  initPort(addedPort);
}

void LookupClassUpdater::processPortRemoved(
    const std::shared_ptr<SwitchState>& switchState,
    std::shared_ptr<Port> removedPort) {
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

    /*
     * At this point in time, queue-per-host fix is needed (and thus
     * supported) for physical link only.
     */

    for (const auto& entry : *(vlan->template getNeighborTable<ArpTable>())) {
      if (entry->getPort().isPhysicalPort()) {
        CHECK(entry->getPort().phyPortID() != portID);
      }
    }

    for (const auto& entry : *(vlan->template getNeighborTable<NdpTable>())) {
      if (entry->getPort().isPhysicalPort()) {
        CHECK(entry->getPort().phyPortID() != portID);
      }
    }
  }

  port2MacAndClassID_.erase(portID);
  port2ClassIDAndCount_.erase(portID);
}

void LookupClassUpdater::processPortChanged(
    const StateDelta& stateDelta,
    std::shared_ptr<Port> oldPort,
    std::shared_ptr<Port> newPort) {
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

  if (oldLookupClasses == newLookupClasses) {
    return;
  }

  clearClassIdsForResolvedNeighbors<folly::IPAddressV6>(
      stateDelta.oldState(), newPort->getID());
  clearClassIdsForResolvedNeighbors<folly::IPAddressV4>(
      stateDelta.oldState(), newPort->getID());

  initPort(newPort);

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

void LookupClassUpdater::stateUpdated(const StateDelta& stateDelta) {
  processNeighborUpdates<folly::IPAddressV6>(stateDelta);
  processNeighborUpdates<folly::IPAddressV4>(stateDelta);

  processPortUpdates(stateDelta);
}

} // namespace fboss
} // namespace facebook
