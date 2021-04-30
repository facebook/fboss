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
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/ControlPlane.h"

#include "thrift/lib/cpp/util/EnumUtils.h"

#include <chrono>

using namespace std::chrono;

namespace facebook::fboss {

SaiHostifManager::SaiHostifManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
  if (platform_->getAsic()->isSupported(HwAsic::Feature::CPU_PORT)) {
    loadCpuPort();
  }
}

std::pair<sai_hostif_trap_type_t, sai_packet_action_t>
SaiHostifManager::packetReasonToHostifTrap(
    cfg::PacketRxReason reason,
    const SaiPlatform* platform) {
  /*
   * Traps such as ARP request, ARP response and IPv6 ND are configured with
   * packet action COPY:
   *  - One copy reaches the CPU if the ARP/NDP is for the switch.
   *  - Also flooded to the vlan so that the hosts on the vlan receive it.
   * For eg, all rsw downlinks will be in the same vlan. Pinging between hosts
   * will generate an ARP/NdP which has to be flooded to the vlan members.
   * IP2ME, BGP and BGPV6 are destined to the switch and hence configured as
   * TRAP. LLDP and DHCP is link local and hence configured as TRAP.
   */
  switch (reason) {
    case cfg::PacketRxReason::ARP:
      return std::make_pair(
          SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST, SAI_PACKET_ACTION_COPY);
    case cfg::PacketRxReason::ARP_RESPONSE:
      return std::make_pair(
          SAI_HOSTIF_TRAP_TYPE_ARP_RESPONSE, SAI_PACKET_ACTION_COPY);
    case cfg::PacketRxReason::NDP:
      return std::make_pair(
          SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY, SAI_PACKET_ACTION_COPY);
    case cfg::PacketRxReason::CPU_IS_NHOP:
      return std::make_pair(SAI_HOSTIF_TRAP_TYPE_IP2ME, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::DHCP:
      return std::make_pair(SAI_HOSTIF_TRAP_TYPE_DHCP, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::LLDP:
      return std::make_pair(SAI_HOSTIF_TRAP_TYPE_LLDP, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::BGP:
      return std::make_pair(SAI_HOSTIF_TRAP_TYPE_BGP, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::BGPV6:
      return std::make_pair(SAI_HOSTIF_TRAP_TYPE_BGPV6, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::LACP:
      return std::make_pair(SAI_HOSTIF_TRAP_TYPE_LACP, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::L3_MTU_ERROR:
      return std::make_pair(
          SAI_HOSTIF_TRAP_TYPE_L3_MTU_ERROR, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::TTL_1:
      return std::make_pair(
          SAI_HOSTIF_TRAP_TYPE_TTL_ERROR, SAI_PACKET_ACTION_TRAP);
    case cfg::PacketRxReason::BPDU:
    case cfg::PacketRxReason::L3_SLOW_PATH:
    case cfg::PacketRxReason::L3_DEST_MISS:
    case cfg::PacketRxReason::UNMATCHED:
      break;
  }
  throw FbossError("invalid packet reason: ", reason);
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
    case SAI_HOSTIF_TRAP_TYPE_LACP:
      return cfg::PacketRxReason::LACP;
    case SAI_HOSTIF_TRAP_TYPE_L3_MTU_ERROR:
      return cfg::PacketRxReason::L3_MTU_ERROR;
    case SAI_HOSTIF_TRAP_TYPE_TTL_ERROR:
      return cfg::PacketRxReason::TTL_1;
    default:
      throw FbossError("invalid trap type: ", trapType);
  }
}

SaiHostifTrapTraits::CreateAttributes
SaiHostifManager::makeHostifTrapAttributes(
    cfg::PacketRxReason trapId,
    HostifTrapGroupSaiId trapGroupId,
    uint16_t priority,
    const SaiPlatform* platform) {
  sai_hostif_trap_type_t hostifTrapId;
  sai_packet_action_t hostifPacketAction;
  std::tie(hostifTrapId, hostifPacketAction) =
      packetReasonToHostifTrap(trapId, platform);
  SaiHostifTrapTraits::Attributes::PacketAction packetAction{
      hostifPacketAction};
  SaiHostifTrapTraits::Attributes::TrapType trapType{hostifTrapId};
  SaiHostifTrapTraits::Attributes::TrapPriority trapPriority{priority};
  SaiHostifTrapTraits::Attributes::TrapGroup trapGroup{trapGroupId};
  return SaiHostifTrapTraits::CreateAttributes{
      trapType, packetAction, trapPriority, trapGroup};
}

std::shared_ptr<SaiHostifTrapGroup> SaiHostifManager::ensureHostifTrapGroup(
    uint32_t queueId) {
  auto& store = saiStore_->get<SaiHostifTrapGroupTraits>();
  SaiHostifTrapGroupTraits::AdapterHostKey k{queueId};
  SaiHostifTrapGroupTraits::CreateAttributes attributes{queueId, std::nullopt};
  return store.setObject(k, attributes);
}

HostifTrapSaiId SaiHostifManager::addHostifTrap(
    cfg::PacketRxReason trapId,
    uint32_t queueId,
    uint16_t priority) {
  XLOGF(
      INFO,
      "add trap for {}, send to queue {}, with priority {}",
      apache::thrift::util::enumName(trapId),
      queueId,
      priority);
  if (handles_.find(trapId) != handles_.end()) {
    throw FbossError(
        "Attempted to re-add existing trap for rx reason: ",
        apache::thrift::util::enumName(trapId));
  }
  auto hostifTrapGroup = ensureHostifTrapGroup(queueId);
  auto attributes = makeHostifTrapAttributes(
      trapId, hostifTrapGroup->adapterKey(), priority, platform_);
  SaiHostifTrapTraits::AdapterHostKey k =
      GET_ATTR(HostifTrap, TrapType, attributes);
  auto& store = saiStore_->get<SaiHostifTrapTraits>();
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
    uint32_t queueId,
    uint16_t priority) {
  auto handleItr = handles_.find(trapId);
  if (handleItr == handles_.end()) {
    throw FbossError(
        "Attempted to change non-existent trap for rx reason: ",
        apache::thrift::util::enumName(trapId));
  }
  auto hostifTrapGroup = ensureHostifTrapGroup(queueId);
  auto attributes = makeHostifTrapAttributes(
      trapId, hostifTrapGroup->adapterKey(), priority, platform_);
  SaiHostifTrapTraits::AdapterHostKey k =
      GET_ATTR(HostifTrap, TrapType, attributes);
  auto& store = saiStore_->get<SaiHostifTrapTraits>();
  auto hostifTrap = store.setObject(k, attributes);

  handleItr->second->trap = hostifTrap;
  handleItr->second->trapGroup = hostifTrapGroup;
}

void SaiHostifManager::processQosDelta(
    const DeltaValue<ControlPlane>& controlPlaneDelta) {
  if (platform_->getAsic()->isSupported(HwAsic::Feature::QOS_MAP_GLOBAL)) {
    // Global QOS policy applies to all ports including the CPU port
    // nothing to do here
    return;
  }
  auto oldQos = controlPlaneDelta.getOld()->getQosPolicy();
  auto newQos = controlPlaneDelta.getNew()->getQosPolicy();
  if (oldQos != newQos) {
    if (newQos) {
      setQosPolicy();
    } else if (oldQos) {
      clearQosPolicy();
    }
  }
}

void SaiHostifManager::processRxReasonToQueueDelta(
    const DeltaValue<ControlPlane>& controlPlaneDelta) {
  auto& oldRxReasonToQueue = controlPlaneDelta.getOld()->getRxReasonToQueue();
  auto& newRxReasonToQueue = controlPlaneDelta.getNew()->getRxReasonToQueue();
  /*
   * RxReasonToQueue is an ordered list and the enum values dicatates the
   * priority of the traps. Lower the enum value, lower the priority.
   */
  for (auto index = 0; index < newRxReasonToQueue.size(); index++) {
    auto& newRxReasonEntry = newRxReasonToQueue[index];
    auto oldRxReasonEntry = std::find_if(
        oldRxReasonToQueue.begin(),
        oldRxReasonToQueue.end(),
        [newRxReasonEntry](const auto& rxReasonEntry) {
          return rxReasonEntry.rxReason_ref() ==
              newRxReasonEntry.rxReason_ref();
        });
    /*
     * Lower index must have higher priority.
     */
    auto priority = newRxReasonToQueue.size() - index;
    CHECK_GT(priority, 0);
    if (oldRxReasonEntry != oldRxReasonToQueue.end()) {
      /*
       * If old reason exists and does not match the index, priority of the trap
       * group needs to be updated.
       */
      auto oldIndex =
          std::distance(oldRxReasonToQueue.begin(), oldRxReasonEntry);
      if (oldIndex != index ||
          oldRxReasonEntry->queueId_ref() != newRxReasonEntry.queueId_ref()) {
        changeHostifTrap(
            *newRxReasonEntry.rxReason_ref(),
            *newRxReasonEntry.queueId_ref(),
            priority);
      }
    } else {
      if (newRxReasonEntry.rxReason_ref() == cfg::PacketRxReason::UNMATCHED) {
        // what is the trap for unmatched?
        XLOG(WARN) << "ignoring UNMATCHED packet rx reason";
        continue;
      }
      addHostifTrap(
          *newRxReasonEntry.rxReason_ref(),
          *newRxReasonEntry.queueId_ref(),
          priority);
    }
  }

  for (auto index = 0; index < oldRxReasonToQueue.size(); index++) {
    auto& oldRxReasonEntry = oldRxReasonToQueue[index];
    auto newRxReasonEntry = std::find_if(
        newRxReasonToQueue.begin(),
        newRxReasonToQueue.end(),
        [&](const auto& rxReasonEntry) {
          return rxReasonEntry.rxReason_ref() ==
              oldRxReasonEntry.rxReason_ref();
        });
    if (newRxReasonEntry == newRxReasonToQueue.end()) {
      removeHostifTrap(*oldRxReasonEntry.rxReason_ref());
    }
  }
}

void SaiHostifManager::processQueueDelta(
    const DeltaValue<ControlPlane>& controlPlaneDelta) {
  auto& oldQueueConfig = controlPlaneDelta.getOld()->getQueues();
  auto& newQueueConfig = controlPlaneDelta.getNew()->getQueues();
  changeCpuQueue(oldQueueConfig, newQueueConfig);
}

void SaiHostifManager::processHostifDelta(
    const DeltaValue<ControlPlane>& controlPlaneDelta) {
  // TODO: Can we have reason code to a queue mapping that does not have
  // corresponding sai queue oid for cpu port ?
  processRxReasonToQueueDelta(controlPlaneDelta);
  processQueueDelta(controlPlaneDelta);
  processQosDelta(controlPlaneDelta);
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
  cpuPortHandle_->configuredQueues.clear();

  const auto asic = platform_->getAsic();
  auto maxCpuQueues = getMaxCpuQueues();
  for (auto newPortQueue : newQueueConfig) {
    // Queue create or update
    if (newPortQueue->getID() > maxCpuQueues) {
      throw FbossError(
          "Queue ID : ",
          newPortQueue->getID(),
          " exceeds max supported CPU queues: ",
          maxCpuQueues);
    }
    SaiQueueConfig saiQueueConfig =
        std::make_pair(newPortQueue->getID(), newPortQueue->getStreamType());
    auto queueHandle = getQueueHandle(saiQueueConfig);
    newPortQueue->setReservedBytes(
        newPortQueue->getReservedBytes()
            ? *newPortQueue->getReservedBytes()
            : asic->getDefaultReservedBytes(
                  newPortQueue->getStreamType(), true /*cpu port*/));
    newPortQueue->setScalingFactor(
        newPortQueue->getScalingFactor()
            ? *newPortQueue->getScalingFactor()
            : asic->getDefaultScalingFactor(
                  newPortQueue->getStreamType(), true /*cpu port*/));
    managerTable_->queueManager().changeQueue(queueHandle, *newPortQueue);
    auto queueName = newPortQueue->getName()
        ? *newPortQueue->getName()
        : folly::to<std::string>("cpuQueue", newPortQueue->getID());
    cpuStats_.queueChanged(newPortQueue->getID(), queueName);
    cpuPortHandle_->configuredQueues.push_back(queueHandle);
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
  cpuPortHandle_->cpuPortId = managerTable_->switchManager().getCpuPort();
  loadCpuPortQueues();
}

void SaiHostifManager::updateStats() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  HwPortStats cpuQueueStats;
  managerTable_->queueManager().updateStats(
      cpuPortHandle_->configuredQueues, cpuQueueStats);
  cpuStats_.updateStats(cpuQueueStats, now);
}

HwPortStats SaiHostifManager::getCpuPortStats() const {
  HwPortStats hwPortStats;
  managerTable_->queueManager().getStats(cpuPortHandle_->queues, hwPortStats);
  return hwPortStats;
}

uint32_t SaiHostifManager::getMaxCpuQueues() const {
  auto asic = platform_->getAsic();
  auto cpuQueueTypes = asic->getQueueStreamTypes(true /*cpu*/);
  CHECK_EQ(cpuQueueTypes.size(), 1);
  return asic->getDefaultNumPortQueues(*cpuQueueTypes.begin(), true /*cpu*/);
}

QueueConfig SaiHostifManager::getQueueSettings() const {
  if (!cpuPortHandle_) {
    return QueueConfig{};
  }
  auto queueConfig =
      managerTable_->queueManager().getQueueSettings(cpuPortHandle_->queues);
  auto maxCpuQueues = getMaxCpuQueues();
  QueueConfig filteredQueueConfig;
  // Prepare queue config only upto max CPU queues
  std::copy_if(
      queueConfig.begin(),
      queueConfig.end(),
      std::back_inserter(filteredQueueConfig),
      [maxCpuQueues](const auto& portQueue) {
        return portQueue->getID() < maxCpuQueues;
      });
  return filteredQueueConfig;
}

SaiHostifTrapHandle* SaiHostifManager::getHostifTrapHandleImpl(
    cfg::PacketRxReason rxReason) const {
  auto itr = handles_.find(rxReason);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiHostifTrapHandle";
  }
  return itr->second.get();
}

const SaiHostifTrapHandle* SaiHostifManager::getHostifTrapHandle(
    cfg::PacketRxReason rxReason) const {
  return getHostifTrapHandleImpl(rxReason);
}

SaiHostifTrapHandle* SaiHostifManager::getHostifTrapHandle(
    cfg::PacketRxReason rxReason) {
  return getHostifTrapHandleImpl(rxReason);
}

void SaiHostifManager::setQosPolicy() {
  auto& qosMapManager = managerTable_->qosMapManager();
  XLOG(INFO) << "Set cpu qos map";
  // We allow only a single QoS policy right now, so
  // pick that up.
  auto qosMapHandle = qosMapManager.getQosMap();
  globalDscpToTcQosMap_ = qosMapHandle->dscpQosMap;
  globalTcToQueueQosMap_ = qosMapHandle->tcQosMap;
  setCpuQosPolicy(
      globalDscpToTcQosMap_->adapterKey(),
      globalTcToQueueQosMap_->adapterKey());
}

void SaiHostifManager::clearQosPolicy() {
  setCpuQosPolicy(
      QosMapSaiId(SAI_NULL_OBJECT_ID), QosMapSaiId(SAI_NULL_OBJECT_ID));
  globalDscpToTcQosMap_.reset();
  globalTcToQueueQosMap_.reset();
}

void SaiHostifManager::setCpuQosPolicy(
    QosMapSaiId dscpToTc,
    QosMapSaiId tcToQueue) {
  auto& portApi = SaiApiTable::getInstance()->portApi();
  portApi.setAttribute(
      cpuPortHandle_->cpuPortId,
      SaiPortTraits::Attributes::QosDscpToTcMap{dscpToTc});
  portApi.setAttribute(
      cpuPortHandle_->cpuPortId,
      SaiPortTraits::Attributes::QosTcToQueueMap{tcToQueue});
}

} // namespace facebook::fboss
