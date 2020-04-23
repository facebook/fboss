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

std::shared_ptr<SaiFdbEntry> SaiFdbManager::addFdbEntry(
    const InterfaceID& intfId,
    const folly::MacAddress& mac,
    const PortDescriptor& portDesc) {
  XLOG(INFO) << "addFdb " << mac;
  SaiRouterInterfaceHandle* routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(intfId);
  if (!routerInterfaceHandle) {
    throw FbossError(
        "Attempted to add non-existent interface to Fdb: ", intfId);
  }
  auto rif = routerInterfaceHandle->routerInterface;
  auto vlanId = GET_ATTR(RouterInterface, VlanId, rif->attributes());
  // TODO(srikrishnagopu): Can it be an AGG Port ?
  auto portId = portDesc.phyPortID();
  auto portHandle = managerTable_->portManager().getPortHandle(portId);
  if (!portHandle) {
    throw FbossError("Attempted to add non-existent port to Fdb: ", portId);
  }
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto bridgePortId = portHandle->bridgePort->adapterKey();
  SaiFdbTraits::FdbEntry entry{switchId, vlanId, mac};
  SaiFdbTraits::CreateAttributes attributes{SAI_FDB_ENTRY_TYPE_STATIC,
                                            bridgePortId};
  auto& store = SaiStore::getInstance()->get<SaiFdbTraits>();
  return store.setObject(entry, attributes, std::make_tuple(intfId, mac));
}

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

void SaiFdbManager::addFdbEntry(
    PortID port,
    InterfaceID interfaceId,
    folly::MacAddress mac) {
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto key = PublisherKey<SaiFdbTraits>::custom_type{interfaceId, mac};
  auto subscriber =
      std::make_shared<SubscriberForFdbEntry>(switchId, port, interfaceId, mac);

  SaiObjectEventPublisher::getInstance()->get<SaiBridgePortTraits>().subscribe(
      subscriber);
  SaiObjectEventPublisher::getInstance()
      ->get<SaiRouterInterfaceTraits>()
      .subscribe(subscriber);

  subscribersForFdbEntry_.emplace(key, subscriber);
}

void SaiFdbManager::removeFdbEntry(
    InterfaceID interfaceId,
    folly::MacAddress mac) {
  auto key = PublisherKey<SaiFdbTraits>::custom_type{interfaceId, mac};
  subscribersForFdbEntry_.erase(key);
}

} // namespace facebook::fboss
