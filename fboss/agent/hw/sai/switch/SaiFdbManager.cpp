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
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

#include <memory>
#include <tuple>

namespace facebook::fboss {

SaiFdbManager::SaiFdbManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SubscriberForFdbEntry::createObject(PublisherObjects objects) {
  /* both interface and  bridge port exist, create fdb entry */
  auto interface = std::get<RouterInterfaceWeakPtr>(objects).lock();
  auto vlan = SaiApiTable::getInstance()->routerInterfaceApi().getAttribute(
      interface->adapterKey(), SaiRouterInterfaceTraits::Attributes::VlanId{});
  SaiFdbTraits::FdbEntry entry{switchId_, vlan, mac_};

  auto bridgePort = std::get<BridgePortWeakPtr>(objects).lock();
  auto bridgePortId = bridgePort->adapterKey();
  SaiFdbTraits::CreateAttributes attributes{SAI_FDB_ENTRY_TYPE_STATIC,
                                            bridgePortId};

  auto& store = SaiStore::getInstance()->get<SaiFdbTraits>();
  auto fdbEntry =
      store.setObject(entry, attributes, std::make_tuple(interfaceId_, mac_));
  setObject(fdbEntry);
}
void SubscriberForFdbEntry::removeObject(size_t, PublisherObjects) {
  /* either interface is removed or bridge port is removed, delete fdb entry */
  this->resetObject();
}

PortID SubscriberForFdbEntry::getPortId() const {
  return portId_;
}

void SaiFdbManager::addFdbEntry(
    PortID port,
    InterfaceID interfaceId,
    folly::MacAddress mac) {
  XLOGF(INFO, "Add fdb entry {}, {}, {}", port, interfaceId, mac);
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto key = PublisherKey<SaiFdbTraits>::custom_type{interfaceId, mac};
  auto subscriber =
      std::make_shared<SubscriberForFdbEntry>(switchId, port, interfaceId, mac);

  SaiObjectEventPublisher::getInstance()->get<SaiBridgePortTraits>().subscribe(
      subscriber);
  SaiObjectEventPublisher::getInstance()
      ->get<SaiRouterInterfaceTraits>()
      .subscribe(subscriber);

  portToKeys_[port].emplace(key);
  subscribersForFdbEntry_.emplace(key, subscriber);
}

void SaiFdbManager::removeFdbEntry(
    InterfaceID interfaceId,
    folly::MacAddress mac) {
  XLOGF(INFO, "Remove fdb entry {}, {}", interfaceId, mac);
  auto key = PublisherKey<SaiFdbTraits>::custom_type{interfaceId, mac};
  auto subscriberItr = subscribersForFdbEntry_.find(key);
  if (subscriberItr == subscribersForFdbEntry_.end()) {
    XLOG(WARN) << "Attempted to remove non-existent FDB entry";
    return;
  }
  auto portId = subscriberItr->second->getPortId();
  subscribersForFdbEntry_.erase(subscriberItr);
  portToKeys_[portId].erase(key);
  if (portToKeys_[portId].empty()) {
    portToKeys_.erase(portId);
  }
}

void SaiFdbManager::handleLinkDown(PortID portId) {
  auto portToKeysItr = portToKeys_.find(portId);
  if (portToKeysItr != portToKeys_.end()) {
    for (const auto& key : portToKeysItr->second) {
      auto subscriberItr = subscribersForFdbEntry_.find(key);
      if (UNLIKELY(subscriberItr == subscribersForFdbEntry_.end())) {
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
      subscriberItr->second->resetObject();
    }
  }
}

} // namespace facebook::fboss
