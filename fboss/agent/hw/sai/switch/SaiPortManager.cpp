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
#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"
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
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    const SaiDebugCounterManager& debugCounterManager,
    HwPortStats& hwPortStats) {
  // TODO fill these in when we have debug counter support in SAI
  hwPortStats.inDstNullDiscards__ref() = 0;
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
      case SAI_PORT_STAT_IF_IN_OCTETS:
        hwPortStats.inBytes__ref() = value;
        break;
      case SAI_PORT_STAT_IF_IN_UCAST_PKTS:
        hwPortStats.inUnicastPkts__ref() = value;
        break;
      case SAI_PORT_STAT_IF_IN_MULTICAST_PKTS:
        hwPortStats.inMulticastPkts__ref() = value;
        break;
      case SAI_PORT_STAT_IF_IN_BROADCAST_PKTS:
        hwPortStats.inBroadcastPkts__ref() = value;
        break;
      case SAI_PORT_STAT_IF_IN_DISCARDS:
        // Fill into inDiscards raw, we will then compute
        // inDiscards by subtracting dst null and in pause
        // discards from these
        hwPortStats.inDiscardsRaw__ref() = value;
        break;
      case SAI_PORT_STAT_IF_IN_ERRORS:
        hwPortStats.inErrors__ref() = value;
        break;
      case SAI_PORT_STAT_PAUSE_RX_PKTS:
        hwPortStats.inPause__ref() = value;
        break;
      case SAI_PORT_STAT_IF_OUT_OCTETS:
        hwPortStats.outBytes__ref() = value;
        break;
      case SAI_PORT_STAT_IF_OUT_UCAST_PKTS:
        hwPortStats.outUnicastPkts__ref() = value;
        break;
      case SAI_PORT_STAT_IF_OUT_MULTICAST_PKTS:
        hwPortStats.outMulticastPkts__ref() = value;
        break;
      case SAI_PORT_STAT_IF_OUT_BROADCAST_PKTS:
        hwPortStats.outBroadcastPkts__ref() = value;
        break;
      case SAI_PORT_STAT_IF_OUT_DISCARDS:
        hwPortStats.outDiscards__ref() = value;
        break;
      case SAI_PORT_STAT_IF_OUT_ERRORS:
        hwPortStats.outErrors__ref() = value;
        break;
      case SAI_PORT_STAT_PAUSE_TX_PKTS:
        hwPortStats.outPause__ref() = value;
        break;
      case SAI_PORT_STAT_ECN_MARKED_PACKETS:
        hwPortStats.outEcnCounter__ref() = value;
        break;
      case SAI_PORT_STAT_WRED_DROPPED_PACKETS:
        hwPortStats.wredDroppedPackets__ref() = value;
        break;
      default:
        if (counterId ==
            debugCounterManager.getPortL3BlackHoleCounterStatId()) {
          hwPortStats.inDstNullDiscards__ref() = value;
        } else {
          throw FbossError("Got unexpected port counter id: ", counterId);
        }
        break;
    }
  }
}

sai_bridge_port_fdb_learning_mode_t getFdbLearningMode(
    cfg::L2LearningMode l2LearningMode) {
  sai_bridge_port_fdb_learning_mode_t fdbLearningMode;
  switch (l2LearningMode) {
    case cfg::L2LearningMode::HARDWARE:
      fdbLearningMode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;
      break;
    case cfg::L2LearningMode::SOFTWARE:
      fdbLearningMode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_FDB_NOTIFICATION;
      break;
  }
  return fdbLearningMode;
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
  removeRemovedHandleIf(swPort->getID());
  SaiPortTraits::CreateAttributes attributes = attributesFromSwPort(swPort);
  SaiPortTraits::AdapterHostKey portKey{GET_ATTR(Port, HwLaneList, attributes)};
  auto handle = std::make_unique<SaiPortHandle>();

  auto& portStore = SaiStore::getInstance()->get<SaiPortTraits>();
  auto saiPort = portStore.setObject(portKey, attributes, swPort->getID());
  handle->port = saiPort;
  handle->serdes = programSerdes(saiPort, swPort);

  handle->bridgePort = managerTable_->bridgeManager().addBridgePort(
      swPort->getID(), saiPort->adapterKey());
  loadPortQueues(handle.get());
  for (auto portQueue : swPort->getPortQueues()) {
    auto queueKey =
        std::make_pair(portQueue->getID(), portQueue->getStreamType());
    const auto& configuredQueue = handle->queues[queueKey];
    handle->configuredQueues.push_back(configuredQueue.get());
  }
  managerTable_->queueManager().ensurePortQueueConfig(
      saiPort->adapterKey(), handle->queues, swPort->getPortQueues());
  if (l2LearningMode_) {
    handle->bridgePort->setOptionalAttribute(
        SaiBridgePortTraits::Attributes::FdbLearningMode{
            getFdbLearningMode(l2LearningMode_.value())});
  }
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
  concurrentIndices_->portSaiIds.emplace(
      swPort->getID(), saiPort->adapterKey());
  concurrentIndices_->vlanIds.emplace(
      saiPort->adapterKey(), swPort->getIngressVlan());
  XLOG(INFO) << "added port " << swPort->getID() << " with vlan "
             << swPort->getIngressVlan();

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());
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
  concurrentIndices_->portSaiIds.erase(swId);
  concurrentIndices_->vlanIds.erase(itr->second->port->adapterKey());
  addRemovedHandle(itr->first);
  handles_.erase(itr);
  portStats_.erase(swId);
  // TODO: do FDB entries associated with this port need to be removed
  // now?
  XLOG(INFO) << "removed port " << swPort->getID() << " with vlan "
             << swPort->getIngressVlan();
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
  portHandle->configuredQueues.clear();
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
    portHandle->configuredQueues.push_back(queueHandle);
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
  auto saiPort = portStore.setObject(portKey, newAttributes, newPort->getID());
  // if vlan changed update it, this is important for rx processing
  if (newPort->getIngressVlan() != oldPort->getIngressVlan()) {
    concurrentIndices_->vlanIds.insert_or_assign(
        saiPort->adapterKey(), newPort->getIngressVlan());
    XLOG(INFO) << "changed vlan on port " << newPort->getID()
               << ": old vlan: " << oldPort->getIngressVlan()
               << ", new vlan: " << newPort->getIngressVlan();
  }
  if (newPort->getProfileID() != oldPort->getProfileID()) {
    auto platformPort = platform_->getPort(newPort->getID());
    platformPort->setCurrentProfile(newPort->getProfileID());
  }
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
  auto speed = *portProfileConfig->speed_ref();
  auto platformPort = platform_->getPort(swPort->getID());
  auto hwLaneList = platformPort->getHwPortLanes(speed);
  auto globalFlowControlMode = utility::getSaiPortPauseMode(swPort->getPause());
  auto internalLoopbackMode =
      utility::getSaiPortInternalLoopbackMode(swPort->getLoopbackMode());
  auto mediaType =
      utility::getSaiPortMediaType(platformPort->getTransmitterTech(), speed);

  auto enableFec =
      (speed >= cfg::PortSpeed::HUNDREDG) || !platformPort->shouldDisableFEC();

  SaiPortTraits::Attributes::FecMode fecMode;
  if (!enableFec) {
    fecMode = SAI_PORT_FEC_MODE_NONE;
  } else {
    auto phyFecMode = platform_->getPhyFecMode(profileID);
    fecMode = utility::getSaiPortFecMode(phyFecMode);
  }
  uint16_t vlanId = swPort->getIngressVlan();

  std::optional<SaiPortTraits::Attributes::InterfaceType> interfaceType{};
  if (auto saiInterfaceType = platform_->getInterfaceType(
          platformPort->getTransmitterTech(), speed)) {
    interfaceType = saiInterfaceType.value();
  }
  return SaiPortTraits::CreateAttributes{hwLaneList,
                                         static_cast<uint32_t>(speed),
                                         adminState,
                                         fecMode,
                                         internalLoopbackMode,
                                         mediaType,
                                         globalFlowControlMode,
                                         vlanId,
                                         std::nullopt,
                                         swPort->getMaxFrameSize(),
                                         std::nullopt,
                                         std::nullopt,
                                         std::nullopt,
                                         interfaceType};
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
  port->setSpeed(speed);

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
  std::set<sai_stat_id_t> countersToFilter;
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::ECN)) {
    countersToFilter.insert(SAI_PORT_STAT_ECN_MARKED_PACKETS);
  }
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::SAI_ECN_WRED)) {
    countersToFilter.insert(SAI_PORT_STAT_WRED_DROPPED_PACKETS);
  }
  counterIds.reserve(SaiPortTraits::CounterIdsToRead.size() + 1);
  std::copy_if(
      SaiPortTraits::CounterIdsToRead.begin(),
      SaiPortTraits::CounterIdsToRead.end(),
      std::back_inserter(counterIds),
      [&countersToFilter](auto statId) {
        return countersToFilter.find(statId) == countersToFilter.end();
      });
  if (platform_->getAsic()->isSupported(HwAsic::Feature::DEBUG_COUNTER)) {
    counterIds.emplace_back(
        managerTable_->debugCounterManager().getPortL3BlackHoleCounterStatId());
  }
  return counterIds;
}

void SaiPortManager::updateStats(PortID portId) {
  auto handlesItr = handles_.find(portId);
  if (handlesItr == handles_.end()) {
    return;
  }
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  auto* handle = handlesItr->second.get();
  auto portStatItr = portStats_.find(portId);
  if (portStatItr == portStats_.end()) {
    // We don't maintain port stats for disabled ports.
    return;
  }
  const auto& prevPortStats = portStatItr->second->portStats();
  HwPortStats curPortStats{prevPortStats};
  // All stats start with a unitialized (-1) value. If there are no in
  // discards (first collection) we will just report that -1 as the monotonic
  // counter. Instead set it to 0 if uninintialized
  *curPortStats.inDiscards__ref() = *curPortStats.inDiscards__ref() ==
          hardware_stats_constants::STAT_UNINITIALIZED()
      ? 0
      : *curPortStats.inDiscards__ref();
  curPortStats.timestamp__ref() = now.count();
  handle->port->updateStats(supportedStats(), SAI_STATS_MODE_READ);
  const auto& counters = handle->port->getStats();
  fillHwPortStats(counters, managerTable_->debugCounterManager(), curPortStats);
  std::vector<utility::CounterPrevAndCur> toSubtractFromInDiscardsRaw = {
      {*prevPortStats.inDstNullDiscards__ref(),
       *curPortStats.inDstNullDiscards__ref()},
      {*prevPortStats.inPause__ref(), *curPortStats.inPause__ref()}};
  *curPortStats.inDiscards__ref() += utility::subtractIncrements(
      {*prevPortStats.inDiscardsRaw__ref(), *curPortStats.inDiscardsRaw__ref()},
      toSubtractFromInDiscardsRaw);
  managerTable_->queueManager().updateStats(
      handle->configuredQueues, curPortStats);
  portStats_[portId]->updateStats(curPortStats, now);
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
    fillHwPortStats(
        counters, managerTable_->debugCounterManager(), hwPortStats);
    managerTable_->queueManager().getStats(handle->queues, hwPortStats);
    portStats.emplace(portId, hwPortStats);
  }
  return portStats;
}

void SaiPortManager::clearStats(PortID port) {
  auto portHandle = getPortHandle(PortID(port));
  if (!portHandle) {
    return;
  }
  auto statsToClear = supportedStats();
  if (platform_->getAsic()->isSupported(HwAsic::Feature::DEBUG_COUNTER)) {
    // Debug counters are implemented differently than regular port counters
    // and not all implementations support clearing them. For our use case
    // it doesn't particularly matter if we can't clear them. So prune the
    // debug counter clear for now.
    auto debugCounterId =
        managerTable_->debugCounterManager().getPortL3BlackHoleCounterStatId();
    statsToClear.erase(
        std::remove_if(
            statsToClear.begin(),
            statsToClear.end(),
            [debugCounterId](auto counterId) {
              return counterId == debugCounterId;
            }),
        statsToClear.end());
  }
  portHandle->port->clearStats(statsToClear);
  for (auto& queueAndHandle : portHandle->queues) {
    queueAndHandle.second->queue->clearStats();
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
  for (auto& iter : portStore.objects()) {
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

void SaiPortManager::setQosPolicy() {
  auto& qosMapManager = managerTable_->qosMapManager();
  XLOG(INFO) << "Set default qos map";
  auto qosMapHandle = qosMapManager.getQosMap();
  globalDscpToTcQosMap_ = qosMapHandle->dscpQosMap;
  globalTcToQueueQosMap_ = qosMapHandle->tcQosMap;
  setQosMapsOnAllPorts(
      globalDscpToTcQosMap_->adapterKey(),
      globalTcToQueueQosMap_->adapterKey());
}

void SaiPortManager::clearQosPolicy() {
  setQosMapsOnAllPorts(
      QosMapSaiId(SAI_NULL_OBJECT_ID), QosMapSaiId(SAI_NULL_OBJECT_ID));
  globalDscpToTcQosMap_.reset();
  globalTcToQueueQosMap_.reset();
}

void SaiPortManager::setL2LearningMode(cfg::L2LearningMode l2LearningMode) {
  auto fdbLearningMode = getFdbLearningMode(l2LearningMode);
  for (auto& portIdAndHandle : managerTable_->portManager()) {
    auto& portHandle = portIdAndHandle.second;
    portHandle->bridgePort->setOptionalAttribute(
        SaiBridgePortTraits::Attributes::FdbLearningMode{fdbLearningMode});
  }
  l2LearningMode_ = l2LearningMode;
}

std::shared_ptr<SaiPortSerdes> SaiPortManager::programSerdes(
    std::shared_ptr<SaiPort> saiPort,
    std::shared_ptr<Port> swPort) {
  if (!platform_->isSerdesApiSupported()) {
    return nullptr;
  }
  auto txSettings = platform_->getPlatformPortTxSettings(
      swPort->getID(), swPort->getProfileID());
  auto rxSettings = platform_->getPlatformPortRxSettings(
      swPort->getID(), swPort->getProfileID());
  if (txSettings.empty() && rxSettings.empty()) {
    return nullptr;
  }
  auto& portKey = saiPort->adapterHostKey();
  if (!txSettings.empty()) {
    CHECK_EQ(txSettings.size(), portKey.value().size())
        << "some lanes are missing for tx-settings";
  }
  if (!rxSettings.empty()) {
    CHECK_EQ(rxSettings.size(), portKey.value().size())
        << "some lanes are missing for rx-settings";
  }
  auto& store = SaiStore::getInstance()->get<SaiPortSerdesTraits>();
  SaiPortSerdesTraits::AdapterHostKey serdesKey{saiPort->adapterKey()};
  SaiPortSerdesTraits::CreateAttributes serdesAttributes =
      serdesAttributesFromSwPort(saiPort->adapterKey(), swPort);
  return store.setObject(serdesKey, serdesAttributes);
}

SaiPortSerdesTraits::CreateAttributes
SaiPortManager::serdesAttributesFromSwPort(
    PortSaiId portSaiId,
    const std::shared_ptr<Port>& swPort) {
  SaiPortSerdesTraits::CreateAttributes attrs;

  auto txSettings = platform_->getPlatformPortTxSettings(
      swPort->getID(), swPort->getProfileID());
  auto rxSettings = platform_->getPlatformPortRxSettings(
      swPort->getID(), swPort->getProfileID());

  SaiPortSerdesTraits::Attributes::TxFirPre1::ValueType txPre1;
  SaiPortSerdesTraits::Attributes::TxFirMain::ValueType txMain;
  SaiPortSerdesTraits::Attributes::TxFirPost1::ValueType txPost1;
  SaiPortSerdesTraits::Attributes::IDriver::ValueType txIDriver;

  SaiPortSerdesTraits::Attributes::RxCtleCode::ValueType rxCtleCode;
  SaiPortSerdesTraits::Attributes::RxDspMode::ValueType rxDspMode;
  SaiPortSerdesTraits::Attributes::RxAfeTrim::ValueType rxAfeTrim;
  SaiPortSerdesTraits::Attributes::RxAcCouplingByPass::ValueType
      rxAcCouplingByPass;

  if (!txSettings.empty()) {
    for (auto& tx : txSettings) {
      txPre1.push_back(*tx.pre_ref());
      txMain.push_back(*tx.main_ref());
      txPost1.push_back(*tx.post_ref());
      if (auto driveCurrent = tx.driveCurrent_ref()) {
        txIDriver.push_back(driveCurrent.value());
      }
    }
  }

  if (!rxSettings.empty()) {
    for (auto& rx : rxSettings) {
      if (auto ctlCode = rx.ctlCode_ref()) {
        rxCtleCode.push_back(*ctlCode);
      }
      if (auto dscpMode = rx.dspMode_ref()) {
        rxDspMode.push_back(*dscpMode);
      }
      if (auto afeTrim = rx.afeTrim_ref()) {
        rxAfeTrim.push_back(*afeTrim);
      }
      if (auto acCouplingByPass = rx.acCouplingBypass_ref()) {
        rxAcCouplingByPass.push_back(*acCouplingByPass);
      }
    }
  }

  auto setTxRxAttr = [](auto& attrs, auto type, const auto& val) {
    auto& attr = std::get<std::optional<std::decay_t<decltype(type)>>>(attrs);
    if (!val.empty()) {
      attr = val;
    }
  };

  std::get<SaiPortSerdesTraits::Attributes::PortId>(attrs) =
      static_cast<sai_object_id_t>(portSaiId);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPre1{}, txPre1);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPost1{}, txPost1);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirMain{}, txMain);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::IDriver{}, txIDriver);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxCtleCode{}, rxCtleCode);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxDspMode{}, rxDspMode);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxAfeTrim{}, rxAfeTrim);
  setTxRxAttr(
      attrs,
      SaiPortSerdesTraits::Attributes::RxAcCouplingByPass{},
      rxAcCouplingByPass);
  return attrs;
}
} // namespace facebook::fboss
