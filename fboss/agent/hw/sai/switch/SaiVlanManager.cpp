/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/logging/xlog.h>

#include <algorithm>
#include <tuple>

namespace facebook::fboss {

SaiVlanManager::SaiVlanManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform,
    ConcurrentIndices* concurrentIndices)
    : managerTable_(managerTable), platform_(platform) {}

VlanSaiId SaiVlanManager::addVlan(const std::shared_ptr<Vlan>& swVlan) {
  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  VlanID swVlanId = swVlan->getID();

  // If we already store a handle to this VLAN, fail to add a new one
  auto handle = getVlanHandle(swVlanId);
  if (handle) {
    throw FbossError(
        "attempted to add a duplicate vlan with VlanID: ", swVlanId);
  }

  // Create the VLAN
  auto& vlanStore = s->get<SaiVlanTraits>();
  SaiVlanTraits::AdapterHostKey adapterHostKey{swVlanId};
  SaiVlanTraits::CreateAttributes attributes{swVlanId};
  auto saiVlan = vlanStore.setObject(adapterHostKey, attributes);
  auto vlanHandle = std::make_unique<SaiVlanHandle>();
  vlanHandle->vlan = saiVlan;
  vlanHandle->vlanMembers.reserve(swVlan->getPorts().size());
  // createVlanMember relies on the handle being in handles_,
  // so we must do this before we create the members
  handles_.emplace(swVlanId, std::move(vlanHandle));

  // Create VLAN members
  for (const auto& memberPort : swVlan->getPorts()) {
    PortID swPortId = memberPort.first;
    createVlanMember(swVlanId, swPortId);
  }
  return saiVlan->adapterKey();
}

void SaiVlanManager::createVlanMember(VlanID swVlanId, PortID swPortId) {
  auto vlanHandle = getVlanHandle(swVlanId);
  if (!vlanHandle) {
    throw FbossError(
        "Failed to add vlan member: no vlan matching vlanID: ", swVlanId);
  }

  // Compute the BridgePort sai id to associate with this vlan member
  SaiPortHandle* portHandle =
      managerTable_->portManager().getPortHandle(swPortId);
  if (!portHandle) {
    throw FbossError(
        "Failed to add vlan member: no port handle matching vlan member port: ",
        swPortId);
  }

  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{
      vlanHandle->vlan->adapterKey()};
  auto vlanMember =
      std::make_shared<ManagedVlanMember>(swPortId, swVlanId, vlanIdAttribute);
  SaiObjectEventPublisher::getInstance()->get<SaiBridgePortTraits>().subscribe(
      vlanMember);
  vlanHandle->vlanMembers.emplace(swPortId, std::move(vlanMember));
}

void SaiVlanManager::removeVlan(const std::shared_ptr<Vlan>& swVlan) {
  auto swVlanId = swVlan->getID();
  const auto citr = handles_.find(swVlanId);
  if (citr == handles_.cend()) {
    throw FbossError(
        "attempted to remove a vlan which does not exist: ", swVlanId);
  }
  handles_.erase(citr);
}

void SaiVlanManager::changeVlan(
    const std::shared_ptr<Vlan>& swVlanOld,
    const std::shared_ptr<Vlan>& swVlanNew) {
  VlanID swVlanId = swVlanNew->getID();
  auto handle = getVlanHandle(swVlanId);
  if (!handle) {
    throw FbossError(
        "attempted to change a vlan which does not exist: ", swVlanId);
  }
  const VlanFields::MemberPorts& oldPorts = swVlanOld->getPorts();
  auto compareIds = [](const std::pair<PortID, VlanFields::PortInfo>& p1,
                       const std::pair<PortID, VlanFields::PortInfo>& p2) {
    return p1.first < p2.first;
  };
  const VlanFields::MemberPorts& newPorts = swVlanNew->getPorts();
  VlanFields::MemberPorts removed;
  std::set_difference(
      oldPorts.begin(),
      oldPorts.end(),
      newPorts.begin(),
      newPorts.end(),
      std::inserter(removed, removed.begin()),
      compareIds);
  for (const auto& swPortId : removed) {
    handle->vlanMembers.erase(swPortId.first);
    SaiPortHandle* portHandle =
        managerTable_->portManager().getPortHandle(swPortId.first);
    if (!portHandle) {
      throw FbossError(
          "Failed to remove vlan member: "
          "no port handle matching vlan member port: ",
          swPortId.first);
    }
  }
  VlanFields::MemberPorts added;
  std::set_difference(
      newPorts.begin(),
      newPorts.end(),
      oldPorts.begin(),
      oldPorts.end(),
      std::inserter(added, added.begin()),
      compareIds);
  for (const auto& swPortId : added) {
    createVlanMember(swVlanId, swPortId.first);
  }
}

const SaiVlanHandle* SaiVlanManager::getVlanHandle(VlanID swVlanId) const {
  return getVlanHandleImpl(swVlanId);
}

SaiVlanHandle* SaiVlanManager::getVlanHandle(VlanID swVlanId) {
  return getVlanHandleImpl(swVlanId);
}

SaiVlanHandle* SaiVlanManager::getVlanHandleImpl(VlanID swVlanId) const {
  auto itr = handles_.find(swVlanId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second || !itr->second->vlan) {
    XLOG(FATAL) << "invalid null VLAN for VlanID: " << swVlanId;
  }
  return itr->second.get();
}

void ManagedVlanMember::createObject(PublisherObjects objects) {
  auto bridgePort = std::get<BridgePortWeakPtr>(objects).lock();
  CHECK(bridgePort);
  BridgePortSaiId bridgePortSaiId = bridgePort->adapterKey();

  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{saiVlanId_};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{
      bridgePortSaiId};
  SaiVlanMemberTraits::CreateAttributes memberAttributes{vlanIdAttribute,
                                                         bridgePortIdAttribute};
  SaiVlanMemberTraits::AdapterHostKey memberAdapterHostKey{
      bridgePortIdAttribute};

  this->setObject(memberAdapterHostKey, memberAttributes);
}

void ManagedVlanMember::removeObject(size_t, PublisherObjects) {
  this->resetObject();
}
} // namespace facebook::fboss
