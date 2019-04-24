/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NdpEntry.h"

namespace facebook {
namespace fboss {

SaiNeighbor::SaiNeighbor(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const NeighborApiParameters::EntryType& entry,
    const NeighborApiParameters::Attributes& attributes,
    std::unique_ptr<SaiFdbEntry> fdbEntry)
    : apiTable_(apiTable),
      managerTable_(managerTable),
      entry_(entry),
      attributes_(attributes),
      fdbEntry_(std::move(fdbEntry)) {
  auto& neighborApi = apiTable_->neighborApi();
  neighborApi.create(entry_, attributes.attrs());
  nextHop_ = managerTable_->nextHopManager().addNextHop(
      entry_.routerInterfaceId(), entry_.ip());
}

SaiNeighbor::~SaiNeighbor() {
  auto& neighborApi = apiTable_->neighborApi();
  fdbEntry_.reset();
  nextHop_.reset();
  neighborApi.remove(entry_);
}

bool SaiNeighbor::operator==(const SaiNeighbor& other) const {
  return attributes_ == other.attributes_;
}

bool SaiNeighbor::operator!=(const SaiNeighbor& other) const {
  return !(*this == other);
}

sai_object_id_t SaiNeighbor::nextHopId() const {
  return nextHop_->id();
}

SaiNeighborManager::SaiNeighborManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable)
    : apiTable_(apiTable), managerTable_(managerTable) {}

// Helper function to create a SAI NeighborEntry from an FBOSS SwitchState
// NeighborEntry (e.g., NeighborEntry<IPAddressV6, NDPTable>)
template <typename NeighborEntryT>
NeighborApiParameters::EntryType SaiNeighborManager::saiEntryFromSwEntry(
    const std::shared_ptr<NeighborEntryT>& swEntry) {
  folly::IPAddress ip(swEntry->getIP());
  SaiRouterInterface* routerInterface =
      managerTable_->routerInterfaceManager().getRouterInterface(
          swEntry->getIntfID());
  if (!routerInterface) {
    throw FbossError(
        "Failed to create sai_neighbor_entry from NeighborEntry. "
        "No SaiRouterInterface for InterfaceID: ",
        swEntry->getIntfID());
  }
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  return NeighborApiParameters::EntryType(switchId, routerInterface->id(), ip);
}

template <typename NeighborEntryT>
void SaiNeighborManager::changeNeighbor(
    const std::shared_ptr<NeighborEntryT>& oldSwEntry,
    const std::shared_ptr<NeighborEntryT>& newSwEntry) {
  if (oldSwEntry->isPending() && newSwEntry->isPending()) {
  }
  if (oldSwEntry->isPending() && !newSwEntry->isPending()) {
    XLOG(INFO) << "BO: previously unresolved neighbor is now resolved";
    removeNeighbor(oldSwEntry);
    addNeighbor(newSwEntry);
  }
  if (!oldSwEntry->isPending() && newSwEntry->isPending()) {
    XLOG(INFO) << "BO: previously resolved neighbor is now unresolved";
    removeNeighbor(oldSwEntry);
    addNeighbor(newSwEntry);
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
  auto existingSaiNeighbor = getNeighbor(saiEntry);
  if (existingSaiNeighbor) {
    throw FbossError(
        "Attempted to add duplicate neighbor: ", swEntry->getIP().str());
  }
  if (swEntry->isPending()) {
    XLOG(INFO) << "add unresolved neighbor";
    unresolvedNeighbors_.insert(saiEntry);
  } else {
    XLOG(INFO) << "add resolved neighbor";
    NeighborApiParameters::Attributes attributes{{swEntry->getMac()}};
    auto fdbEntry = managerTable_->fdbManager().addFdbEntry(
      swEntry->getIntfID(),
      swEntry->getMac(),
      swEntry->getPort());
    auto neighbor = std::make_unique<SaiNeighbor>(
        apiTable_, managerTable_, saiEntry, attributes, std::move(fdbEntry));
    sai_object_id_t nextHopId = neighbor->nextHopId();
    neighbors_.insert(std::make_pair(saiEntry, std::move(neighbor)));
    managerTable_->nextHopGroupManager().handleResolvedNeighbor(
        saiEntry, nextHopId);
  }
}

template <typename NeighborEntryT>
void SaiNeighborManager::removeNeighbor(
    const std::shared_ptr<NeighborEntryT>& swEntry) {
  XLOG(INFO) << "removeNeighbor " << swEntry->getIP();
  auto saiEntry = saiEntryFromSwEntry(swEntry);
  auto count = neighbors_.erase(saiEntry);
  if (count == 0) {
    throw FbossError(
        "Attempted to remove non-existent neighbor: ", swEntry->getIP().str());
  }
}

void SaiNeighborManager::processNeighborDelta(const StateDelta& delta) {
  for (const auto& vlanDelta : delta.getVlansDelta()) {
    auto processChanged = [this](auto oldNeighbor, auto newNeighbor) -> void {
      changeNeighbor(oldNeighbor, newNeighbor);
    };
    auto processAdded = [this](auto newNeighbor) -> void {
      addNeighbor(newNeighbor);
    };
    auto processRemoved = [this](auto oldNeighbor) -> void {
      removeNeighbor(oldNeighbor);
    };
    DeltaFunctions::forEachChanged(
        vlanDelta.getArpDelta(), processChanged, processAdded, processRemoved);
    DeltaFunctions::forEachChanged(
        vlanDelta.getNdpDelta(), processChanged, processAdded, processRemoved);
  }
}

void SaiNeighborManager::clear() {
  neighbors_.clear();
}

const SaiNeighbor* SaiNeighborManager::getNeighbor(
    const NeighborApiParameters::EntryType& saiEntry) const {
  return getNeighborImpl(saiEntry);
}
SaiNeighbor* SaiNeighborManager::getNeighbor(
    const NeighborApiParameters::EntryType& saiEntry) {
  return getNeighborImpl(saiEntry);
}
SaiNeighbor* SaiNeighborManager::getNeighborImpl(
    const NeighborApiParameters::EntryType& saiEntry) const {
  auto itr = neighbors_.find(saiEntry);
  if (itr == neighbors_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null neighbor for ip: " << saiEntry.ip().str();
  }
  return itr->second.get();
}

template NeighborApiParameters::EntryType
SaiNeighborManager::saiEntryFromSwEntry<NdpEntry>(
    const std::shared_ptr<NdpEntry>& swEntry);
template NeighborApiParameters::EntryType
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
