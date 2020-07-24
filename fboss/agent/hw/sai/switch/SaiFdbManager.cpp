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

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/state/MacEntry.h"

#include <memory>
#include <tuple>

namespace facebook::fboss {

SaiFdbManager::SaiFdbManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform,
    const ConcurrentIndices* concurrentIndices)
    : managerTable_(managerTable),
      platform_(platform),
      concurrentIndices_(concurrentIndices) {}

void ManagedFdbEntry::createObject(PublisherObjects objects) {
  /* both interface and  bridge port exist, create fdb entry */
  auto interface = std::get<RouterInterfaceWeakPtr>(objects).lock();
  auto vlan = SaiApiTable::getInstance()->routerInterfaceApi().getAttribute(
      interface->adapterKey(), SaiRouterInterfaceTraits::Attributes::VlanId{});
  SaiFdbTraits::FdbEntry entry{switchId_, vlan, mac_};

  auto bridgePort = std::get<BridgePortWeakPtr>(objects).lock();
  auto bridgePortId = bridgePort->adapterKey();
  SaiFdbTraits::CreateAttributes attributes{type_, bridgePortId, metadata_};

  auto& store = SaiStore::getInstance()->get<SaiFdbTraits>();
  auto fdbEntry =
      store.setObject(entry, attributes, std::make_tuple(interfaceId_, mac_));
  setObject(fdbEntry);
}

void ManagedFdbEntry::removeObject(size_t, PublisherObjects) {
  /* either interface is removed or bridge port is removed, delete fdb entry */
  this->resetObject();
}

PortID ManagedFdbEntry::getPortId() const {
  return portId_;
}

void SaiFdbManager::addFdbEntry(
    PortID port,
    InterfaceID interfaceId,
    folly::MacAddress mac,
    sai_fdb_entry_type_t type,
    std::optional<sai_uint32_t> metadata) {
  XLOGF(
      INFO,
      "Add fdb entry {}, {}, {}, metadata: {}",
      port,
      interfaceId,
      mac,
      metadata ? metadata.value() : 0);

  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto key = PublisherKey<SaiFdbTraits>::custom_type{interfaceId, mac};
  auto managedFdbEntry = std::make_shared<ManagedFdbEntry>(
      switchId, port, interfaceId, mac, type, metadata);

  SaiObjectEventPublisher::getInstance()->get<SaiBridgePortTraits>().subscribe(
      managedFdbEntry);
  SaiObjectEventPublisher::getInstance()
      ->get<SaiRouterInterfaceTraits>()
      .subscribe(managedFdbEntry);

  portToKeys_[port].emplace(key);
  managedFdbEntries_.emplace(key, managedFdbEntry);
}

void SaiFdbManager::removeFdbEntry(
    InterfaceID interfaceId,
    folly::MacAddress mac) {
  XLOGF(INFO, "Remove fdb entry {}, {}", interfaceId, mac);
  auto key = PublisherKey<SaiFdbTraits>::custom_type{interfaceId, mac};
  auto fdbEntryItr = managedFdbEntries_.find(key);
  if (fdbEntryItr == managedFdbEntries_.end()) {
    XLOG(WARN) << "Attempted to remove non-existent FDB entry";
    return;
  }
  auto portId = fdbEntryItr->second->getPortId();
  managedFdbEntries_.erase(fdbEntryItr);
  portToKeys_[portId].erase(key);
  if (portToKeys_[portId].empty()) {
    portToKeys_.erase(portId);
  }
}

void SaiFdbManager::addMac(const std::shared_ptr<MacEntry>& macEntry) {
  std::optional<sai_uint32_t> metadata{std::nullopt};
  if (macEntry->getClassID()) {
    metadata = static_cast<sai_uint32_t>(macEntry->getClassID().value());
  }
  addFdbEntry(
      macEntry->getPort().phyPortID(),
      getInterfaceId(macEntry),
      macEntry->getMac(),
      SAI_FDB_ENTRY_TYPE_DYNAMIC,
      metadata);
}

void SaiFdbManager::removeMac(const std::shared_ptr<MacEntry>& macEntry) {
  removeFdbEntry(getInterfaceId(macEntry), macEntry->getMac());
}

InterfaceID SaiFdbManager::getInterfaceId(
    const std::shared_ptr<MacEntry>& macEntry) const {
  if (macEntry->getPort().isAggregatePort()) {
    throw FbossError("Aggregate ports not yet supported in SAI");
  }
  auto const portHandle = managerTable_->portManager().getPortHandle(
      macEntry->getPort().phyPortID());
  // FBOSS assumes a 1:1 correpondance b/w Vlan and interfae IDs
  return InterfaceID(
      concurrentIndices_->vlanIds.find(portHandle->port->adapterKey())->second);
}

void SaiFdbManager::handleLinkDown(PortID portId) {
  auto portToKeysItr = portToKeys_.find(portId);
  if (portToKeysItr != portToKeys_.end()) {
    for (const auto& key : portToKeysItr->second) {
      auto fdbEntryItr = managedFdbEntries_.find(key);
      if (UNLIKELY(fdbEntryItr == managedFdbEntries_.end())) {
        XLOGF(
            FATAL,
            "no fdb entry found for key in portToKeys_ mapping: {}",
            key);
      }
      /*
       * remove the fdb entry object, which will trigger removing a chain of
       * subscribed dependent objects:
       * fdb_entry->neighbor->next_hop->next_hop_group_member
       *
       * Removing the next hop group member will effectively shrink
       * each affected ECMP group which will minimize blackholing while
       * ARP/NDP converge.
       */
      fdbEntryItr->second->resetObject();
    }
  }
}

} // namespace facebook::fboss
