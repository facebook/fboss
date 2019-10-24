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

  auto mac = removedEntry->getMac();
  auto& mac2ClassID = port2MacAndClassID_[portID];
  auto& classID2Count = port2ClassIDAndCount_[portID];
  auto classID = mac2ClassID[mac];

  mac2ClassID.erase(mac);
  classID2Count[classID]--;
  CHECK_GE(classID2Count[classID], 0);

  updater->updateEntryClassID(vlan, removedEntry->getIP());
}

void LookupClassUpdater::initClassID2Count(std::shared_ptr<Port> port) {
  auto& classID2Count = port2ClassIDAndCount_[port->getID()];

  /*
   * When first neighbor is discovered for a port, start maintaining counters
   * for the number of neigbhors associated with a classID for that port
   * (initialized to 0). When a new neighbor is discovered, these counters help
   * find out the least used classID that can then be used for the newly
   * discovered neighbor.
   */
  if (classID2Count.size() == 0) {
    for (auto classID : port->getLookupClassesToDistributeTrafficOn()) {
      classID2Count[classID] = 0;
    }
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

  initClassID2Count(port);

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

void LookupClassUpdater::stateUpdated(const StateDelta& stateDelta) {
  processNeighborUpdates<folly::IPAddressV6>(stateDelta);
  processNeighborUpdates<folly::IPAddressV4>(stateDelta);
}

} // namespace fboss
} // namespace facebook
