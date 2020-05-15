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
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

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
  }
  if (oldSwEntry->isPending() && !newSwEntry->isPending()) {
    addNeighbor(newSwEntry);
  }
  if (!oldSwEntry->isPending() && newSwEntry->isPending()) {
    removeNeighbor(oldSwEntry);
    // TODO(borisb): unresolve in next hop group...
  }
  if (!oldSwEntry->isPending() && !newSwEntry->isPending()) {
  }
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
  if (subscribersForNeighbor_.find(subscriberKey) !=
      subscribersForNeighbor_.end()) {
    throw FbossError(
        "Attempted to add duplicate neighbor: ", swEntry->getIP().str());
  }

  managerTable_->fdbManager().addFdbEntry(
      swEntry->getPort().phyPortID(), swEntry->getIntfID(), swEntry->getMac());

  auto subscriber = std::make_shared<SubscriberForNeighbor>(
      swEntry->getIntfID(), swEntry->getIP(), swEntry->getMac());

  SaiObjectEventPublisher::getInstance()
      ->get<SaiRouterInterfaceTraits>()
      .subscribe(subscriber);
  SaiObjectEventPublisher::getInstance()->get<SaiFdbTraits>().subscribe(
      subscriber);
  subscribersForNeighbor_.emplace(subscriberKey, std::move(subscriber));
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
  if (subscribersForNeighbor_.find(subscriberKey) ==
      subscribersForNeighbor_.end()) {
    throw FbossError(
        "Attempted to remove non-existent neighbor: ", swEntry->getIP());
  }
  managerTable_->fdbManager().removeFdbEntry(
      swEntry->getIntfID(), swEntry->getMac());
  subscribersForNeighbor_.erase(subscriberKey);
}

void SaiNeighborManager::processNeighborDelta(
    const StateDelta& delta,
    std::mutex& lock) {
  for (const auto& vlanDelta : delta.getVlansDelta()) {
    auto processChanged =
        [this, &lock](const auto& oldNeighbor, const auto& newNeighbor) {
          std::lock_guard g{lock};
          changeNeighbor(oldNeighbor, newNeighbor);
        };
    auto processAdded = [this, &lock](const auto& newNeighbor) {
      std::lock_guard g{lock};
      addNeighbor(newNeighbor);
    };
    auto processRemoved = [this, &lock](const auto& oldNeighbor) {
      std::lock_guard g{lock};
      removeNeighbor(oldNeighbor);
    };
    DeltaFunctions::forEachChanged(
        vlanDelta.getArpDelta(), processChanged, processAdded, processRemoved);
    DeltaFunctions::forEachChanged(
        vlanDelta.getNdpDelta(), processChanged, processAdded, processRemoved);
  }
}

void SaiNeighborManager::clear() {
  subscribersForNeighbor_.clear();
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
  auto itr = subscribersForNeighbor_.find(saiEntry);
  if (itr == subscribersForNeighbor_.end()) {
    return nullptr;
  }
  auto subscriber = itr->second.get();
  if (!subscriber) {
    XLOG(FATAL) << "Invalid null neighbor for ip: " << saiEntry.ip().str();
  }
  return subscriber->getHandle();
}

void SubscriberForNeighbor::createObject(PublisherObjects objects) {
  auto interface = std::get<RouterInterfaceWeakPtr>(objects).lock();
  auto fdbEntry = std::get<FdbWeakptr>(objects).lock();
  auto adapterHostKey = SaiNeighborTraits::NeighborEntry(
      fdbEntry->adapterHostKey().switchId(), interface->adapterKey(), ip_);

  auto createAttributes =
      SaiNeighborTraits::CreateAttributes{fdbEntry->adapterHostKey().mac()};
  this->setObject(adapterHostKey, createAttributes);
  handle_->neighbor = getSaiObject();
  handle_->fdbEntry = fdbEntry.get();
}

void SubscriberForNeighbor::removeObject(size_t, PublisherObjects) {
  this->resetObject();
  handle_->neighbor = nullptr;
  handle_->fdbEntry = nullptr;
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
