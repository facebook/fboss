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
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

SaiRouterInterface::SaiRouterInterface(
    SaiApiTable* apiTable,
    const RouterInterfaceApiParameters::Attributes& attributes,
    const sai_object_id_t& switchID)
    : apiTable_(apiTable), attributes_(attributes) {
  auto& routerInterfaceApi = apiTable_->routerInterfaceApi();
  id_ = routerInterfaceApi.create2(attributes_.attrs(), switchID);
}

SaiRouterInterface::~SaiRouterInterface() {
  auto& routerInterfaceApi = apiTable_->routerInterfaceApi();
  routerInterfaceApi.remove(id_);
}

bool SaiRouterInterface::operator==(const SaiRouterInterface& other) const {
  return attributes_ == other.attributes();
}
bool SaiRouterInterface::operator!=(const SaiRouterInterface& other) const {
  return !(*this == other);
}

SaiRouterInterfaceManager::SaiRouterInterfaceManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable)
    : apiTable_(apiTable), managerTable_(managerTable) {}

sai_object_id_t SaiRouterInterfaceManager::addRouterInterface(
    const std::shared_ptr<Interface>& swInterface) {
  InterfaceID swId(swInterface->getID());
  auto itr = routerInterfaces_.find(swId);
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  if (itr != routerInterfaces_.end()) {
    throw FbossError(
        "Attempted to add duplicate router interface with InterfaceID ", swId);
  }
  RouterID routerId = swInterface->getRouterID();
  folly::MacAddress srcMac = swInterface->getMac();
  SaiVirtualRouter* virtualRouter =
      managerTable_->virtualRouterManager().getVirtualRouter(routerId);
  if (!virtualRouter) {
    throw FbossError("No virtual router for RouterID ", routerId);
  }
  sai_object_id_t saiVirtualRouterId = virtualRouter->id();
  RouterInterfaceApiParameters::Attributes::VirtualRouterId
      virtualRouterIdAttribute{saiVirtualRouterId};
  RouterInterfaceApiParameters::Attributes::Type typeAttribute{
      SAI_ROUTER_INTERFACE_TYPE_VLAN};
  VlanID swVlanId = swInterface->getVlanID();
  SaiVlan* saiVlan = managerTable_->vlanManager().getVlan(swVlanId);
  if (!saiVlan) {
    throw FbossError(
        "Failed to add router interface: no sai vlan for VlanID ", swVlanId);
  }
  RouterInterfaceApiParameters::Attributes::VlanId vlanIdAttribute{
      saiVlan->id()};
  RouterInterfaceApiParameters::Attributes::SrcMac srcMacAttribute{srcMac};
  RouterInterfaceApiParameters::Attributes attributes{{virtualRouterIdAttribute,
                                                       typeAttribute,
                                                       vlanIdAttribute,
                                                       srcMacAttribute}};
  auto routerInterface =
      std::make_unique<SaiRouterInterface>(apiTable_, attributes, switchId);
  sai_object_id_t saiId = routerInterface->id();
  routerInterfaces_.insert(std::make_pair(swId, std::move(routerInterface)));
  return saiId;
}

void SaiRouterInterfaceManager::removeRouterInterface(const InterfaceID& swId) {
  auto itr = routerInterfaces_.find(swId);
  if (itr == routerInterfaces_.end()) {
    throw FbossError("Failed to remove non-existent router interface: ", swId);
  }
  routerInterfaces_.erase(itr);
}

SaiRouterInterface* SaiRouterInterfaceManager::getRouterInterface(
    const InterfaceID& swId) {
  return getRouterInterfaceImpl(swId);
}

const SaiRouterInterface* SaiRouterInterfaceManager::getRouterInterface(
    const InterfaceID& swId) const {
  return getRouterInterfaceImpl(swId);
}

SaiRouterInterface* SaiRouterInterfaceManager::getRouterInterfaceImpl(
    const InterfaceID& swId) const {
  auto itr = routerInterfaces_.find(swId);
  if (itr == routerInterfaces_.end()) {
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
  auto processChanged = [this](auto oldInterface, auto newInterface) {
    // TODO
  };
  auto processAdded = [this] (auto newInterface) {
    addRouterInterface(newInterface);
  };
  auto processRemoved = [this] (auto oldInterface) {
    removeRouterInterface(oldInterface->getID());
  };
  DeltaFunctions::forEachChanged(
      delta, processChanged, processAdded, processRemoved);
}

} // namespace fboss
} // namespace facebook
