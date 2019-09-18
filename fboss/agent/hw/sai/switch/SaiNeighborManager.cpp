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

namespace facebook {
namespace fboss {

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
    removeNeighbor(oldSwEntry);
    addNeighbor(newSwEntry);
  }
  if (!oldSwEntry->isPending() && newSwEntry->isPending()) {
    removeNeighbor(oldSwEntry);
    addNeighbor(newSwEntry);
    // TODO(borisb): unresolve in next hop group...
  }
  if (!oldSwEntry->isPending() && !newSwEntry->isPending()) {
  }
}

template <typename NeighborEntryT>
void SaiNeighborManager::addNeighbor(
    const std::shared_ptr<NeighborEntryT>& swEntry) {
  // Handle pending()
  XLOG(INFO) << "addNeighbor " << swEntry->getIP();
  auto saiEntry = saiEntryFromSwEntry(swEntry);
  auto existingSaiNeighbor = getNeighborHandle(saiEntry);
  if (existingSaiNeighbor) {
    throw FbossError(
        "Attempted to add duplicate neighbor: ", swEntry->getIP().str());
  }
  if (swEntry->isPending()) {
    XLOG(INFO) << "add unresolved neighbor";
    unresolvedNeighbors_.insert(saiEntry);
  } else {
    XLOG(INFO) << "add resolved neighbor";
    if (swEntry->getIP().isLinkLocal()) {
      /* TODO: investigate and fix adding link local neighbors */
      XLOG(INFO) << "skip link local neighbor " << swEntry->getIP();
      return;
    }
    SaiNeighborTraits::CreateAttributes attributes{swEntry->getMac()};
    auto& store = SaiStore::getInstance()->get<SaiNeighborTraits>();
    /*
     * program fdb entry before creating neighbor, neighbor requires fdb entry
     */
    auto fdbEntry = managerTable_->fdbManager().addFdbEntry(
        swEntry->getIntfID(), swEntry->getMac(), swEntry->getPort());
    auto neighbor = store.setObject(saiEntry, attributes);
    /* add next hop to discovered neighbor over */
    auto nextHop = managerTable_->nextHopManager().addNextHop(
        saiEntry.routerInterfaceId(), swEntry->getIP());
    auto neighborHandle = std::make_unique<SaiNeighborHandle>();
    neighborHandle->neighbor = neighbor;
    neighborHandle->fdbEntry = fdbEntry;
    neighborHandle->nextHop = nextHop;
    handles_.emplace(saiEntry, std::move(neighborHandle));
    managerTable_->nextHopGroupManager().handleResolvedNeighbor(
        saiEntry, nextHop->adapterKey());
  }
}

template <typename NeighborEntryT>
void SaiNeighborManager::removeNeighbor(
    const std::shared_ptr<NeighborEntryT>& swEntry) {
  XLOG(INFO) << "removeNeighbor " << swEntry->getIP();
  auto saiEntry = saiEntryFromSwEntry(swEntry);
  auto neighborHandle = getNeighborHandle(saiEntry);
  if (neighborHandle) {
    managerTable_->nextHopGroupManager().handleUnresolvedNeighbor(
        saiEntry, neighborHandle->nextHop->adapterKey());
    handles_.erase(saiEntry);
  } else {
    auto count = unresolvedNeighbors_.erase(saiEntry);
    if (count == 0) {
      throw FbossError(
          "Attempted to remove non-existent neighbor: ",
          swEntry->getIP().str());
    }
  }
}

void SaiNeighborManager::processNeighborDelta(const StateDelta& delta) {
  for (const auto& vlanDelta : delta.getVlansDelta()) {
    auto processChanged =
        [this](const auto& oldNeighbor, const auto& newNeighbor) {
          changeNeighbor(oldNeighbor, newNeighbor);
        };
    auto processAdded = [this](const auto& newNeighbor) {
      addNeighbor(newNeighbor);
    };
    auto processRemoved = [this](const auto& oldNeighbor) {
      removeNeighbor(oldNeighbor);
    };
    DeltaFunctions::forEachChanged(
        vlanDelta.getArpDelta(), processChanged, processAdded, processRemoved);
    DeltaFunctions::forEachChanged(
        vlanDelta.getNdpDelta(), processChanged, processAdded, processRemoved);
  }
}

void SaiNeighborManager::clear() {
  handles_.clear();
  unresolvedNeighbors_.clear();
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
  auto itr = handles_.find(saiEntry);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null neighbor for ip: " << saiEntry.ip().str();
  }
  return itr->second.get();
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
} // namespace fboss
} // namespace facebook
