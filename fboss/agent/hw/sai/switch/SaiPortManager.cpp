/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <folly/logging/xlog.h>

#include <chrono>

using namespace std::chrono;

namespace facebook::fboss {
namespace {
sai_port_flow_control_mode_t getSaiPortPauseMode(cfg::PortPause pause) {
  if (pause.tx && pause.rx) {
    return SAI_PORT_FLOW_CONTROL_MODE_BOTH_ENABLE;
  } else if (pause.tx) {
    return SAI_PORT_FLOW_CONTROL_MODE_TX_ONLY;
  } else if (pause.rx) {
    return SAI_PORT_FLOW_CONTROL_MODE_RX_ONLY;
  } else {
    return SAI_PORT_FLOW_CONTROL_MODE_DISABLE;
  }
}

sai_port_internal_loopback_mode_t getSaiPortInternalLoopbackMode(
    cfg::PortLoopbackMode loopbackMode) {
  switch (loopbackMode) {
    case cfg::PortLoopbackMode::NONE:
      return SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE;
    case cfg::PortLoopbackMode::PHY:
      return SAI_PORT_INTERNAL_LOOPBACK_MODE_PHY;
    case cfg::PortLoopbackMode::MAC:
      return SAI_PORT_INTERNAL_LOOPBACK_MODE_MAC;
    default:
      return SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE;
  }
}

sai_port_media_type_t getSaiPortMediaType(
    TransmitterTechnology transmitterTech) {
  switch (transmitterTech) {
    case TransmitterTechnology::COPPER:
      return SAI_PORT_MEDIA_TYPE_COPPER;
    case TransmitterTechnology::OPTICAL:
      return SAI_PORT_MEDIA_TYPE_FIBER;
    default:
      return SAI_PORT_MEDIA_TYPE_UNKNOWN;
  }
}

sai_port_fec_mode_t getSaiPortFecMode(phy::FecMode fec) {
  if (fec == phy::FecMode::CL91 || fec == phy::FecMode::CL74) {
    return SAI_PORT_FEC_MODE_FC;
  } else if (fec == phy::FecMode::RS528 || fec == phy::FecMode::RS544) {
    return SAI_PORT_FEC_MODE_RS;
  } else {
    return SAI_PORT_FEC_MODE_NONE;
  }
}

void fillHwPortStats(
    const std::vector<sai_object_id_t>& counters,
    HwPortStats& hwPortStats) {
  uint16_t index = 0;
  if (counters.size() != SaiPortTraits::CounterIds.size()) {
    throw FbossError("port counter size does not match counter id size");
  }
  for (auto counterId : SaiPortTraits::CounterIds) {
    switch (counterId) {
      case SAI_PORT_STAT_IF_IN_OCTETS:
        hwPortStats.inBytes_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_UCAST_PKTS:
        hwPortStats.inUnicastPkts_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_MULTICAST_PKTS:
        hwPortStats.inMulticastPkts_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_BROADCAST_PKTS:
        hwPortStats.inBroadcastPkts_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_DISCARDS:
        hwPortStats.inDiscards_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_ERRORS:
        hwPortStats.inErrors_ = counters[index];
        break;
      case SAI_PORT_STAT_PAUSE_RX_PKTS:
        hwPortStats.inPause_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_OCTETS:
        hwPortStats.outBytes_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_UCAST_PKTS:
        hwPortStats.outUnicastPkts_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_MULTICAST_PKTS:
        hwPortStats.outMulticastPkts_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_BROADCAST_PKTS:
        hwPortStats.outBroadcastPkts_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_DISCARDS:
        hwPortStats.outDiscards_ = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_ERRORS:
        hwPortStats.outErrors_ = counters[index];
        break;
      case SAI_PORT_STAT_PAUSE_TX_PKTS:
        hwPortStats.outPause_ = counters[index];
        break;
      default:
        throw FbossError("Got unexpected counter id: ", counterId);
    }
    index++;
  }
}
} // namespace

SaiPortManager::SaiPortManager(
    SaiManagerTable* managerTable,
    SaiPlatform* platform,
    ConcurrentIndices* concurrentIndices)
    : managerTable_(managerTable),
      platform_(platform),
      concurrentIndices_(concurrentIndices) {}

SaiPortManager::~SaiPortManager() {}

void SaiPortManager::loadPortQueues(SaiPortHandle* portHandle) {
  std::vector<sai_object_id_t> queueList;
  queueList.resize(1);
  SaiPortTraits::Attributes::QosQueueList queueListAttribute{queueList};
  auto queueSaiIdList = SaiApiTable::getInstance()->portApi().getAttribute(
      portHandle->port->adapterKey(), queueListAttribute);
  if (queueSaiIdList.size() == 0) {
    throw FbossError("no queues exist for port ");
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
  portHandle->queues = managerTable_->queueManager().loadQueues(
      portHandle->port->adapterKey(), queueSaiIds);
}

PortSaiId SaiPortManager::addPort(const std::shared_ptr<Port>& swPort) {
  SaiPortHandle* portHandle = getPortHandle(swPort->getID());
  if (portHandle) {
    throw FbossError(
        "Attempted to add port which already exists: ",
        swPort->getID(),
        " SAI id: ",
        portHandle->port->adapterKey());
  }
  SaiPortTraits::CreateAttributes attributes = attributesFromSwPort(swPort);
  SaiPortTraits::AdapterHostKey portKey{GET_ATTR(Port, HwLaneList, attributes)};
  auto handle = std::make_unique<SaiPortHandle>();

  auto& portStore = SaiStore::getInstance()->get<SaiPortTraits>();
  auto saiPort = portStore.setObject(portKey, attributes);
  handle->port = saiPort;
  handle->bridgePort =
      managerTable_->bridgeManager().addBridgePort(saiPort->adapterKey());
  loadPortQueues(handle.get());
  managerTable_->queueManager().ensurePortQueueConfig(
      saiPort->adapterKey(), handle->queues, swPort->getPortQueues());
  handles_.emplace(swPort->getID(), std::move(handle));
  if (swPort->isEnabled()) {
    portStats_.emplace(
        swPort->getID(), std::make_unique<HwPortFb303Stats>(swPort->getName()));
  }
  concurrentIndices_->portIds.emplace(saiPort->adapterKey(), swPort->getID());
  return saiPort->adapterKey();
}

void SaiPortManager::removePort(PortID swId) {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Attempted to remove non-existent port: ", swId);
  }
  concurrentIndices_->portIds.erase(itr->second->port->adapterKey());
  handles_.erase(itr);
  portStats_.erase(swId);
}

void SaiPortManager::changeQueue(
    PortID swId,
    const QueueConfig& oldQueueConfig,
    const QueueConfig& newQueueConfig) {
  SaiPortHandle* portHandle = getPortHandle(swId);
  if (!portHandle) {
    throw FbossError("Attempted to change non-existent port ");
  }

  for (auto newPortQueue : newQueueConfig) {
    // Queue create or update
    SaiQueueConfig saiQueueConfig =
        std::make_pair(newPortQueue->getID(), newPortQueue->getStreamType());
    auto queueHandle = getQueueHandle(swId, saiQueueConfig);
    managerTable_->queueManager().changeQueue(queueHandle, *newPortQueue);
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
      auto queueHandle = getQueueHandle(swId, saiQueueConfig);
      managerTable_->queueManager().resetQueue(queueHandle);
      portHandle->queues.erase(saiQueueConfig);
    }
  }
}

void SaiPortManager::changePort(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  SaiPortHandle* existingPort = getPortHandle(newPort->getID());
  if (!existingPort) {
    throw FbossError("Attempted to change non-existent port ");
  }
  SaiPortTraits::CreateAttributes attributes = attributesFromSwPort(newPort);
  SaiPortTraits::AdapterHostKey portKey{GET_ATTR(Port, HwLaneList, attributes)};
  auto& portStore = SaiStore::getInstance()->get<SaiPortTraits>();
  portStore.setObject(portKey, attributes);
  changeQueue(
      newPort->getID(), oldPort->getPortQueues(), newPort->getPortQueues());
  if (newPort->isEnabled()) {
    if (!oldPort->isEnabled()) {
      // Port transitioned from disabled to enabled, setup port stats
      portStats_.emplace(
          newPort->getID(),
          std::make_unique<HwPortFb303Stats>(newPort->getName()));
    } else if (oldPort->getName() != newPort->getName()) {
      // Port was already enabled, but Port name changed - update stats
      portStats_.find(newPort->getID())
          ->second->portNameChanged(newPort->getName());
    }
  } else if (oldPort->isEnabled()) {
    // Port transitioned from enabled to disabled, remove stats
    portStats_.erase(newPort->getID());
  }
}

SaiPortTraits::CreateAttributes SaiPortManager::attributesFromSwPort(
    const std::shared_ptr<Port>& swPort) const {
  bool adminState =
      swPort->getAdminState() == cfg::PortState::ENABLED ? true : false;
  uint32_t speed = static_cast<uint32_t>(swPort->getSpeed());
  auto platformPort = platform_->getPort(swPort->getID());
  auto hwLaneList = platformPort->getHwPortLanes(swPort->getSpeed());
  auto globalFlowControlMode = getSaiPortPauseMode(swPort->getPause());
  auto internalLoopbackMode =
      getSaiPortInternalLoopbackMode(swPort->getLoopbackMode());
  auto mediaType = getSaiPortMediaType(platformPort->getTransmitterTech());
  // TODO: Use getSaiPortFecMode once the platform config has fec mode
  auto fecMode = SAI_PORT_FEC_MODE_NONE;
  if (swPort->getFEC() == cfg::PortFEC::ON) {
    fecMode = SAI_PORT_FEC_MODE_RS;
  }

  uint16_t vlanId = swPort->getIngressVlan();
  return SaiPortTraits::CreateAttributes{hwLaneList,
                                         speed,
                                         adminState,
                                         fecMode,
                                         internalLoopbackMode,
                                         mediaType,
                                         globalFlowControlMode,
                                         vlanId,
                                         std::nullopt,
                                         swPort->getMaxFrameSize()};
}

// private const getter for use by const and non-const getters
SaiPortHandle* SaiPortManager::getPortHandleImpl(PortID swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiPortHandle for " << swId;
  }
  return itr->second.get();
}

const SaiPortHandle* SaiPortManager::getPortHandle(PortID swId) const {
  return getPortHandleImpl(swId);
}

SaiPortHandle* SaiPortManager::getPortHandle(PortID swId) {
  return getPortHandleImpl(swId);
}

// private const getter for use by const and non-const getters
SaiQueueHandle* SaiPortManager::getQueueHandleImpl(
    PortID swId,
    const SaiQueueConfig& saiQueueConfig) const {
  auto portHandle = getPortHandleImpl(swId);
  if (!portHandle) {
    XLOG(FATAL) << "Invalid null SaiPortHandle for " << swId;
  }
  auto itr = portHandle->queues.find(saiQueueConfig);
  if (itr == portHandle->queues.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiQueueHandle for " << swId;
  }
  return itr->second.get();
}

const SaiQueueHandle* SaiPortManager::getQueueHandle(
    PortID swId,
    const SaiQueueConfig& saiQueueConfig) const {
  return getQueueHandleImpl(swId, saiQueueConfig);
}

SaiQueueHandle* SaiPortManager::getQueueHandle(
    PortID swId,
    const SaiQueueConfig& saiQueueConfig) {
  return getQueueHandleImpl(swId, saiQueueConfig);
}

void SaiPortManager::processPortDelta(const StateDelta& stateDelta) {
  auto delta = stateDelta.getPortsDelta();
  auto processChanged = [this](const auto& oldPort, const auto& newPort) {
    changePort(oldPort, newPort);
  };
  auto processAdded = [this](const auto& newPort) { addPort(newPort); };
  auto processRemoved = [this](const auto& oldPort) {
    removePort(oldPort->getID());
  };
  DeltaFunctions::forEachChanged(
      delta, processChanged, processAdded, processRemoved);
}

void SaiPortManager::updateStats() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  for (const auto& [portId, handle] : handles_) {
    handle->port->updateStats();
    const auto& counters = handle->port->getStats();
    HwPortStats hwPortStats;
    fillHwPortStats(counters, hwPortStats);
    managerTable_->queueManager().updateStats(handle->queues, hwPortStats);
    portStats_[portId]->updateStats(hwPortStats, now);
  }
}

std::map<PortID, HwPortStats> SaiPortManager::getPortStats() const {
  std::map<PortID, HwPortStats> portStats;
  for (const auto& [portId, handle] : handles_) {
    const auto& counters = handle->port->getStats();
    HwPortStats hwPortStats;
    fillHwPortStats(counters, hwPortStats);
    managerTable_->queueManager().getStats(handle->queues, hwPortStats);
    portStats.emplace(portId, hwPortStats);
  }
  return portStats;
}

} // namespace facebook::fboss
