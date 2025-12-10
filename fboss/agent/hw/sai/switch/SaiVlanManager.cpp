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
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

#include <algorithm>
#include <tuple>

namespace facebook::fboss {
SaiVlanManager::SaiVlanManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
  if (platform_->getAsic()->isSupported(HwAsic::Feature::DEFAULT_VLAN)) {
    auto key = managerTable_->switchManager().getDefaultVlanAdapterKey();
    // save the default vlan in store, so it can be loaded if needed
    // for chenab add vlan-1 in config and router interface for vlan-1
    // this will allow pipeline look up to work.

    saiStore_->get<SaiVlanTraits>().loadObjectOwnedByAdapter(
        VlanSaiId(key), true);
  }
}

VlanSaiId SaiVlanManager::addVlan(const std::shared_ptr<Vlan>& swVlan) {
  VlanID swVlanId = swVlan->getID();

  // If we already store a handle to this VLAN, fail to add a new one
  auto handle = getVlanHandle(swVlanId);
  if (handle) {
    throw FbossError(
        "attempted to add a duplicate vlan with VlanID: ", swVlanId);
  }

  // Create the VLAN
  auto& vlanStore = saiStore_->get<SaiVlanTraits>();
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
  for (auto [swPortId, emitTags] : swVlan->getPorts()) {
    createVlanMember(swVlanId, SaiPortDescriptor(PortID(swPortId)), emitTags);
  }
  return saiVlan->adapterKey();
}

void SaiVlanManager::createVlanMember(
    VlanID swVlanId,
    SaiPortDescriptor portDesc,
    bool tagged) {
  auto vlanHandle = getVlanHandle(swVlanId);
  if (!vlanHandle) {
    throw FbossError(
        "Failed to add vlan member: no vlan matching vlanID: ", swVlanId);
  }

  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{
      vlanHandle->vlan->adapterKey()};
  auto vlanMember = std::make_shared<ManagedVlanMember>(
      this, portDesc, swVlanId, vlanIdAttribute, tagged);
  SaiObjectEventPublisher::getInstance()->get<SaiBridgePortTraits>().subscribe(
      vlanMember);
  vlanHandle->vlanMembers.emplace(portDesc, std::move(vlanMember));
}

void SaiVlanManager::removeVlanMember(
    VlanID swVlanId,
    SaiPortDescriptor portDesc) {
  const auto vlanIter = handles_.find(swVlanId);
  if (vlanIter == handles_.end()) {
    throw FbossError(
        "attempted to remove member from non-existent vlan ", swVlanId);
  }

  auto portDescIter = vlanIter->second->vlanMembers.find(portDesc);
  if (portDescIter == vlanIter->second->vlanMembers.end()) {
    throw FbossError(
        "attempted to remove non-existent member ",
        portDesc.str(),
        " from vlan ",
        swVlanId);
  }
  vlanIter->second->vlanMembers.erase(portDescIter);
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
  auto oldPorts = swVlanOld->getPorts();
  auto compareIds = [](const std::pair<int16_t, bool>& p1,
                       const std::pair<int16_t, bool>& p2) {
    return p1.first < p2.first;
  };
  auto newPorts = swVlanNew->getPorts();
  Vlan::MemberPorts removed;
  std::set_difference(
      oldPorts.begin(),
      oldPorts.end(),
      newPorts.begin(),
      newPorts.end(),
      std::inserter(removed, removed.begin()),
      compareIds);
  for (const auto& swPortId : removed) {
    handle->vlanMembers.erase(SaiPortDescriptor(PortID(swPortId.first)));
  }
  Vlan::MemberPorts added;
  std::set_difference(
      newPorts.begin(),
      newPorts.end(),
      oldPorts.begin(),
      oldPorts.end(),
      std::inserter(added, added.begin()),
      compareIds);
  for (const auto& swPortId : added) {
    createVlanMember(
        swVlanId, SaiPortDescriptor(PortID(swPortId.first)), swPortId.second);
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

std::shared_ptr<SaiVlanMember> SaiVlanManager::createSaiObject(
    const typename SaiVlanMemberTraits::AdapterHostKey& key,
    const typename SaiVlanMemberTraits::CreateAttributes& attributes) {
  auto& store = saiStore_->get<SaiVlanMemberTraits>();
  return store.setObject(key, attributes);
}

void ManagedVlanMember::createObject(PublisherObjects objects) {
  auto bridgePort = std::get<BridgePortWeakPtr>(objects).lock();
  CHECK(bridgePort);
  BridgePortSaiId bridgePortSaiId = bridgePort->adapterKey();

  SaiVlanMemberTraits::Attributes::VlanId vlanIdAttribute{saiVlanId_};
  SaiVlanMemberTraits::Attributes::BridgePortId bridgePortIdAttribute{
      bridgePortSaiId};
  auto taggingMode =
      tagged_ ? SAI_VLAN_TAGGING_MODE_TAGGED : SAI_VLAN_TAGGING_MODE_UNTAGGED;
  SaiVlanMemberTraits::Attributes::VlanTaggingMode taggingModeAttribute{
      taggingMode};
  SaiVlanMemberTraits::CreateAttributes memberAttributes{
      vlanIdAttribute, bridgePortIdAttribute, taggingModeAttribute};
  SaiVlanMemberTraits::AdapterHostKey key{
      vlanIdAttribute, bridgePortIdAttribute};

  auto object = manager_->createSaiObject(key, memberAttributes);
  this->setObject(object);
}

void ManagedVlanMember::removeObject(size_t, PublisherObjects) {
  this->resetObject();
}
} // namespace facebook::fboss
