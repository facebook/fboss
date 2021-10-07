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
#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
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

#include <fmt/ranges.h>

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
} // namespace

SaiPortManager::SaiPortManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform,
    ConcurrentIndices* concurrentIndices)
    : saiStore_(saiStore),
      managerTable_(managerTable),
      platform_(platform),
      removePortsAtExit_(platform_->getAsic()->isSupported(
          HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT)),
      concurrentIndices_(concurrentIndices) {}

SaiPortHandle::~SaiPortHandle() {
  if (ingressSamplePacket) {
    port->setOptionalAttribute(
        SaiPortTraits::Attributes::IngressSamplePacketEnable{
            SAI_NULL_OBJECT_ID});
  }
  if (egressSamplePacket) {
    port->setOptionalAttribute(
        SaiPortTraits::Attributes::EgressSamplePacketEnable{
            SAI_NULL_OBJECT_ID});
  }
}

SaiPortManager::~SaiPortManager() {
  releasePorts();
}

void SaiPortManager::releasePorts() {
  if (!removePortsAtExit_) {
    for (const auto& handle : handles_) {
      const auto& saiPortHandle = handle.second;
      saiPortHandle->port->release();
    }
    for (const auto& handle : removedHandles_) {
      const auto& saiPortHandle = handle.second;
      saiPortHandle->port->release();
    }
  }
}

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

void SaiPortManager::addMirror(const std::shared_ptr<Port>& swPort) {
  bool samplingMirror = swPort->getSampleDestination().has_value() &&
      swPort->getSampleDestination() == cfg::SampleDestination::MIRROR;
  /*
   * Based on sample destination, configure sflow based mirroring
   * or regular mirror on port attribute
   */
  if (samplingMirror) {
    if (swPort->getIngressMirror().has_value()) {
      programSamplingMirror(
          swPort->getID(),
          MirrorDirection::INGRESS,
          MirrorAction::START,
          swPort->getIngressMirror());
    }
    if (swPort->getEgressMirror().has_value()) {
      programSamplingMirror(
          swPort->getID(),
          MirrorDirection::EGRESS,
          MirrorAction::START,
          swPort->getEgressMirror());
    }
  } else {
    if (swPort->getIngressMirror().has_value()) {
      programMirror(
          swPort->getID(),
          MirrorDirection::INGRESS,
          MirrorAction::START,
          swPort->getIngressMirror());
    }
    if (swPort->getEgressMirror().has_value()) {
      programMirror(
          swPort->getID(),
          MirrorDirection::EGRESS,
          MirrorAction::START,
          swPort->getEgressMirror());
    }
  }
}

PortSaiId SaiPortManager::addPort(const std::shared_ptr<Port>& swPort) {
  auto portSaiId = addPortImpl(swPort);
  concurrentIndices_->portIds.emplace(portSaiId, swPort->getID());
  concurrentIndices_->portSaiIds.emplace(swPort->getID(), portSaiId);
  concurrentIndices_->vlanIds.emplace(
      PortDescriptorSaiId(portSaiId), swPort->getIngressVlan());
  XLOG(INFO) << "added port " << swPort->getID() << " with vlan "
             << swPort->getIngressVlan();
  return portSaiId;
}

void SaiPortManager::addSamplePacket(const std::shared_ptr<Port>& swPort) {
  if (swPort->getSflowIngressRate()) {
    programSampling(
        swPort->getID(),
        SamplePacketDirection::INGRESS,
        SamplePacketAction::START,
        swPort->getSflowIngressRate(),
        swPort->getSampleDestination());
  }
  if (swPort->getSflowEgressRate()) {
    programSampling(
        swPort->getID(),
        SamplePacketDirection::EGRESS,
        SamplePacketAction::START,
        swPort->getSflowEgressRate(),
        swPort->getSampleDestination());
  }
}

void SaiPortManager::removeMirror(const std::shared_ptr<Port>& swPort) {
  SaiPortHandle* portHandle = getPortHandle(swPort->getID());
  if (swPort->getIngressMirror().has_value()) {
    if (portHandle->mirrorInfo.isMirrorSampled()) {
      programSamplingMirror(
          swPort->getID(),
          MirrorDirection::INGRESS,
          MirrorAction::STOP,
          swPort->getIngressMirror());
    } else {
      programMirror(
          swPort->getID(),
          MirrorDirection::INGRESS,
          MirrorAction::STOP,
          swPort->getIngressMirror());
    }
  }

  if (swPort->getEgressMirror().has_value()) {
    if (portHandle->mirrorInfo.isMirrorSampled()) {
      programSamplingMirror(
          swPort->getID(),
          MirrorDirection::EGRESS,
          MirrorAction::STOP,
          swPort->getEgressMirror());
    } else {
      programMirror(
          swPort->getID(),
          MirrorDirection::EGRESS,
          MirrorAction::STOP,
          swPort->getEgressMirror());
    }
  }
}

void SaiPortManager::removeSamplePacket(const std::shared_ptr<Port>& swPort) {
  if (swPort->getSflowIngressRate()) {
    programSampling(
        swPort->getID(),
        SamplePacketDirection::INGRESS,
        SamplePacketAction::STOP,
        swPort->getSflowIngressRate(),
        swPort->getSampleDestination());
  }
  if (swPort->getSflowEgressRate()) {
    programSampling(
        swPort->getID(),
        SamplePacketDirection::EGRESS,
        SamplePacketAction::STOP,
        swPort->getSflowEgressRate(),
        swPort->getSampleDestination());
  }
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

  removeMirror(swPort);
  removeSamplePacket(swPort);

  concurrentIndices_->portIds.erase(itr->second->port->adapterKey());
  concurrentIndices_->portSaiIds.erase(swId);
  concurrentIndices_->vlanIds.erase(
      PortDescriptorSaiId(itr->second->port->adapterKey()));
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
  const auto asic = platform_->getAsic();
  for (auto newPortQueue : newQueueConfig) {
    // Queue create or update
    SaiQueueConfig saiQueueConfig =
        std::make_pair(newPortQueue->getID(), newPortQueue->getStreamType());
    auto queueHandle = getQueueHandle(swId, saiQueueConfig);
    newPortQueue->setReservedBytes(
        newPortQueue->getReservedBytes()
            ? *newPortQueue->getReservedBytes()
            : asic->getDefaultReservedBytes(
                  newPortQueue->getStreamType(), false /* not cpu port*/));
    newPortQueue->setScalingFactor(
        newPortQueue->getScalingFactor()
            ? *newPortQueue->getScalingFactor()
            : asic->getDefaultScalingFactor(
                  newPortQueue->getStreamType(), false /* not cpu port*/));
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

void SaiPortManager::changeSamplePacket(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  if (newPort->getSflowIngressRate() != oldPort->getSflowIngressRate()) {
    if (newPort->getSflowIngressRate() != 0) {
      programSampling(
          newPort->getID(),
          SamplePacketDirection::INGRESS,
          SamplePacketAction::START,
          newPort->getSflowIngressRate(),
          newPort->getSampleDestination());
    } else {
      programSampling(
          newPort->getID(),
          SamplePacketDirection::INGRESS,
          SamplePacketAction::STOP,
          newPort->getSflowIngressRate(),
          newPort->getSampleDestination());
    }
  }

  if (newPort->getSflowEgressRate() != oldPort->getSflowEgressRate()) {
    if (newPort->getSflowEgressRate() != 0) {
      programSampling(
          newPort->getID(),
          SamplePacketDirection::EGRESS,
          SamplePacketAction::START,
          newPort->getSflowEgressRate(),
          newPort->getSampleDestination());
    } else {
      programSampling(
          newPort->getID(),
          SamplePacketDirection::EGRESS,
          SamplePacketAction::STOP,
          newPort->getSflowEgressRate(),
          newPort->getSampleDestination());
    }
  }
}

void SaiPortManager::changeMirror(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  bool oldSamplingMirror = oldPort->getSampleDestination().has_value() &&
      oldPort->getSampleDestination().value() == cfg::SampleDestination::MIRROR;
  bool newSamplingMirror = newPort->getSampleDestination().has_value() &&
      newPort->getSampleDestination().value() == cfg::SampleDestination::MIRROR;

  /*
   * If there is any update to the mirror session on a port, detach the
   * current mirror session from the port and re-attach the new mirror session
   * if valid
   */
  auto programMirroring =
      [this, oldPort, newPort, oldSamplingMirror, newSamplingMirror](
          MirrorDirection direction) {
        auto oldMirrorId = direction == MirrorDirection::INGRESS
            ? oldPort->getIngressMirror()
            : oldPort->getEgressMirror();
        auto newMirrorId = direction == MirrorDirection::INGRESS
            ? newPort->getIngressMirror()
            : newPort->getEgressMirror();
        if (oldSamplingMirror) {
          programSamplingMirror(
              newPort->getID(), direction, MirrorAction::STOP, oldMirrorId);
        } else {
          programMirror(
              newPort->getID(), direction, MirrorAction::STOP, oldMirrorId);
        }
        if (newMirrorId.has_value()) {
          if (newSamplingMirror) {
            programSamplingMirror(
                newPort->getID(), direction, MirrorAction::START, newMirrorId);
          } else {
            programMirror(
                newPort->getID(), direction, MirrorAction::START, newMirrorId);
          }
        }
      };

  if (newPort->getIngressMirror() != oldPort->getIngressMirror()) {
    programMirroring(MirrorDirection::INGRESS);
  }

  if (newPort->getEgressMirror() != oldPort->getEgressMirror()) {
    programMirroring(MirrorDirection::EGRESS);
  }

  SaiPortHandle* portHandle = getPortHandle(newPort->getID());
  SaiPortMirrorInfo mirrorInfo{
      newPort->getIngressMirror(),
      newPort->getEgressMirror(),
      newSamplingMirror};
  portHandle->mirrorInfo = mirrorInfo;
}

bool SaiPortManager::createOnlyAttributeChanged(
    const SaiPortTraits::CreateAttributes& oldAttributes,
    const SaiPortTraits::CreateAttributes& newAttributes) {
  return (std::get<SaiPortTraits::Attributes::HwLaneList>(oldAttributes) !=
          std::get<SaiPortTraits::Attributes::HwLaneList>(newAttributes)) ||
      (platform_->getAsic()->isSupported(
           HwAsic::Feature::SAI_PORT_SPEED_CHANGE) &&
       (std::get<SaiPortTraits::Attributes::Speed>(oldAttributes) !=
        std::get<SaiPortTraits::Attributes::Speed>(newAttributes)));
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
  PlatformPortProfileConfigMatcher matcher{port->getProfileID(), portID};
  if (auto profileConfig = platform_->getPortProfileConfig(matcher)) {
    port->setProfileConfig(*profileConfig->iphy_ref());
  } else {
    throw FbossError(
        "No port profile config found with matcher:", matcher.toString());
  }
  port->resetPinConfigs(platformPort->getIphyPinConfigs(port->getProfileID()));
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

void SaiPortManager::updateStats(PortID portId, bool updateWatermarks) {
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
      handle->configuredQueues, curPortStats, updateWatermarks);
  managerTable_->macsecManager().updateStats(portId, curPortStats);
  portStats_[portId]->updateStats(curPortStats, now);
}

std::map<PortID, HwPortStats> SaiPortManager::getPortStats() const {
  std::map<PortID, HwPortStats> portStats;
  for (const auto& handle : handles_) {
    auto portStatItr = portStats_.find(handle.first);
    if (portStatItr == portStats_.end()) {
      // We don't maintain port stats for disabled ports.
      continue;
    }
    HwPortStats hwPortStats{portStatItr->second->portStats()};
    portStats.emplace(handle.first, hwPortStats);
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
  auto& portStore = saiStore_->get<SaiPortTraits>();
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
  globalDscpToTcQosMap_ = qosMapHandle->dscpToTcMap;
  globalTcToQueueQosMap_ = qosMapHandle->tcToQueueMap;
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

void SaiPortManager::programSerdes(
    std::shared_ptr<SaiPort> saiPort,
    std::shared_ptr<Port> swPort,
    SaiPortHandle* portHandle) {
  if (!platform_->isSerdesApiSupported()) {
    return;
  }

  auto& portKey = saiPort->adapterHostKey();
  SaiPortSerdesTraits::AdapterHostKey serdesKey{saiPort->adapterKey()};
  auto& store = saiStore_->get<SaiPortSerdesTraits>();
  // check if serdes object already exists for given port,
  // either already programmed or reloaded from adapter.
  std::shared_ptr<SaiPortSerdes> serdes = store.get(serdesKey);

  if (!serdes) {
    // serdes is not yet programmed or reloaded from adapter
    std::optional<SaiPortTraits::Attributes::SerdesId> serdesAttr{};
    auto serdesId = SaiApiTable::getInstance()->portApi().getAttribute(
        saiPort->adapterKey(), serdesAttr);
    if (serdesId.has_value() && serdesId.value() != SAI_NULL_OBJECT_ID) {
      // but default serdes exists in the adapter.
      serdes =
          store.reloadObject(static_cast<PortSerdesSaiId>(serdesId.value()));
    }
  } else {
    // ensure warm boot handles are reclaimed removed
    // no-op if serdes is already programmed
    // remove warm boot handle if reloaded from adapter but not yet programmed
    serdes = store.setObject(serdesKey, serdes->attributes());
  }

  // Check if there are expected tx/rx settings from SW port, the number of
  // lanes should match to the number of portKey
  auto numExpectedTxLanes = 0;
  auto numExpectedRxLanes = 0;
  for (const auto& pinConfig : swPort->getPinConfigs()) {
    if (auto tx = pinConfig.tx_ref()) {
      ++numExpectedTxLanes;
    }
    if (auto rx = pinConfig.rx_ref()) {
      ++numExpectedRxLanes;
    }
  }
  if (numExpectedTxLanes) {
    CHECK_EQ(numExpectedTxLanes, portKey.value().size())
        << "some lanes are missing for tx-settings";
  }
  if (numExpectedRxLanes) {
    CHECK_EQ(numExpectedRxLanes, portKey.value().size())
        << "some lanes are missing for rx-settings";
  }

  SaiPortSerdesTraits::CreateAttributes serdesAttributes =
      serdesAttributesFromSwPinConfigs(
          saiPort->adapterKey(), swPort->getPinConfigs());
  if (serdes &&
      checkPortSerdesAttributes(serdes->attributes(), serdesAttributes)) {
    portHandle->serdes = serdes;
    return;
  }
  // Currently TH3 requires setting sixtap attributes at once, but sai
  // interface only suppports setAttribute() one at a time. Therefore, we'll
  // need to remove the serdes object and then recreate with the sixtap
  // attributes.
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET) &&
      serdes) {
    // Give up all references to the serdes object to delete the serdes object.
    portHandle->serdes.reset();
    serdes.reset();
  }
  // create if serdes doesn't exist or update existing serdes
  portHandle->serdes = store.setObject(serdesKey, serdesAttributes);
}

SaiPortSerdesTraits::CreateAttributes
SaiPortManager::serdesAttributesFromSwPinConfigs(
    PortSaiId portSaiId,
    const std::vector<phy::PinConfig>& pinConfigs) {
  SaiPortSerdesTraits::CreateAttributes attrs;

  SaiPortSerdesTraits::Attributes::TxFirPre1::ValueType txPre1;
  SaiPortSerdesTraits::Attributes::TxFirMain::ValueType txMain;
  SaiPortSerdesTraits::Attributes::TxFirPost1::ValueType txPost1;
  SaiPortSerdesTraits::Attributes::IDriver::ValueType txIDriver;

  SaiPortSerdesTraits::Attributes::RxCtleCode::ValueType rxCtleCode;
  SaiPortSerdesTraits::Attributes::RxDspMode::ValueType rxDspMode;
  SaiPortSerdesTraits::Attributes::RxAfeTrim::ValueType rxAfeTrim;
  SaiPortSerdesTraits::Attributes::RxAcCouplingByPass::ValueType
      rxAcCouplingByPass;

  // Now use pinConfigs from SW port as the source of truth
  auto numExpectedTxLanes = 0;
  auto numExpectedRxLanes = 0;
  for (const auto& pinConfig : pinConfigs) {
    if (auto tx = pinConfig.tx_ref()) {
      ++numExpectedTxLanes;
      txPre1.push_back(*tx->pre_ref());
      txMain.push_back(*tx->main_ref());
      txPost1.push_back(*tx->post_ref());
      if (auto driveCurrent = tx->driveCurrent_ref()) {
        txIDriver.push_back(driveCurrent.value());
      }
    }
    if (auto rx = pinConfig.rx_ref()) {
      ++numExpectedRxLanes;
      if (auto ctlCode = rx->ctlCode_ref()) {
        rxCtleCode.push_back(*ctlCode);
      }
      if (auto dscpMode = rx->dspMode_ref()) {
        rxDspMode.push_back(*dscpMode);
      }
      if (auto afeTrim = rx->afeTrim_ref()) {
        rxAfeTrim.push_back(*afeTrim);
      }
      if (auto acCouplingByPass = rx->acCouplingBypass_ref()) {
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

  if (platform_->getAsic()->getPortSerdesPreemphasis().has_value()) {
    SaiPortSerdesTraits::Attributes::Preemphasis::ValueType preempahsis(
        numExpectedTxLanes,
        platform_->getAsic()->getPortSerdesPreemphasis().value());
    setTxRxAttr(
        attrs, SaiPortSerdesTraits::Attributes::Preemphasis{}, preempahsis);
  }

  if (numExpectedRxLanes) {
    setTxRxAttr(
        attrs, SaiPortSerdesTraits::Attributes::RxCtleCode{}, rxCtleCode);
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxDspMode{}, rxDspMode);
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxAfeTrim{}, rxAfeTrim);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxAcCouplingByPass{},
        rxAcCouplingByPass);
  }

  return attrs;
}

void SaiPortManager::programSampling(
    PortID portId,
    SamplePacketDirection direction,
    SamplePacketAction action,
    uint64_t sampleRate,
    std::optional<cfg::SampleDestination> sampleDestination) {
  auto destination = sampleDestination.has_value()
      ? sampleDestination.value()
      : cfg::SampleDestination::CPU;
  SaiPortHandle* portHandle = getPortHandle(portId);
  auto samplePacketHandle = action == SamplePacketAction::START
      ? managerTable_->samplePacketManager().getOrCreateSamplePacket(
            sampleRate, destination)
      : nullptr;
  /*
   * Create the sammple packet object, update the port sample packet
   * oid and then overwrite the sample packet object on port.
   */
  auto samplePacketAdapterKey = samplePacketHandle
      ? samplePacketHandle->adapterKey()
      : SAI_NULL_OBJECT_ID;
  if (direction == SamplePacketDirection::INGRESS) {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::IngressSamplePacketEnable{
            SamplePacketSaiId(samplePacketAdapterKey)});
    portHandle->ingressSamplePacket = samplePacketHandle;
  } else {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::EgressSamplePacketEnable{
            SamplePacketSaiId(samplePacketAdapterKey)});
    portHandle->egressSamplePacket = samplePacketHandle;
  }

  XLOG(DBG) << "Programming sample packet on port: " << std::hex
            << portHandle->port->adapterKey() << " action: "
            << (action == SamplePacketAction::START ? "start" : "stop")
            << " direction: "
            << (direction == SamplePacketDirection::INGRESS ? "ingress"
                                                            : "egress")
            << " sample packet oid " << std::hex << samplePacketAdapterKey
            << "destination: "
            << (destination == cfg::SampleDestination::CPU ? "cpu" : "mirror");
}

void SaiPortManager::programMirror(
    PortID portId,
    MirrorDirection direction,
    MirrorAction action,
    std::optional<std::string> mirrorId) {
  auto portHandle = getPortHandle(portId);
  std::vector<sai_object_id_t> mirrorOidList{};
  if (action == MirrorAction::START) {
    auto mirrorHandle =
        managerTable_->mirrorManager().getMirrorHandle(mirrorId.value());
    if (!mirrorHandle) {
      XLOG(DBG) << "Failed to find mirror session: " << mirrorId.value();
      return;
    }
    mirrorOidList.push_back(mirrorHandle->adapterKey());
  }

  if (direction == MirrorDirection::INGRESS) {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::IngressMirrorSession{mirrorOidList});
  } else {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::EgressMirrorSession{mirrorOidList});
  }

  XLOG(DBG) << "Programming mirror on port: " << std::hex
            << portHandle->port->adapterKey()
            << " action: " << (action == MirrorAction::START ? "start" : "stop")
            << " direction: "
            << (direction == MirrorDirection::INGRESS ? "ingress" : "egress");
}

void SaiPortManager::programSamplingMirror(
    PortID portId,
    MirrorDirection direction,
    MirrorAction action,
    std::optional<std::string> mirrorId) {
  auto portHandle = getPortHandle(portId);
  std::vector<sai_object_id_t> mirrorOidList{};
  if (action == MirrorAction::START) {
    auto mirrorHandle =
        managerTable_->mirrorManager().getMirrorHandle(mirrorId.value());
    if (!mirrorHandle) {
      XLOG(DBG) << "Failed to find mirror session: " << mirrorId.value();
      return;
    }
    mirrorOidList.push_back(mirrorHandle->adapterKey());
  }
  /*
   * case 1: Only mirroring:
   * Sample destination will be empty and no sample object will be created
   * INGRESS_MIRROR/EGRESS_MIRROR attribute will be set to the mirror OID
   * and INGRESS_SAMPLE_MIRROR/EGRESS_SAMPLE_MIRROR will be reset to
   * NULL object ID.
   *
   * case 2: Only sampling
   * When sample destination is CPU, then no mirroring object will be created.
   * A sample packet object will be created with destination as CPU and the
   * mirroring attributes on a port will be reset to NULL object ID.
   *
   * case 3: Mirroring and sampling
   * When sample destination is MIRROR, a sample object will be created with
   * destination as MIRROR. INGRESS_SAMPLE_MIRROR/EGRESS_SAMPLE_MIRROR will be
   * set with the right mirror OID and INGRESS_MIRROR/EGRESS_MIRROR attribute
   * will be reset to NULL object ID.
   *
   * NOTE: Some vendors do not free the SAI object unless the attributes
   * are set to NULL OID.
   */

  if (direction == MirrorDirection::INGRESS) {
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::IngressSampleMirrorSession{mirrorOidList});
#endif
  } else {
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::EgressSampleMirrorSession{mirrorOidList});
#endif
  }

  XLOG(DBG) << "Programming sampling mirror on port: " << std::hex
            << portHandle->port->adapterKey()
            << " action: " << (action == MirrorAction::START ? "start" : "stop")
            << " direction: "
            << (direction == MirrorDirection::INGRESS ? "ingress" : "egress");
}

void SaiPortManager::programMirrorOnAllPorts(
    const std::string& mirrorName,
    MirrorAction action) {
  /*
   * This is invoked by the mirror manager when a mirror session is
   * created or deleted. Based on the action and samplingMirror flag,
   * configure the right attribute.
   *
   * TODO: The ingress and egress mirror is cached in port handle
   * for this comparison. Query the mirror session ID from the port,
   * compare it with mirrorName adapterkey and update port if they
   * are the same.
   */
  for (const auto& portIdAndHandle : handles_) {
    auto& mirrorInfo = portIdAndHandle.second->mirrorInfo;
    if (mirrorInfo.getIngressMirror() == mirrorName) {
      if (mirrorInfo.isMirrorSampled()) {
        programSamplingMirror(
            portIdAndHandle.first,
            MirrorDirection::INGRESS,
            action,
            mirrorName);
      } else {
        programMirror(
            portIdAndHandle.first,
            MirrorDirection::INGRESS,
            action,
            mirrorName);
      }
    }
    if (mirrorInfo.getEgressMirror() == mirrorName) {
      if (mirrorInfo.isMirrorSampled()) {
        programSamplingMirror(
            portIdAndHandle.first, MirrorDirection::EGRESS, action, mirrorName);
      } else {
        programMirror(
            portIdAndHandle.first, MirrorDirection::EGRESS, action, mirrorName);
      }
    }
  }
}

void SaiPortManager::addBridgePort(const std::shared_ptr<Port>& port) {
  auto handle = getPortHandle(port->getID());
  CHECK(handle) << "unknown port " << port->getID();

  const auto& saiPort = handle->port;
  handle->bridgePort = managerTable_->bridgeManager().addBridgePort(
      SaiPortDescriptor(port->getID()),
      PortDescriptorSaiId(saiPort->adapterKey()));
}

void SaiPortManager::changeBridgePort(
    const std::shared_ptr<Port>& /*oldPort*/,
    const std::shared_ptr<Port>& newPort) {
  return addBridgePort(newPort);
}

bool SaiPortManager::isUp(PortID portID) const {
  auto handle = getPortHandle(portID);
  auto saiPortId = handle->port->adapterKey();
  return isUp(saiPortId);
}

bool SaiPortManager::isUp(PortSaiId saiPortId) const {
  auto adminState = SaiApiTable::getInstance()->portApi().getAttribute(
      saiPortId, SaiPortTraits::Attributes::AdminState{});
  auto operStatus = SaiApiTable::getInstance()->portApi().getAttribute(
      saiPortId, SaiPortTraits::Attributes::OperStatus{});
  return adminState && (operStatus == SAI_PORT_OPER_STATUS_UP);
}

std::optional<SaiPortTraits::Attributes::PtpMode> SaiPortManager::getPtpMode()
    const {
  std::set<SaiPortTraits::Attributes::PtpMode> ptpModes;
  for (const auto& portIdAndHandle : handles_) {
    const auto ptpMode = SaiApiTable::getInstance()->portApi().getAttribute(
        portIdAndHandle.second->port->adapterKey(),
        SaiPortTraits::Attributes::PtpMode());
    ptpModes.insert(ptpMode);
  }

  if (ptpModes.size() > 1) {
    XLOG(WARN) << fmt::format(
        "all ports do not have same ptpMode: {}", ptpModes);
    return {};
  }

  // if handles_ is empty, treat as ptp disabled
  return ptpModes.empty() ? SAI_PORT_PTP_MODE_NONE : *ptpModes.begin();
}

bool SaiPortManager::isPtpTcEnabled() const {
  auto ptpMode = getPtpMode();
  return ptpMode && *ptpMode == SAI_PORT_PTP_MODE_SINGLE_STEP_TIMESTAMP;
}

void SaiPortManager::setPtpTcEnable(bool enable) {
  for (const auto& portIdAndHandle : handles_) {
    portIdAndHandle.second->port->setOptionalAttribute(
        SaiPortTraits::Attributes::PtpMode{utility::getSaiPortPtpMode(enable)});
  }
}

void SaiPortManager::programMacsec(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  CHECK(newPort);
  auto portId = newPort->getID();
  auto& macsecManager = managerTable_->macsecManager();
  // TX SAKs
  std::optional<mka::MKASak> oldTxSak =
      oldPort ? oldPort->getTxSak() : std::nullopt;
  std::optional<mka::MKASak> newTxSak = newPort->getTxSak();
  if (oldTxSak != newTxSak) {
    if (!newTxSak) {
      auto txSak = *oldTxSak;
      macsecManager.deleteMacsec(
          portId, txSak, *txSak.sci_ref(), SAI_MACSEC_DIRECTION_EGRESS);
    } else {
      // TX SAK mismatch between old and new. Reprogram SAK TX
      auto txSak = *newTxSak;
      macsecManager.setupMacsec(
          portId, txSak, *txSak.sci_ref(), SAI_MACSEC_DIRECTION_EGRESS);
    }
  }
  // RX SAKS
  auto oldRxSaks = oldPort ? oldPort->getRxSaks() : PortFields::RxSaks{};
  auto newRxSaks = newPort->getRxSaks();
  for (const auto& keyAndSak : newRxSaks) {
    const auto& [key, sak] = keyAndSak;
    auto kitr = oldRxSaks.find(key);
    if (kitr == oldRxSaks.end() || sak != kitr->second) {
      // Use the SCI from the key. Since for RX we use SCI of peer, which
      // is stored in MKASakKey
      macsecManager.setupMacsec(
          portId, sak, key.sci, SAI_MACSEC_DIRECTION_INGRESS);
      oldRxSaks.erase(key);
    }
  }
  // Erase whatever could not be found in newRxSaks
  for (const auto& keyAndSak : oldRxSaks) {
    const auto& [key, sak] = keyAndSak;
    // Use the SCI from the key. Since for RX we use SCI of peer, which
    // is stored in MKASakKey
    macsecManager.deleteMacsec(
        portId, sak, key.sci, SAI_MACSEC_DIRECTION_INGRESS);
  }
}
} // namespace facebook::fboss
