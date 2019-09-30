/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

SaiRouterInterfaceManager::SaiRouterInterfaceManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

RouterInterfaceSaiId SaiRouterInterfaceManager::addRouterInterface(
    const std::shared_ptr<Interface>& swInterface) {
  // check if the router interface already exists
  InterfaceID swId(swInterface->getID());
  auto handle = getRouterInterfaceHandle(swId);
  if (handle) {
    throw FbossError(
        "Attempted to add duplicate router interface with InterfaceID ", swId);
  }

  // compute the virtual router id for this router interface
  RouterID routerId = swInterface->getRouterID();
  SaiVirtualRouterHandle* virtualRouterHandle =
      managerTable_->virtualRouterManager().getVirtualRouterHandle(routerId);
  if (!virtualRouterHandle) {
    throw FbossError("No virtual router for RouterID ", routerId);
  }
  sai_object_id_t saiVirtualRouterId =
      virtualRouterHandle->virtualRouter->adapterKey();
  SaiRouterInterfaceTraits::Attributes::VirtualRouterId
      virtualRouterIdAttribute{saiVirtualRouterId};

  // we only use VLAN based router interfaces
  SaiRouterInterfaceTraits::Attributes::Type typeAttribute{
      SAI_ROUTER_INTERFACE_TYPE_VLAN};

  // compute the VLAN sai id for this router interface
  VlanID swVlanId = swInterface->getVlanID();
  SaiVlanHandle* saiVlanHandle =
      managerTable_->vlanManager().getVlanHandle(swVlanId);
  if (!saiVlanHandle) {
    throw FbossError(
        "Failed to add router interface: no sai vlan for VlanID ", swVlanId);
  }
  SaiRouterInterfaceTraits::Attributes::VlanId vlanIdAttribute{
      saiVlanHandle->vlan->adapterKey()};

  // get the src mac for this router interface
  folly::MacAddress srcMac = swInterface->getMac();
  SaiRouterInterfaceTraits::Attributes::SrcMac srcMacAttribute{srcMac};

  // get MTU
  SaiRouterInterfaceTraits::Attributes::Mtu mtuAttribute{swInterface->getMtu()};

  // create the router interface
  SaiRouterInterfaceTraits::CreateAttributes attributes{
      virtualRouterIdAttribute,
      typeAttribute,
      vlanIdAttribute,
      srcMacAttribute,
      mtuAttribute};
  SaiRouterInterfaceTraits::AdapterHostKey k{
      virtualRouterIdAttribute,
      vlanIdAttribute,
  };
  auto& store = SaiStore::getInstance()->get<SaiRouterInterfaceTraits>();
  std::shared_ptr<SaiRouterInterface> routerInterface =
      store.setObject(k, attributes);
  auto routerInterfaceHandle = std::make_unique<SaiRouterInterfaceHandle>();
  routerInterfaceHandle->routerInterface = routerInterface;

  // create the ToMe routes for this router interface
  auto toMeRoutes =
      managerTable_->routeManager().makeInterfaceToMeRoutes(swInterface);
  routerInterfaceHandle->toMeRoutes.insert(
      routerInterfaceHandle->toMeRoutes.end(),
      std::make_move_iterator(toMeRoutes.begin()),
      std::make_move_iterator(toMeRoutes.end()));

  handles_.emplace(swId, std::move(routerInterfaceHandle));
  return routerInterface->adapterKey();
}

void SaiRouterInterfaceManager::removeRouterInterface(const InterfaceID& swId) {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Failed to remove non-existent router interface: ", swId);
  }
  handles_.erase(itr);
}

void SaiRouterInterfaceManager::changeRouterInterface(
    const std::shared_ptr<Interface>& /* oldInterface */,
    const std::shared_ptr<Interface>& /* newInterface */) {
  throw FbossError("Not implemented");
}

SaiRouterInterfaceHandle* SaiRouterInterfaceManager::getRouterInterfaceHandle(
    const InterfaceID& swId) {
  return getRouterInterfaceHandleImpl(swId);
}

const SaiRouterInterfaceHandle*
SaiRouterInterfaceManager::getRouterInterfaceHandle(
    const InterfaceID& swId) const {
  return getRouterInterfaceHandleImpl(swId);
}

SaiRouterInterfaceHandle*
SaiRouterInterfaceManager::getRouterInterfaceHandleImpl(
    const InterfaceID& swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second) {
    XLOG(FATAL) << "Invalid null router interface for InterfaceID " << swId;
  }
  return itr->second.get();
}

void SaiRouterInterfaceManager::processInterfaceDelta(
    const StateDelta& stateDelta) {
  auto delta = stateDelta.getIntfsDelta();
  auto processChanged =
      [this](const auto& oldInterface, const auto& newInterface) {
        changeRouterInterface(oldInterface, newInterface);
      };
  auto processAdded = [this](const auto& newInterface) {
    addRouterInterface(newInterface);
  };
  auto processRemoved = [this](const auto& oldInterface) {
    removeRouterInterface(oldInterface->getID());
  };
  DeltaFunctions::forEachChanged(
      delta, processChanged, processAdded, processRemoved);
}

} // namespace facebook::fboss
