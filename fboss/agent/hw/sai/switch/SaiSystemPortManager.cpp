/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/HwSysPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

#include <fmt/ranges.h>

using namespace std::chrono;

namespace facebook::fboss {
namespace {
bool createOnlyAttributesChanged(
    const SaiSystemPortTraits::CreateAttributes& oldAttrs,
    const SaiSystemPortTraits::CreateAttributes& newAttrs) {
  return GET_ATTR(SystemPort, ConfigInfo, oldAttrs) !=
      GET_ATTR(SystemPort, ConfigInfo, newAttrs);
}
} // namespace
SaiSystemPortManager::SaiSystemPortManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform,
    ConcurrentIndices* concurrentIndices)
    : saiStore_(saiStore),
      managerTable_(managerTable),
      platform_(platform),
      concurrentIndices_(concurrentIndices),
      tcToQueueMapAllowedOnSystemPort_(platform_->getAsic()->isSupported(
          HwAsic::Feature::TC_TO_QUEUE_QOS_MAP_ON_SYSTEM_PORT)) {}

SaiSystemPortManager::~SaiSystemPortManager() {}

SaiSystemPortTraits::CreateAttributes
SaiSystemPortManager::attributesFromSwSystemPort(
    const std::shared_ptr<SystemPort>& swSystemPort) const {
  sai_system_port_config_t config{
      .port_id = static_cast<uint32_t>(swSystemPort->getID()),
      .attached_switch_id = static_cast<uint32_t>(swSystemPort->getSwitchId()),
      .attached_core_index =
          static_cast<uint32_t>(swSystemPort->getCoreIndex()),
      .attached_core_port_index =
          static_cast<uint32_t>(swSystemPort->getCorePortIndex()),
      .speed = static_cast<uint32_t>(swSystemPort->getSpeedMbps()),
      .num_voq = static_cast<uint32_t>(swSystemPort->getNumVoqs()),
  };
  return SaiSystemPortTraits::CreateAttributes{
      config, swSystemPort->getEnabled(), std::nullopt};
}

SystemPortSaiId SaiSystemPortManager::addSystemPort(
    const std::shared_ptr<SystemPort>& swSystemPort) {
  auto systemPortHandle = getSystemPortHandle(swSystemPort->getID());
  if (systemPortHandle) {
    throw FbossError(
        "Attempted to add systemPort which already exists: ",
        swSystemPort->getID(),
        " SAI id: ",
        systemPortHandle->systemPort->adapterKey());
  }
  SaiSystemPortTraits::CreateAttributes attributes =
      attributesFromSwSystemPort(swSystemPort);

  if (swSystemPort->getEnabled()) {
    portStats_.emplace(
        swSystemPort->getID(),
        std::make_unique<HwSysPortFb303Stats>(swSystemPort->getPortName()));
  }
  auto handle = std::make_unique<SaiSystemPortHandle>();

  auto& systemPortStore = saiStore_->get<SaiSystemPortTraits>();
  SaiSystemPortTraits::AdapterHostKey systemPortKey{
      GET_ATTR(SystemPort, ConfigInfo, attributes)};
  auto saiSystemPort = systemPortStore.setObject(
      systemPortKey, attributes, swSystemPort->getID());
  handle->systemPort = saiSystemPort;
  loadQueues(*handle, swSystemPort);
  handles_.emplace(swSystemPort->getID(), std::move(handle));
  concurrentIndices_->sysPortIds.insert(
      {saiSystemPort->adapterKey(), swSystemPort->getID()});
  concurrentIndices_->sysPortSaiIds.insert(
      {swSystemPort->getID(), saiSystemPort->adapterKey()});
  return saiSystemPort->adapterKey();
}

void SaiSystemPortManager::changeSystemPort(
    const std::shared_ptr<SystemPort>& oldSystemPort,
    const std::shared_ptr<SystemPort>& newSystemPort) {
  CHECK_EQ(oldSystemPort->getID(), newSystemPort->getID());
  auto handle = getSystemPortHandleImpl(newSystemPort->getID());
  auto newAttributes = attributesFromSwSystemPort(newSystemPort);
  if (createOnlyAttributesChanged(
          handle->systemPort->attributes(), newAttributes)) {
    removeSystemPort(oldSystemPort);
    addSystemPort(newSystemPort);
  } else {
    handle->systemPort->setAttributes(newAttributes);
    if (newSystemPort->getEnabled()) {
      if (!oldSystemPort->getEnabled()) {
        // Port transitioned from disabled to enabled, setup port stats
        portStats_.emplace(
            newSystemPort->getID(),
            std::make_unique<HwSysPortFb303Stats>(
                newSystemPort->getPortName()));
        setupVoqStats(newSystemPort);
      } else if (oldSystemPort->getPortName() != newSystemPort->getPortName()) {
        // Port was already enabled, but Port name changed - update stats
        portStats_.find(newSystemPort->getID())
            ->second->portNameChanged(newSystemPort->getPortName());
      }
    } else if (oldSystemPort->getEnabled()) {
      // Port transitioned from enabled to disabled, remove stats
      portStats_.erase(newSystemPort->getID());
    }
    // TODO:
    // Compare qos queues changing and if so update qosmap
    // and port queue stats.
    // Note that we don't need to worry about numVoqs changing
    // since that's a create only attribute, which upon change
    // would cause a remove + add sys port sequence
  }
}

const HwSysPortFb303Stats* SaiSystemPortManager::getLastPortStats(
    SystemPortID port) const {
  auto pitr = portStats_.find(port);
  return pitr != portStats_.end() ? pitr->second.get() : nullptr;
}

void SaiSystemPortManager::setupVoqStats(
    const std::shared_ptr<SystemPort>& swSystemPort) {
  auto pitr = portStats_.find(swSystemPort->getID());
  if (pitr != portStats_.end()) {
    for (auto i = 0; i < swSystemPort->getNumVoqs(); ++i) {
      // TODO pull name from qos config
      auto queueName = folly::to<std::string>("queue", i);
      pitr->second->queueChanged(i, queueName);
    }
  }
}

void SaiSystemPortManager::loadQueues(
    SaiSystemPortHandle& sysPortHandle,
    const std::shared_ptr<SystemPort>& swSystemPort) {
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::VOQ)) {
    return;
  }
  std::vector<sai_object_id_t> queueList;
  queueList.resize(swSystemPort->getNumVoqs());
  SaiSystemPortTraits::Attributes::QosVoqList queueListAttribute{queueList};
  auto queueSaiIdList =
      SaiApiTable::getInstance()->systemPortApi().getAttribute(
          sysPortHandle.systemPort->adapterKey(), queueListAttribute);
  if (queueSaiIdList.size() == 0) {
    throw FbossError("no queues exist for system port ");
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
  sysPortHandle.queues = managerTable_->queueManager().loadQueues(queueSaiIds);
  setupVoqStats(swSystemPort);
}

void SaiSystemPortManager::removeSystemPort(
    const std::shared_ptr<SystemPort>& swSystemPort) {
  auto swId = swSystemPort->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Attempted to remove non-existent system port: ", swId);
  }
  concurrentIndices_->sysPortSaiIds.erase(swId);
  concurrentIndices_->sysPortIds.erase(itr->second->systemPort->adapterKey());
  handles_.erase(itr);
  portStats_.erase(swId);
  XLOG(DBG2) << "removed system port: " << swId;
}

// private const getter for use by const and non-const getters
SaiSystemPortHandle* SaiSystemPortManager::getSystemPortHandleImpl(
    SystemPortID swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiSystemPortHandle for " << swId;
  }
  return itr->second.get();
}

void SaiSystemPortManager::updateStats(
    SystemPortID portId,
    bool updateWatermarks) {
  auto handlesItr = handles_.find(portId);
  if (handlesItr == handles_.end()) {
    return;
  }
  auto* handle = handlesItr->second.get();
  // TODO - only populate queues that show up in qos config
  std::vector<SaiQueueHandle*> configuredQueues;
  for (auto& confAndQueueHandle : handle->queues) {
    configuredQueues.push_back(confAndQueueHandle.second.get());
  }

  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  auto portStatItr = portStats_.find(portId);
  if (portStatItr == portStats_.end()) {
    // We don't maintain port stats for disabled ports.
    return;
  }
  const auto& prevPortStats = portStatItr->second->portStats();
  HwSysPortStats curPortStats{prevPortStats};
  managerTable_->queueManager().updateStats(
      configuredQueues, curPortStats, updateWatermarks);
  portStats_[portId]->updateStats(curPortStats, now);
}
std::shared_ptr<SystemPortMap> SaiSystemPortManager::constructSystemPorts(
    const std::shared_ptr<PortMap>& ports,
    int64_t switchId,
    std::optional<cfg::Range64> systemPortRange) {
  auto sysPortMap = std::make_shared<SystemPortMap>();
  CHECK(systemPortRange);
  const std::set<cfg::PortType> kCreateSysPortsFor = {
      cfg::PortType::INTERFACE_PORT, cfg::PortType::RECYCLE_PORT};
  for (const auto& port : std::as_const(*ports)) {
    if (kCreateSysPortsFor.find(port.second->getPortType()) ==
        kCreateSysPortsFor.end()) {
      continue;
    }
    auto sysPort = std::make_shared<SystemPort>(
        SystemPortID{*systemPortRange->minimum() + port.second->getID()});
    sysPort->setSwitchId(SwitchID(switchId));
    sysPort->setPortName(
        folly::sformat("{}:{}", switchId, port.second->getName()));
    auto platformPort = platform_->getPlatformPort(port.second->getID());
    sysPort->setCoreIndex(*platformPort->getAttachedCoreId());
    sysPort->setCorePortIndex(*platformPort->getCorePortIndex());
    sysPort->setSpeedMbps(static_cast<int>(port.second->getSpeed()));
    sysPort->setNumVoqs(8);
    sysPort->setEnabled(port.second->isEnabled());
    sysPort->setQosPolicy(port.second->getQosPolicy());
    sysPortMap->addSystemPort(std::move(sysPort));
  }

  return sysPortMap;
}

void SaiSystemPortManager::setQosMapOnAllSystemPorts(QosMapSaiId qosMapId) {
  for (const auto& systemPortIdAndHandle : handles_) {
    systemPortIdAndHandle.second->systemPort->setOptionalAttribute(
        SaiSystemPortTraits::Attributes::QosTcToQueueMap{qosMapId});
  }
}

void SaiSystemPortManager::setQosPolicy() {
  if (platform_->getAsic()->getSwitchType() != cfg::SwitchType::VOQ) {
    return;
  }
  if (!tcToQueueMapAllowedOnSystemPort_) {
    return;
  }
  auto& qosMapManager = managerTable_->qosMapManager();
  auto qosMapHandle = qosMapManager.getQosMap();
  globalTcToQueueQosMap_ = qosMapHandle->tcToQueueMap;
  setQosMapOnAllSystemPorts(globalTcToQueueQosMap_->adapterKey());
}

void SaiSystemPortManager::clearQosPolicy() {
  if (platform_->getAsic()->getSwitchType() != cfg::SwitchType::VOQ) {
    return;
  }
  if (!tcToQueueMapAllowedOnSystemPort_) {
    return;
  }
  setQosMapOnAllSystemPorts(QosMapSaiId(SAI_NULL_OBJECT_ID));
  globalTcToQueueQosMap_.reset();
}

void SaiSystemPortManager::resetQosMaps() {
  if (!tcToQueueMapAllowedOnSystemPort_) {
    return;
  }
  setQosMapOnAllSystemPorts(QosMapSaiId(SAI_NULL_OBJECT_ID));
}

} // namespace facebook::fboss
