/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/state/ControlPlane.h"

#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook {
namespace fboss {

sai_hostif_trap_type_t SaiHostifManager::packetReasonToHostifTrap(
    cfg::PacketRxReason reason) {
  switch (reason) {
    case cfg::PacketRxReason::ARP:
      return SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST;
    case cfg::PacketRxReason::DHCP:
      return SAI_HOSTIF_TRAP_TYPE_DHCP;
    default:
      throw FbossError("invalid packet reason: ", reason);
  }
}

cfg::PacketRxReason SaiHostifManager::hostifTrapToPacketReason(
    sai_hostif_trap_type_t trapType) {
  switch (trapType) {
    case SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST:
      return cfg::PacketRxReason::ARP;
    case SAI_HOSTIF_TRAP_TYPE_DHCP:
      return cfg::PacketRxReason::DHCP;
    default:
      throw FbossError("invalid trap type: ", trapType);
  }
}

SaiHostifTrapTraits::CreateAttributes
SaiHostifManager::makeHostifTrapAttributes(
    cfg::PacketRxReason trapId,
    HostifTrapGroupSaiId trapGroupId) {
  SaiHostifTrapTraits::Attributes::PacketAction packetAction{
      SAI_PACKET_ACTION_TRAP};
  SaiHostifTrapTraits::Attributes::TrapType trapType{
      packetReasonToHostifTrap(trapId)};
  SaiHostifTrapTraits::Attributes::TrapPriority trapPriority{
      SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY};
  SaiHostifTrapTraits::Attributes::TrapGroup trapGroup{trapGroupId};

  return SaiHostifTrapTraits::CreateAttributes{
      trapType, packetAction, trapPriority, trapGroup};
}

std::shared_ptr<SaiHostifTrapGroup> SaiHostifManager::ensureHostifTrapGroup(
    uint32_t queueId) {
  auto& store = SaiStore::getInstance()->get<SaiHostifTrapGroupTraits>();
  SaiHostifTrapGroupTraits::AdapterHostKey k{queueId};
  SaiHostifTrapGroupTraits::CreateAttributes attributes{queueId, std::nullopt};
  return store.setObject(k, attributes);
}

HostifTrapSaiId SaiHostifManager::addHostifTrap(
    cfg::PacketRxReason trapId,
    uint32_t queueId) {
  if (handles_.find(trapId) != handles_.end()) {
    throw FbossError(
        "Attempted to re-add existing trap for rx reason: ",
        apache::thrift::util::enumName(trapId));
  }
  auto hostifTrapGroup = ensureHostifTrapGroup(queueId);
  auto attributes =
      makeHostifTrapAttributes(trapId, hostifTrapGroup->adapterKey());
  SaiHostifTrapTraits::AdapterHostKey k =
      GET_ATTR(HostifTrap, TrapType, attributes);
  auto& store = SaiStore::getInstance()->get<SaiHostifTrapTraits>();
  auto hostifTrap = store.setObject(k, attributes);

  auto handle = std::make_unique<SaiHostifTrapHandle>();
  handle->trap = hostifTrap;
  handle->trapGroup = hostifTrapGroup;
  handles_.emplace(trapId, std::move(handle));
  return hostifTrap->adapterKey();
}

void SaiHostifManager::removeHostifTrap(cfg::PacketRxReason trapId) {
  auto count = handles_.erase(trapId);
  if (!count) {
    throw FbossError(
        "Attempted to remove non-existent trap for rx reason: ",
        apache::thrift::util::enumName(trapId));
  }
}

void SaiHostifManager::changeHostifTrap(
    cfg::PacketRxReason trapId,
    uint32_t queueId) {
  auto handleItr = handles_.find(trapId);
  if (handleItr == handles_.end()) {
    throw FbossError(
        "Attempted to change non-existent trap for rx reason: ",
        apache::thrift::util::enumName(trapId));
  }
  auto hostifTrapGroup = ensureHostifTrapGroup(queueId);
  auto attributes =
      makeHostifTrapAttributes(trapId, hostifTrapGroup->adapterKey());
  SaiHostifTrapTraits::AdapterHostKey k =
      GET_ATTR(HostifTrap, TrapType, attributes);
  auto& store = SaiStore::getInstance()->get<SaiHostifTrapTraits>();
  auto hostifTrap = store.setObject(k, attributes);

  handleItr->second->trap = hostifTrap;
  handleItr->second->trapGroup = hostifTrapGroup;
}

void SaiHostifManager::processControlPlaneDelta(const StateDelta& delta) {
  auto controlPlaneDelta = delta.getControlPlaneDelta();
  auto& oldRxReasonToQueue = controlPlaneDelta.getOld()->getRxReasonToQueue();
  auto& newRxReasonToQueue = controlPlaneDelta.getNew()->getRxReasonToQueue();

  // RxReasonToQueue is a flat_map, so the iterator is sorted. We can rely on
  // that fact to implement handling changes in one pass.
  auto oldItr = oldRxReasonToQueue.begin();
  auto newItr = newRxReasonToQueue.begin();
  while (oldItr != oldRxReasonToQueue.end() ||
         newItr != newRxReasonToQueue.end()) {
    if (oldItr == oldRxReasonToQueue.end()) {
      // no more old reasons, all new reasons need to be added
      addHostifTrap(newItr->first, newItr->second);
      ++newItr;
    } else if (newItr == newRxReasonToQueue.end()) {
      // no more new reasons, all old reasons need to be removed
      removeHostifTrap(oldItr->first);
      ++oldItr;
    } else if (oldItr->first < newItr->first) {
      // current old is not found in new, needs to be removed
      removeHostifTrap(oldItr->first);
      ++oldItr;
    } else if (newItr->first < oldItr->first) {
      // current new is not found in old, needs to be added
      addHostifTrap(newItr->first, newItr->second);
      ++newItr;
    } else {
      // rx reasons are equal -- check if the queue changed!
      if (oldItr->second != newItr->second) {
        changeHostifTrap(newItr->first, newItr->second);
      }
      ++oldItr;
      ++newItr;
    }
  }
}

SaiHostifManager::SaiHostifManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable)
    : apiTable_(apiTable), managerTable_(managerTable) {}

} // namespace fboss
} // namespace facebook
