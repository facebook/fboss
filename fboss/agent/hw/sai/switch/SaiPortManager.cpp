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
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <folly/logging/xlog.h>

#include <chrono>

using namespace std::chrono;

namespace facebook::fboss {
namespace {
void fillHwPortStats(
    const std::vector<sai_stat_id_t>& counterIds,
    const std::vector<sai_object_id_t>& counters,
    HwPortStats& hwPortStats) {
  auto index = 0;
  if (counters.size() != counterIds.size()) {
    throw FbossError("port counter size does not match counter id size");
  }
  // TODO fill these in when we have debug counte support in SAI
  hwPortStats.inDstNullDiscards__ref() = 0;
  for (auto counterId : counterIds) {
    switch (counterId) {
      case SAI_PORT_STAT_IF_IN_OCTETS:
        *hwPortStats.inBytes__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_UCAST_PKTS:
        *hwPortStats.inUnicastPkts__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_MULTICAST_PKTS:
        *hwPortStats.inMulticastPkts__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_BROADCAST_PKTS:
        *hwPortStats.inBroadcastPkts__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_DISCARDS:
        // Fill into inDiscards raw, we will then compute
        // inDiscards by subtracting dst null and in pause
        // discards from these
        *hwPortStats.inDiscardsRaw__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_IN_ERRORS:
        *hwPortStats.inErrors__ref() = counters[index];
        break;
      case SAI_PORT_STAT_PAUSE_RX_PKTS:
        *hwPortStats.inPause__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_OCTETS:
        *hwPortStats.outBytes__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_UCAST_PKTS:
        *hwPortStats.outUnicastPkts__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_MULTICAST_PKTS:
        *hwPortStats.outMulticastPkts__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_BROADCAST_PKTS:
        *hwPortStats.outBroadcastPkts__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_DISCARDS:
        *hwPortStats.outDiscards__ref() = counters[index];
        break;
      case SAI_PORT_STAT_IF_OUT_ERRORS:
        *hwPortStats.outErrors__ref() = counters[index];
        break;
      case SAI_PORT_STAT_PAUSE_TX_PKTS:
        *hwPortStats.outPause__ref() = counters[index];
        break;
      case SAI_PORT_STAT_ECN_MARKED_PACKETS:
        *hwPortStats.outEcnCounter__ref() = counters[index];
        break;
      default:
        throw FbossError("Got unexpected port counter id: ", counterId);
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
  auto saiPort = portStore.setObject(portKey, attributes, swPort->getID());
  handle->port = saiPort;
  handle->bridgePort = managerTable_->bridgeManager().addBridgePort(
      swPort->getID(), saiPort->adapterKey());
  loadPortQueues(handle.get());
  managerTable_->queueManager().ensurePortQueueConfig(
      saiPort->adapterKey(), handle->queues, swPort->getPortQueues());
  handles_.emplace(swPort->getID(), std::move(handle));
  if (swPort->isEnabled()) {
    portStats_.emplace(
        swPort->getID(), std::make_unique<HwPortFb303Stats>(swPort->getName()));
  }
  if (globalDscpToTcQosMap_) {
    // Both global maps must exist in one of them exists
    CHECK(globalTcToQueueQosMap_);
    setQosMaps(
        globalDscpToTcQosMap_->adapterKey(),
        globalTcToQueueQosMap_->adapterKey(),
        {swPort->getID()});
  }
  concurrentIndices_->portIds.emplace(saiPort->adapterKey(), swPort->getID());
  concurrentIndices_->vlanIds.emplace(
      saiPort->adapterKey(), swPort->getIngressVlan());
  return saiPort->adapterKey();
}

void SaiPortManager::removePort(const std::shared_ptr<Port>& swPort) {
  auto swId = swPort->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Attempted to remove non-existent port: ", swId);
  }
  if (globalDscpToTcQosMap_) {
    setQosMaps(
        QosMapSaiId(SAI_NULL_OBJECT_ID),
        QosMapSaiId(SAI_NULL_OBJECT_ID),
        {swPort->getID()});
  }
  concurrentIndices_->portIds.erase(itr->second->port->adapterKey());
  concurrentIndices_->vlanIds.erase(itr->second->port->adapterKey());
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
  auto pitr = portStats_.find(swId);
  for (auto newPortQueue : newQueueConfig) {
    // Queue create or update
    SaiQueueConfig saiQueueConfig =
        std::make_pair(newPortQueue->getID(), newPortQueue->getStreamType());
    auto queueHandle = getQueueHandle(swId, saiQueueConfig);
    managerTable_->queueManager().changeQueue(queueHandle, *newPortQueue);
    auto queueName = newPortQueue->getName()
        ? *newPortQueue->getName()
        : folly::to<std::string>("queue", newPortQueue->getID());
    if (pitr != portStats_.end()) {
      // Port stats map is sparse, since we don't maintain/publish stats
      // for disabled ports
      pitr->second->queueChanged(newPortQueue->getID(), queueName);
    }
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
      portHandle->queues.erase(saiQueueConfig);
      if (pitr != portStats_.end()) {
        // Port stats map is sparse, since we don't maintain/publish stats
        // for disabled ports
        pitr->second->queueRemoved(oldPortQueue->getID());
      }
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
  SaiPortTraits::CreateAttributes oldAttributes = attributesFromSwPort(oldPort);
  SaiPortTraits::CreateAttributes newAttributes = attributesFromSwPort(newPort);
  if (std::get<SaiPortTraits::Attributes::HwLaneList>(oldAttributes) !=
      std::get<SaiPortTraits::Attributes::HwLaneList>(newAttributes)) {
    // create only attribute has changed, this means delete old one and recreate
    // new one.
    XLOG(INFO) << "lanes changed for " << oldPort->getID();
    removePort(oldPort);
    addPort(newPort);
    return;
  }

  SaiPortTraits::AdapterHostKey portKey{
      GET_ATTR(Port, HwLaneList, newAttributes)};
  auto& portStore = SaiStore::getInstance()->get<SaiPortTraits>();
  portStore.setObject(portKey, newAttributes, newPort->getID());
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
  changeQueue(
      newPort->getID(), oldPort->getPortQueues(), newPort->getPortQueues());
}

SaiPortTraits::CreateAttributes SaiPortManager::attributesFromSwPort(
    const std::shared_ptr<Port>& swPort) const {
  bool adminState =
      swPort->getAdminState() == cfg::PortState::ENABLED ? true : false;
  auto profileID = swPort->getProfileID();
  auto portProfileConfig = platform_->getPortProfileConfig(profileID);
  if (!portProfileConfig) {
    throw FbossError(
        "port profile config with id ",
        profileID,
        " not found for port ",
        swPort->getID());
  }
  auto speed = static_cast<uint32_t>(portProfileConfig->speed);
  auto platformPort = platform_->getPort(swPort->getID());
  auto hwLaneList =
      platformPort->getHwPortLanes(static_cast<cfg::PortSpeed>(speed));
  auto globalFlowControlMode = utility::getSaiPortPauseMode(swPort->getPause());
  auto internalLoopbackMode =
      utility::getSaiPortInternalLoopbackMode(swPort->getLoopbackMode());
  auto mediaType = utility::getSaiPortMediaType(
      platformPort->getTransmitterTech(), static_cast<cfg::PortSpeed>(speed));
  auto phyFecMode = platform_->getPhyFecMode(profileID);
  auto fecMode = utility::getSaiPortFecMode(phyFecMode);
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
                                         swPort->getMaxFrameSize(),
                                         std::nullopt,
                                         std::nullopt};
}

std::shared_ptr<Port> SaiPortManager::swPortFromAttributes(
    SaiPortTraits::CreateAttributes attributes) const {
  auto speed = static_cast<cfg::PortSpeed>(GET_ATTR(Port, Speed, attributes));
  auto lanes = GET_ATTR(Port, HwLaneList, attributes);
  auto portID = platform_->findPortID(speed, lanes);
  auto platformPort = platform_->getPort(portID);
  auto port = std::make_shared<Port>(portID, folly::to<std::string>(portID));

  // speed, hw lane list, fec mode
  port->setProfileId(platformPort->getProfileIDBySpeed(speed));

  // admin state
  bool isEnabled = GET_OPT_ATTR(Port, AdminState, attributes);
  port->setAdminState(
      isEnabled ? cfg::PortState::ENABLED : cfg::PortState::DISABLED);

  // flow control mode
  cfg::PortPause pause;
  auto globalFlowControlMode =
      GET_OPT_ATTR(Port, GlobalFlowControlMode, attributes);
  pause.rx_ref() =
      (globalFlowControlMode == SAI_PORT_FLOW_CONTROL_MODE_BOTH_ENABLE ||
       globalFlowControlMode == SAI_PORT_FLOW_CONTROL_MODE_RX_ONLY);
  pause.tx_ref() =
      (globalFlowControlMode == SAI_PORT_FLOW_CONTROL_MODE_BOTH_ENABLE ||
       globalFlowControlMode == SAI_PORT_FLOW_CONTROL_MODE_TX_ONLY);
  port->setPause(pause);

  // vlan id
  auto vlan = GET_OPT_ATTR(Port, PortVlanId, attributes);
  port->setIngressVlan(static_cast<VlanID>(vlan));

  auto lbMode = GET_OPT_ATTR(Port, InternalLoopbackMode, attributes);
  port->setLoopbackMode(utility::getCfgPortInternalLoopbackMode(
      static_cast<sai_port_internal_loopback_mode_t>(lbMode)));

  // TODO: support Preemphasis once it is also used

  // mtu
  port->setMaxFrameSize(GET_OPT_ATTR(Port, Mtu, attributes));

  return port;
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

const std::vector<sai_stat_id_t>& SaiPortManager::supportedStats() const {
  static std::vector<sai_stat_id_t> counterIds;
  if (counterIds.size()) {
    return counterIds;
  }
  auto ecnSupported = platform_->getAsic()->isSupported(HwAsic::Feature::ECN);
  counterIds.resize(
      ecnSupported ? SaiPortTraits::CounterIds.size()
                   : SaiPortTraits::CounterIds.size() - 1);
  std::copy_if(
      SaiPortTraits::CounterIds.begin(),
      SaiPortTraits::CounterIds.end(),
      counterIds.begin(),
      [ecnSupported](auto statId) {
        return ecnSupported || statId != SAI_PORT_STAT_ECN_MARKED_PACKETS;
      });
  return counterIds;
}

void SaiPortManager::updateStats() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  for (const auto& [portId, handle] : handles_) {
    auto pitr = portStats_.find(portId);
    if (pitr == portStats_.end()) {
      // We don't maintain port stats for disabled ports.
      continue;
    }
    const auto& prevPortStats = pitr->second->portStats();
    HwPortStats curPortStats{prevPortStats};
    // All stats start with a unitialized (-1) value. If there are no in
    // discards (first collection) we will just report that -1 as the monotonic
    // counter. Instead set it to 0 if uninintialized
    *curPortStats.inDiscards__ref() = *curPortStats.inDiscards__ref() ==
            hardware_stats_constants::STAT_UNINITIALIZED()
        ? 0
        : *curPortStats.inDiscards__ref();
    handle->port->updateStats(supportedStats());
    const auto& counters = handle->port->getStats();
    fillHwPortStats(supportedStats(), counters, curPortStats);
    std::vector<utility::CounterPrevAndCur> toSubtractFromInDiscardsRaw = {
        {*prevPortStats.inDstNullDiscards__ref(),
         *curPortStats.inDstNullDiscards__ref()},
        {*prevPortStats.inPause__ref(), *curPortStats.inPause__ref()}};
    *curPortStats.inDiscards__ref() += utility::subtractIncrements(
        {*prevPortStats.inDiscardsRaw__ref(),
         *curPortStats.inDiscardsRaw__ref()},
        toSubtractFromInDiscardsRaw);
    managerTable_->queueManager().updateStats(handle->queues, curPortStats);
    portStats_[portId]->updateStats(curPortStats, now);
  }
}

std::map<PortID, HwPortStats> SaiPortManager::getPortStats() const {
  std::map<PortID, HwPortStats> portStats;
  for (const auto& [portId, handle] : handles_) {
    if (portStats_.find(portId) == portStats_.end()) {
      // We don't maintain port stats for disabled ports.
      continue;
    }
    const auto& counters = handle->port->getStats();
    std::ignore = platform_->getAsic()->isSupported(HwAsic::Feature::ECN);
    HwPortStats hwPortStats{};
    fillHwPortStats(supportedStats(), counters, hwPortStats);
    managerTable_->queueManager().getStats(handle->queues, hwPortStats);
    portStats.emplace(portId, hwPortStats);
  }
  return portStats;
}

void SaiPortManager::clearStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) const {
  auto statsToClear = supportedStats();
  for (auto port : *ports) {
    getPortHandle(PortID(port))->port->clearStats(statsToClear);
  }
}

const HwPortFb303Stats* SaiPortManager::getLastPortStat(PortID port) const {
  auto pitr = portStats_.find(port);
  return pitr != portStats_.end() ? pitr->second.get() : nullptr;
}

cfg::PortSpeed SaiPortManager::getMaxSpeed(PortID port) const {
  // TODO (srikrishnagopu): Use the read-only attribute
  // SAI_PORT_ATTR_SUPPORTED_SPEED to query the list of supported speeds
  // and return the maximum supported speed.
  return platform_->getPortMaxSpeed(port);
}

std::shared_ptr<PortMap> SaiPortManager::reconstructPortsFromStore() const {
  auto& portStore = SaiStore::getInstance()->get<SaiPortTraits>();
  auto portMap = std::make_shared<PortMap>();
  for (auto& iter : portStore) {
    auto saiPort = iter.second.lock();
    auto port = swPortFromAttributes(saiPort->attributes());
    portMap->addNode(port);
  }
  return portMap;
}

void SaiPortManager::setQosMaps(
    QosMapSaiId dscpToTc,
    QosMapSaiId tcToQueue,
    const folly::F14FastSet<PortID>& ports) {
  for (auto& portIdAndHandle : handles_) {
    if (ports.find(portIdAndHandle.first) == ports.end()) {
      continue;
    }
    auto& port = portIdAndHandle.second->port;
    port->setOptionalAttribute(
        SaiPortTraits::Attributes::QosDscpToTcMap{dscpToTc});
    port->setOptionalAttribute(
        SaiPortTraits::Attributes::QosTcToQueueMap{tcToQueue});
  }
}

void SaiPortManager::setQosMapsOnAllPorts(
    QosMapSaiId dscpToTc,
    QosMapSaiId tcToQueue) {
  folly::F14FastSet<PortID> allPorts;
  for (const auto& portIdAndHandle : handles_) {
    allPorts.insert(portIdAndHandle.first);
  }
  setQosMaps(dscpToTc, tcToQueue, allPorts);
}

void SaiPortManager::addDefaultDataPlaneQosPolicy(
    const std::shared_ptr<QosPolicy>& newDefaultQosPolicy) {
  auto& qosMapManager = managerTable_->qosMapManager();
  XLOG(INFO) << "Set default qos map";
  qosMapManager.addQosMap(newDefaultQosPolicy);
  auto qosMapHandle = qosMapManager.getQosMap();
  globalDscpToTcQosMap_ = qosMapHandle->dscpQosMap;
  globalTcToQueueQosMap_ = qosMapHandle->tcQosMap;
  setQosMapsOnAllPorts(
      globalDscpToTcQosMap_->adapterKey(),
      globalTcToQueueQosMap_->adapterKey());
}

void SaiPortManager::removeDefaultDataPlaneQosPolicy(
    const std::shared_ptr<QosPolicy>& policy) {
  setQosMapsOnAllPorts(
      QosMapSaiId(SAI_NULL_OBJECT_ID), QosMapSaiId(SAI_NULL_OBJECT_ID));
  globalDscpToTcQosMap_.reset();
  globalTcToQueueQosMap_.reset();
}
} // namespace facebook::fboss
