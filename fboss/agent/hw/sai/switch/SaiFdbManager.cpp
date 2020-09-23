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
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
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
  // For FDB entry, on delete, Ignore error if entry was already removed from
  // HW. One scenario where this can occur is the following
  // - We learn a MAC and install it in HW
  // - Later we get a state update to transform this into a STATIC FDB entry
  // - Meanwhile the dynamic MAC ages out and gets deleted
  // - While processing the state delta for changed MAC entry, we try to
  // delete the dynamic entry before adding static entry
  fdbEntry->setIgnoreMissingInHwOnDelete(true);
  setObject(fdbEntry);
}

void ManagedFdbEntry::removeObject(size_t, PublisherObjects) {
  /* either interface is removed or bridge port is removed, delete fdb entry */
  this->resetObject();
}

PortID ManagedFdbEntry::getPortId() const {
  return portId_;
}

L2Entry ManagedFdbEntry::toL2Entry() const {
  std::optional<cfg::AclLookupClass> classId;
  if (metadata_) {
    classId = static_cast<cfg::AclLookupClass>(metadata_.value());
  }
  return L2Entry(
      mac_,
      // For FBOSS Vlan and interface ids are always 1:1
      VlanID(interfaceId_),
      PortDescriptor(portId_),
      // Since this entry is already programmed, its validated.
      // In vlan traffic to this MAC will be unicast and not
      // flooded.
      L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED,
      classId);
}

SaiFdbTraits::FdbEntry ManagedFdbEntry::makeFdbEntry(
    const SaiManagerTable* managerTable) const {
  auto rifHandle =
      managerTable->routerInterfaceManager().getRouterInterfaceHandle(
          interfaceId_);
  auto vlan = GET_ATTR(
      RouterInterface, VlanId, rifHandle->routerInterface->attributes());
  return SaiFdbTraits::FdbEntry{switchId_, vlan, mac_};
}

void SaiFdbManager::addFdbEntry(
    PortID port,
    InterfaceID interfaceId,
    folly::MacAddress mac,
    sai_fdb_entry_type_t type,
    std::optional<sai_uint32_t> metadata) {
  XLOGF(
      INFO,
      "Add fdb entry {}, {}, {}, type: {}, metadata: {}",
      port,
      interfaceId,
      mac,
      (type == SAI_FDB_ENTRY_TYPE_STATIC ? "static" : "dynamic"),
      metadata ? metadata.value() : 0);

  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto key = PublisherKey<SaiFdbTraits>::custom_type{interfaceId, mac};
  auto managedFdbEntryIter = managedFdbEntries_.find(key);
  if (managedFdbEntryIter != managedFdbEntries_.end()) {
    XLOGF(
        INFO,
        "fdb entry {}, {}, {}, type: {}, metadata: {} already exists.",
        managedFdbEntryIter->second->getPortId(),
        managedFdbEntryIter->second->getInterfaceID(),
        managedFdbEntryIter->second->getMac(),
        (managedFdbEntryIter->second->getType() == SAI_FDB_ENTRY_TYPE_STATIC
             ? "static"
             : "dynamic"),
        managedFdbEntryIter->second->getMetaData());
    return;
  }
  auto managedFdbEntry = std::make_shared<ManagedFdbEntry>(
      switchId,
      port,
      interfaceId,
      mac,
      type,
      metadata,
      platform_->getAsic()->isSupported(HwAsic::Feature::L2ENTRY_METADATA));

  SaiObjectEventPublisher::getInstance()->get<SaiBridgePortTraits>().subscribe(
      managedFdbEntry);
  SaiObjectEventPublisher::getInstance()
      ->get<SaiRouterInterfaceTraits>()
      .subscribe(managedFdbEntry);

  portToKeys_[port].emplace(key);
  managedFdbEntries_.emplace(key, managedFdbEntry);
}

void ManagedFdbEntry::update(const std::shared_ptr<MacEntry>& updated) {
  CHECK_EQ(portId_, updated->getPort().phyPortID());
  CHECK_EQ(mac_, updated->getMac());
  auto fdbEntry = getSaiObject();
  if (!fdbEntry) {
    return;
  }
  fdbEntry->setAttribute(SaiFdbTraits::Attributes::Type(
      updated->getType() == MacEntryType::STATIC_ENTRY
          ? SAI_FDB_ENTRY_TYPE_STATIC
          : SAI_FDB_ENTRY_TYPE_DYNAMIC));
  if (metadataSupported_) {
    sai_uint32_t metadata = updated->getClassID()
        ? static_cast<sai_uint32_t>(updated->getClassID().value())
        : 0;
    fdbEntry->setOptionalAttribute(
        SaiFdbTraits::Attributes::Metadata{metadata});
  }
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
  portToKeys_[portId].erase(key);
  if (portToKeys_[portId].empty()) {
    portToKeys_.erase(portId);
  }
  managedFdbEntries_.erase(fdbEntryItr);
}

void SaiFdbManager::addMac(const std::shared_ptr<MacEntry>& macEntry) {
  std::optional<sai_uint32_t> metadata{std::nullopt};
  if (macEntry->getClassID()) {
    metadata = static_cast<sai_uint32_t>(macEntry->getClassID().value());
  }
  auto type = SAI_FDB_ENTRY_TYPE_DYNAMIC;
  switch (macEntry->getType()) {
    case MacEntryType::STATIC_ENTRY:
      type = SAI_FDB_ENTRY_TYPE_STATIC;
      break;
    case MacEntryType::DYNAMIC_ENTRY:
      type = SAI_FDB_ENTRY_TYPE_DYNAMIC;
      break;
  };
  addFdbEntry(
      macEntry->getPort().phyPortID(),
      getInterfaceId(macEntry),
      macEntry->getMac(),
      type,
      metadata);
}

void SaiFdbManager::removeMac(const std::shared_ptr<MacEntry>& macEntry) {
  removeFdbEntry(getInterfaceId(macEntry), macEntry->getMac());
}

void SaiFdbManager::changeMac(
    const std::shared_ptr<MacEntry>& oldEntry,
    const std::shared_ptr<MacEntry>& newEntry) {
  if (*oldEntry != *newEntry) {
    CHECK_EQ(oldEntry->getMac(), newEntry->getMac());
    if (oldEntry->getPort() != newEntry->getPort()) {
      removeMac(oldEntry);
      addMac(newEntry);
    } else {
      XLOGF(INFO, "Change fdb entry {}, {}", oldEntry->str(), newEntry->str());
      auto newIntfId = getInterfaceId(newEntry);
      auto key = PublisherKey<SaiFdbTraits>::custom_type{newIntfId,
                                                         newEntry->getMac()};
      try {
        managedFdbEntries_.find(key)->second->update(newEntry);
      } catch (const SaiApiError& e) {
        // For change of MAC entry there is a possibility that the
        // previous entry was dynamic and got aged out
        if (oldEntry->getType() == MacEntryType::DYNAMIC_ENTRY &&
            e.getSaiStatus() == SAI_STATUS_ITEM_NOT_FOUND) {
          removeMac(oldEntry);
          addMac(newEntry);
        } else {
          throw;
        }
      }
    }
  }
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

L2EntryThrift SaiFdbManager::fdbToL2Entry(
    const SaiFdbTraits::FdbEntry& fdbEntry) const {
  L2EntryThrift entry;
  // SwitchState's VlanID is an attribute we store in the vlan, so
  // we can get it via SaiApi
  auto& vlanApi = SaiApiTable::getInstance()->vlanApi();
  VlanID swVlanId{vlanApi.getAttribute(
      VlanSaiId{fdbEntry.bridgeVlanId()}, SaiVlanTraits::Attributes::VlanId{})};
  entry.vlanID_ref() = swVlanId;
  entry.mac_ref() = fdbEntry.mac().toString();

  // To get the PortID, we get the bridgePortId from the fdb entry,
  // then get that Bridge Port's PortId attribute. We can lookup the
  // PortID for a sai port id in ConcurrentIndices
  auto& fdbApi = SaiApiTable::getInstance()->fdbApi();
  auto bridgePortSaiId =
      fdbApi.getAttribute(fdbEntry, SaiFdbTraits::Attributes::BridgePortId());
  auto& bridgeApi = SaiApiTable::getInstance()->bridgeApi();
  auto portSaiId = bridgeApi.getAttribute(
      BridgePortSaiId{bridgePortSaiId},
      SaiBridgePortTraits::Attributes::PortId{});
  const auto portItr = concurrentIndices_->portIds.find(PortSaiId{portSaiId});
  if (portItr == concurrentIndices_->portIds.cend()) {
    throw FbossError("l2 table entry had unknown port sai id: ", portSaiId);
  }
  entry.port_ref() = portItr->second;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::L2ENTRY_METADATA)) {
    auto metadata =
        fdbApi.getAttribute(fdbEntry, SaiFdbTraits::Attributes::Metadata{});
    if (metadata) {
      entry.classID_ref() = metadata;
    }
  }
  return entry;
}

std::vector<L2EntryThrift> SaiFdbManager::getL2Entries() const {
  std::vector<L2EntryThrift> entries;
  entries.resize(managedFdbEntries_.size());
  for (const auto& publisherAndFdbEntry : managedFdbEntries_) {
    entries.emplace_back(
        fdbToL2Entry(publisherAndFdbEntry.second->makeFdbEntry(managerTable_)));
  }
  return entries;
}
} // namespace facebook::fboss
