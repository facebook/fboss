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
    VlanID vlan,
    const NeighborEntryT* removedEntry) {
  auto portID = removedEntry->getPort().phyPortID();
  // TODO(skhare) consume state from stateDelta instead
  auto port = sw_->getState()->getPorts()->getPortIf(portID);
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
    VlanID vlan,
    const NeighborEntryT* newEntry) {
  auto portID = newEntry->getPort().phyPortID();
  // TODO(skhare) consume state from stateDelta instead
  auto port = sw_->getState()->getPorts()->getPortIf(portID);
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
    VlanID vlan,
    const NeighborEntryT* addedEntry) {
  CHECK(addedEntry);

  if (addedEntry->isReachable()) {
    updateNeighborClassID(vlan, addedEntry);
  }
}

template <typename NeighborEntryT>
void LookupClassUpdater::processNeighborRemoved(
    VlanID vlan,
    const NeighborEntryT* removedEntry) {
  CHECK(removedEntry);
  removeClassIDForPortAndMac(vlan, removedEntry);
}

template <typename NeighborEntryT>
void LookupClassUpdater::processNeighborChanged(
    VlanID vlan,
    const NeighborEntryT* oldEntry,
    const NeighborEntryT* newEntry) {
  CHECK(oldEntry);
  CHECK(newEntry);
  CHECK_EQ(oldEntry->getIP(), newEntry->getIP());

  if (!oldEntry->isReachable() && newEntry->isReachable()) {
    updateNeighborClassID(vlan, newEntry);
  }

  if (oldEntry->isReachable() && !newEntry->isReachable()) {
    removeClassIDForPortAndMac(vlan, oldEntry);
  }

  /*
   * If the neighbor remains reachable as before but resolves to different
   * MAC/port, remove the classID for oldEntry and add classID for newEntry.
   */
  if ((oldEntry->isReachable() && newEntry->isReachable()) &&
      (oldEntry->getPort().phyPortID() != newEntry->getPort().phyPortID() ||
       oldEntry->getMac() != newEntry->getMac())) {
    removeClassIDForPortAndMac(vlan, oldEntry);
    updateNeighborClassID(vlan, newEntry);
  }
}

template <typename NeighborEntryT>
void LookupClassUpdater::updateStateObserverLocalCache(
    const NeighborEntryT* newEntry) {
  auto portID = newEntry->getPort().phyPortID();
  // TODO(skhare) consume state from stateDelta instead
  auto port = sw_->getState()->getPorts()->getPortIf(portID);
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
       * If newEntry already has classID populated (e.g. post warmboot), then
       * the classID would be already programmed in the ASIC. Just use the
       * classID to populate this state observer's local cached value.
       */
      if (newEntry && newEntry->getClassID().hasValue()) {
        updateStateObserverLocalCache(newEntry);
        continue;
      }

      if (!oldEntry) {
        processNeighborAdded(vlan, newEntry);
      } else if (!newEntry) {
        processNeighborRemoved(vlan, oldEntry);
      } else {
        processNeighborChanged(vlan, oldEntry, newEntry);
      }
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::clearClassIdsForResolvedNeighbors(
    const StateDelta& stateDelta,
    PortID portID) {
  using NeighborTableT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  auto updater = sw_->getNeighborUpdater();
  auto newState = stateDelta.newState();
  auto port = newState->getPorts()->getPortIf(portID);
  for (auto vlanMember : port->getVlans()) {
    auto vlanID = vlanMember.first;
    // TODO(skhare) consume state from stateDelta instead
    auto vlan = sw_->getState()->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    for (const auto& entry :
         *(vlan->template getNeighborTable<NeighborTableT>())) {
      if (entry->getPort().phyPortID() == portID &&
          entry->getClassID().hasValue()) {
        removeNeighborFromLocalCache(entry.get());
        updater->updateEntryClassID(vlanID, entry.get()->getIP());
      }
    }
  }
}

template <typename AddrT>
void LookupClassUpdater::repopulateClassIdsForResolvedNeighbors(PortID portID) {
  using NeighborTableT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  // TODO(skhare) consume state from stateDelta instead
  auto port = sw_->getState()->getPorts()->getPortIf(portID);
  for (auto vlanMember : port->getVlans()) {
    auto vlanID = vlanMember.first;
    // TODO(skhare) consume state from stateDelta instead
    auto vlan = sw_->getState()->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    for (const auto& entry :
         *(vlan->template getNeighborTable<NeighborTableT>())) {
      if (entry->getPort().phyPortID() == portID) {
        updateNeighborClassID(vlanID, entry.get());
      }
    }
  }
}

void LookupClassUpdater::processPortAdded(
    const StateDelta& /* unused */,
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
    const StateDelta& stateDelta,
    std::shared_ptr<Port> removedPort) {
  CHECK(removedPort);
  auto newState = stateDelta.newState();
  auto portID = removedPort->getID();

  /*
   * The port that is being removed should not be next hop for any neighbor.
   * in the new switch state.
   */
  for (auto vlanMember : removedPort->getVlans()) {
    auto vlanID = vlanMember.first;
    auto vlan = newState->getVlans()->getVlanIf(vlanID);
    if (!vlan) {
      continue;
    }

    for (const auto& entry : *(vlan->template getNeighborTable<ArpTable>())) {
      CHECK(entry->getPort().phyPortID() != portID);
    }

    for (const auto& entry : *(vlan->template getNeighborTable<NdpTable>())) {
      CHECK(entry->getPort().phyPortID() != portID);
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
      stateDelta, newPort->getID());
  clearClassIdsForResolvedNeighbors<folly::IPAddressV4>(
      stateDelta, newPort->getID());

  initPort(newPort);

  repopulateClassIdsForResolvedNeighbors<folly::IPAddressV6>(newPort->getID());
  repopulateClassIdsForResolvedNeighbors<folly::IPAddressV4>(newPort->getID());
}

void LookupClassUpdater::processPortUpdates(const StateDelta& stateDelta) {
  for (const auto& delta : stateDelta.getPortsDelta()) {
    auto oldPort = delta.getOld();
    auto newPort = delta.getNew();

    if (!oldPort && newPort) {
      processPortAdded(stateDelta, newPort);
    } else if (oldPort && !newPort) {
      processPortRemoved(stateDelta, oldPort);
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
