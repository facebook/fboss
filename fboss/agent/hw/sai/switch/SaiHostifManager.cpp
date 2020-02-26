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

#include <chrono>

using namespace std::chrono;

namespace facebook::fboss {

sai_hostif_trap_type_t SaiHostifManager::packetReasonToHostifTrap(
    cfg::PacketRxReason reason) {
  switch (reason) {
    case cfg::PacketRxReason::ARP:
      return SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST;
    case cfg::PacketRxReason::ARP_RESPONSE:
      return SAI_HOSTIF_TRAP_TYPE_ARP_RESPONSE;
    case cfg::PacketRxReason::NDP:
      return SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY;
    case cfg::PacketRxReason::CPU_IS_NHOP:
      return SAI_HOSTIF_TRAP_TYPE_IP2ME;
    case cfg::PacketRxReason::DHCP:
      return SAI_HOSTIF_TRAP_TYPE_DHCP;
    case cfg::PacketRxReason::LLDP:
      return SAI_HOSTIF_TRAP_TYPE_LLDP;
    case cfg::PacketRxReason::BGP:
      return SAI_HOSTIF_TRAP_TYPE_BGP;
    case cfg::PacketRxReason::BGPV6:
      return SAI_HOSTIF_TRAP_TYPE_BGPV6;
    default:
      throw FbossError("invalid packet reason: ", reason);
  }
}

cfg::PacketRxReason SaiHostifManager::hostifTrapToPacketReason(
    sai_hostif_trap_type_t trapType) {
  switch (trapType) {
    case SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST:
      return cfg::PacketRxReason::ARP;
    case SAI_HOSTIF_TRAP_TYPE_ARP_RESPONSE:
      return cfg::PacketRxReason::ARP_RESPONSE;
    case SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY:
      return cfg::PacketRxReason::NDP;
    case SAI_HOSTIF_TRAP_TYPE_IP2ME:
      return cfg::PacketRxReason::CPU_IS_NHOP;
    case SAI_HOSTIF_TRAP_TYPE_DHCP:
      return cfg::PacketRxReason::DHCP;
    case SAI_HOSTIF_TRAP_TYPE_LLDP:
      return cfg::PacketRxReason::LLDP;
    case SAI_HOSTIF_TRAP_TYPE_BGP:
      return cfg::PacketRxReason::BGP;
    case SAI_HOSTIF_TRAP_TYPE_BGPV6:
      return cfg::PacketRxReason::BGPV6;
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
  /*
   * TODO: Setting it to max priority to trap ARP packets.
   */
  SaiHostifTrapTraits::Attributes::TrapPriority trapPriority{255};
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

void SaiHostifManager::processRxReasonToQueueDelta(const StateDelta& delta) {
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
      addHostifTrap(newItr->rxReason, newItr->queueId);
      ++newItr;
    } else if (newItr == newRxReasonToQueue.end()) {
      // no more new reasons, all old reasons need to be removed
      removeHostifTrap(oldItr->rxReason);
      ++oldItr;
    } else if (oldItr->rxReason < newItr->rxReason) {
      // current old is not found in new, needs to be removed
      removeHostifTrap(oldItr->rxReason);
      ++oldItr;
    } else if (newItr->rxReason < oldItr->rxReason) {
      // current new is not found in old, needs to be added
      addHostifTrap(newItr->rxReason, newItr->queueId);
      ++newItr;
    } else {
      // rx reasons are equal -- check if the queue changed!
      if (oldItr->queueId != newItr->queueId) {
        changeHostifTrap(newItr->rxReason, newItr->queueId);
      }
      ++oldItr;
      ++newItr;
    }
  }
}

void SaiHostifManager::processQueueDelta(const StateDelta& delta) {
  auto controlPlaneDelta = delta.getControlPlaneDelta();
  auto& oldQueueConfig = controlPlaneDelta.getOld()->getQueues();
  auto& newQueueConfig = controlPlaneDelta.getNew()->getQueues();
  changeCpuQueue(oldQueueConfig, newQueueConfig);
}

void SaiHostifManager::processHostifDelta(const StateDelta& delta) {
  // TODO: Can we have reason code to a queue mapping that does not have
  // corresponding sai queue oid for cpu port ?
  processRxReasonToQueueDelta(delta);
  processQueueDelta(delta);
}

SaiQueueHandle* SaiHostifManager::getQueueHandleImpl(
    const SaiQueueConfig& saiQueueConfig) const {
  auto itr = cpuPortHandle_->queues.find(saiQueueConfig);
  if (itr == cpuPortHandle_->queues.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiQueueHandle";
  }
  return itr->second.get();
}

const SaiQueueHandle* SaiHostifManager::getQueueHandle(
    const SaiQueueConfig& saiQueueConfig) const {
  return getQueueHandleImpl(saiQueueConfig);
}

SaiQueueHandle* SaiHostifManager::getQueueHandle(
    const SaiQueueConfig& saiQueueConfig) {
  return getQueueHandleImpl(saiQueueConfig);
}

void SaiHostifManager::changeCpuQueue(
    const QueueConfig& oldQueueConfig,
    const QueueConfig& newQueueConfig) {
  for (auto newPortQueue : newQueueConfig) {
    // Queue create or update
    SaiQueueConfig saiQueueConfig =
        std::make_pair(newPortQueue->getID(), newPortQueue->getStreamType());
    auto queueHandle = getQueueHandle(saiQueueConfig);
    managerTable_->queueManager().changeQueue(queueHandle, *newPortQueue);
    auto queueName = newPortQueue->getName()
        ? *newPortQueue->getName()
        : folly::to<std::string>("cpuQueue", newPortQueue->getID());
    cpuStats_.queueChanged(newPortQueue->getID(), queueName);
  }
  for (auto oldPortQueue : oldQueueConfig) {
    auto portQueueIter = std::find_if(
        newQueueConfig.begin(),
        newQueueConfig.end(),
        [&](const std::shared_ptr<PortQueue> portQueue) {
          return portQueue->getID() == oldPortQueue->getID();
        });
    // Queue Remove
    if (portQueueIter == newQueueConfig.end()) {
      SaiQueueConfig saiQueueConfig =
          std::make_pair(oldPortQueue->getID(), oldPortQueue->getStreamType());
      auto queueHandle = getQueueHandle(saiQueueConfig);
      managerTable_->queueManager().resetQueue(queueHandle);
      cpuPortHandle_->queues.erase(saiQueueConfig);
      cpuStats_.queueRemoved(oldPortQueue->getID());
    }
  }
}

void SaiHostifManager::loadCpuPortQueues() {
  std::vector<sai_object_id_t> queueList;
  queueList.resize(1);
  SaiPortTraits::Attributes::QosQueueList queueListAttribute{queueList};
  auto queueSaiIdList = SaiApiTable::getInstance()->portApi().getAttribute(
      cpuPortHandle_->cpuPortId, queueListAttribute);
  if (queueSaiIdList.size() == 0) {
    throw FbossError("no queues exist for cpu port ");
  }
  std::vector<QueueSaiId> queueSaiIds;
  queueSaiIds.reserve(queueSaiIdList.size());
  std::transform(
      queueSaiIdList.begin(),
      queueSaiIdList.end(),
      std::back_inserter(queueSaiIds),
      [](sai_object_id_t queueId) -> QueueSaiId {
        return QueueSaiId(queueId);
      });
  cpuPortHandle_->queues = managerTable_->queueManager().loadQueues(
      cpuPortHandle_->cpuPortId, queueSaiIds);
}

void SaiHostifManager::loadCpuPort() {
  cpuPortHandle_ = std::make_unique<SaiCpuPortHandle>();
  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
  PortSaiId cpuPortId{SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::CpuPort{})};
  cpuPortHandle_->cpuPortId = cpuPortId;
  loadCpuPortQueues();
}

SaiHostifManager::SaiHostifManager(SaiManagerTable* managerTable)
    : managerTable_(managerTable) {
  loadCpuPort();
}

void SaiHostifManager::updateStats() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  HwPortStats cpuQueueStats;
  managerTable_->queueManager().updateStats(
      cpuPortHandle_->queues, cpuQueueStats);
  cpuStats_.updateStats(cpuQueueStats, now);
}

HwPortStats SaiHostifManager::getCpuPortStats() const {
  HwPortStats hwPortStats;
  managerTable_->queueManager().getStats(cpuPortHandle_->queues, hwPortStats);
  return hwPortStats;
}

QueueConfig SaiHostifManager::getQueueSettings() const {
  return managerTable_->queueManager().getQueueSettings(cpuPortHandle_->queues);
}
} // namespace facebook::fboss
