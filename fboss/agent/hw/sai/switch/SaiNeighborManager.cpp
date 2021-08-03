/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NdpEntry.h"

namespace facebook::fboss {

SaiNeighborManager::SaiNeighborManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

// Helper function to create a SAI NeighborEntry from an FBOSS SwitchState
// NeighborEntry (e.g., NeighborEntry<IPAddressV6, NDPTable>)
template <typename NeighborEntryT>
SaiNeighborTraits::NeighborEntry SaiNeighborManager::saiEntryFromSwEntry(
    const std::shared_ptr<NeighborEntryT>& swEntry) {
  folly::IPAddress ip(swEntry->getIP());
  SaiRouterInterfaceHandle* routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          swEntry->getIntfID());
  if (!routerInterfaceHandle) {
    throw FbossError(
        "Failed to create sai_neighbor_entry from NeighborEntry. "
        "No SaiRouterInterface for InterfaceID: ",
        swEntry->getIntfID());
  }
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  return SaiNeighborTraits::NeighborEntry(
      switchId, routerInterfaceHandle->routerInterface->adapterKey(), ip);
}

template <typename NeighborEntryT>
void SaiNeighborManager::changeNeighbor(
    const std::shared_ptr<NeighborEntryT>& oldSwEntry,
    const std::shared_ptr<NeighborEntryT>& newSwEntry) {
  if (oldSwEntry->isPending() && newSwEntry->isPending()) {
    // We don't maintain pending entries so nothing to do here
  }
  if (newSwEntry->getIP().version() == 6 && newSwEntry->getIP().isLinkLocal()) {
    /* TODO: investigate and fix adding link local neighbors */
    XLOG(INFO) << "skip modify of link local neighbor " << newSwEntry->getIP();
    return;
  }
  if (oldSwEntry->isPending() && !newSwEntry->isPending()) {
    addNeighbor(newSwEntry);
  }
  if (!oldSwEntry->isPending() && newSwEntry->isPending()) {
    removeNeighbor(oldSwEntry);
  }
  if (!oldSwEntry->isPending() && !newSwEntry->isPending()) {
    if (*oldSwEntry != *newSwEntry) {
      removeNeighbor(oldSwEntry);
      addNeighbor(newSwEntry);
    } else {
      /* attempt to resolve next hops if not already resolved, if already
       * resolved, it would be no-op */
      auto iter = managedNeighbors_.find(saiEntryFromSwEntry(newSwEntry));
      CHECK(iter != managedNeighbors_.end());
      iter->second->notifySubscribers();
    }
  }

  XLOG(DBG2) << "Change Neighbor:: old Neighbor: " << oldSwEntry->str()
             << " new Neighbor: " << newSwEntry->str();
}

template <typename NeighborEntryT>
void SaiNeighborManager::addNeighbor(
    const std::shared_ptr<NeighborEntryT>& swEntry) {
  if (swEntry->isPending()) {
    XLOG(INFO) << "skip adding unresolved neighbor " << swEntry->getIP();
    return;
  }
  if (swEntry->getIP().version() == 6 && swEntry->getIP().isLinkLocal()) {
    /* TODO: investigate and fix adding link local neighbors */
    XLOG(INFO) << "skip adding link local neighbor " << swEntry->getIP();
    return;
  }
  XLOG(INFO) << "addNeighbor " << swEntry->getIP();
  auto subscriberKey = saiEntryFromSwEntry(swEntry);
  if (managedNeighbors_.find(subscriberKey) != managedNeighbors_.end()) {
    throw FbossError(
        "Attempted to add duplicate neighbor: ", swEntry->getIP().str());
  }

  SaiPortDescriptor saiPortDesc = swEntry->getPort().isPhysicalPort()
      ? SaiPortDescriptor(swEntry->getPort().phyPortID())
      : SaiPortDescriptor(swEntry->getPort().aggPortID());

  std::optional<sai_uint32_t> metadata;
  if (swEntry->getClassID()) {
    metadata = static_cast<sai_uint32_t>(swEntry->getClassID().value());
  }

  auto subscriber = std::make_shared<ManagedNeighbor>(
      this,
      saiPortDesc,
      swEntry->getIntfID(),
      swEntry->getIP(),
      swEntry->getMac(),
      metadata);

  SaiObjectEventPublisher::getInstance()
      ->get<SaiVlanRouterInterfaceTraits>()
      .subscribe(subscriber);
  SaiObjectEventPublisher::getInstance()->get<SaiFdbTraits>().subscribe(
      subscriber);
  managedNeighbors_.emplace(subscriberKey, std::move(subscriber));
  XLOG(DBG2) << "Add Neighbor: create ManagedNeighbor" << swEntry->str();
}

template <typename NeighborEntryT>
void SaiNeighborManager::removeNeighbor(
    const std::shared_ptr<NeighborEntryT>& swEntry) {
  if (swEntry->getIP().version() == 6 && swEntry->getIP().isLinkLocal()) {
    /* TODO: investigate and fix adding link local neighbors */
    XLOG(INFO) << "skip link local neighbor " << swEntry->getIP();
    return;
  }
  if (swEntry->isPending()) {
    XLOG(INFO) << "skip removing unresolved neighbor " << swEntry->getIP();
    return;
  }
  XLOG(INFO) << "removeNeighbor " << swEntry->getIP();
  auto subscriberKey = saiEntryFromSwEntry(swEntry);
  if (managedNeighbors_.find(subscriberKey) == managedNeighbors_.end()) {
    throw FbossError(
        "Attempted to remove non-existent neighbor: ", swEntry->getIP());
  }
  managedNeighbors_.erase(subscriberKey);
  XLOG(DBG2) << "Remove Neighbor: " << swEntry->str();
}

void SaiNeighborManager::clear() {
  managedNeighbors_.clear();
}

std::shared_ptr<SaiNeighbor> SaiNeighborManager::createSaiObject(
    const SaiNeighborTraits::AdapterHostKey& key,
    const SaiNeighborTraits::CreateAttributes& attributes,
    bool notify) {
  auto& store = saiStore_->get<SaiNeighborTraits>();
  return store.setObject(key, attributes, notify);
}

const SaiNeighborHandle* SaiNeighborManager::getNeighborHandle(
    const SaiNeighborTraits::NeighborEntry& saiEntry) const {
  return getNeighborHandleImpl(saiEntry);
}
SaiNeighborHandle* SaiNeighborManager::getNeighborHandle(
    const SaiNeighborTraits::NeighborEntry& saiEntry) {
  return getNeighborHandleImpl(saiEntry);
}
SaiNeighborHandle* SaiNeighborManager::getNeighborHandleImpl(
    const SaiNeighborTraits::NeighborEntry& saiEntry) const {
  auto itr = managedNeighbors_.find(saiEntry);
  if (itr == managedNeighbors_.end()) {
    return nullptr;
  }
  auto subscriber = itr->second.get();
  if (!subscriber) {
    XLOG(FATAL) << "Invalid null neighbor for ip: " << saiEntry.ip().str();
  }
  return subscriber->getHandle();
}

bool SaiNeighborManager::isLinkUp(SaiPortDescriptor port) {
  if (port.isPhysicalPort()) {
    auto portHandle =
        managerTable_->portManager().getPortHandle(port.phyPortID());
    auto portOperStatus = SaiApiTable::getInstance()->portApi().getAttribute(
        portHandle->port->adapterKey(),
        SaiPortTraits::Attributes::OperStatus{});
    return (portOperStatus == SAI_PORT_OPER_STATUS_UP);
  }
  return managerTable_->lagManager().isMinimumLinkMet(port.aggPortID());
}

void ManagedNeighbor::createObject(PublisherObjects objects) {
  auto interface = std::get<RouterInterfaceWeakPtr>(objects).lock();
  auto fdbEntry = std::get<FdbWeakptr>(objects).lock();
  auto adapterHostKey = SaiNeighborTraits::NeighborEntry(
      fdbEntry->adapterHostKey().switchId(), interface->adapterKey(), ip_);

  // warm boot replay may expand ecmp even if link is down. this may happen
  // because warm boot was triggered before sw switch could process link down
  // event and enqueue neighbor delete. if this happens then on warm boot
  // ecmp will be expanded to have members with link down. to prevent this
  // check if link is up or if trunk has minimum links met before notifying
  // next hops.
  bool resolveNexthop = manager_->isLinkUp(port_);

  auto createAttributes = SaiNeighborTraits::CreateAttributes{
      fdbEntry->adapterHostKey().mac(), metadata_};
  auto object = manager_->createSaiObject(
      adapterHostKey, createAttributes, resolveNexthop);
  this->setObject(object);
  handle_->neighbor = getSaiObject();
  handle_->fdbEntry = fdbEntry.get();

  XLOG(DBG2) << "ManagedNeigbhor::createObject: " << toString()
             << " resolveNextHop: " << static_cast<int>(resolveNexthop);
}

void ManagedNeighbor::removeObject(size_t, PublisherObjects) {
  XLOG(DBG2) << "ManagedNeigbhor::removeObject: " << toString();

  this->resetObject();
  handle_->neighbor = nullptr;
  handle_->fdbEntry = nullptr;
}

void ManagedNeighbor::notifySubscribers() const {
  auto neighbor = this->getObject();
  if (!neighbor || !manager_->isLinkUp(port_)) {
    return;
  }
  neighbor->notifyAfterCreate(neighbor);
}

std::string ManagedNeighbor::toString() const {
  auto metadataStr =
      metadata_.has_value() ? std::to_string(metadata_.value()) : "none";
  auto neighborStr = handle_->neighbor
      ? handle_->neighbor->adapterKey().toString()
      : "NeighborEntry: none";
  auto fdbEntryStr = handle_->fdbEntry
      ? handle_->fdbEntry->adapterKey().toString()
      : "FdbEntry: none";

  return folly::to<std::string>(
      "ip: ",
      ip_.str(),
      port_.str(),
      " metadata: ",
      metadataStr,
      " ",
      neighborStr,
      " ",
      fdbEntryStr);
}

template SaiNeighborTraits::NeighborEntry
SaiNeighborManager::saiEntryFromSwEntry<NdpEntry>(
    const std::shared_ptr<NdpEntry>& swEntry);
template SaiNeighborTraits::NeighborEntry
SaiNeighborManager::saiEntryFromSwEntry<ArpEntry>(
    const std::shared_ptr<ArpEntry>& swEntry);

template void SaiNeighborManager::changeNeighbor<NdpEntry>(
    const std::shared_ptr<NdpEntry>& oldSwEntry,
    const std::shared_ptr<NdpEntry>& newSwEntry);
template void SaiNeighborManager::changeNeighbor<ArpEntry>(
    const std::shared_ptr<ArpEntry>& oldSwEntry,
    const std::shared_ptr<ArpEntry>& newSwEntry);

template void SaiNeighborManager::addNeighbor<NdpEntry>(
    const std::shared_ptr<NdpEntry>& swEntry);
template void SaiNeighborManager::addNeighbor<ArpEntry>(
    const std::shared_ptr<ArpEntry>& swEntry);

template void SaiNeighborManager::removeNeighbor<NdpEntry>(
    const std::shared_ptr<NdpEntry>& swEntry);
template void SaiNeighborManager::removeNeighbor<ArpEntry>(
    const std::shared_ptr<ArpEntry>& swEntry);

} // namespace facebook::fboss
