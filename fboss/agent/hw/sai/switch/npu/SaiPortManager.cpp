// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

namespace {
std::optional<SaiPortTraits::Attributes::SystemPortId> getSystemPortId(
    const SaiPlatform* platform,
    PortID portId) {
  if (platform->getAsic()->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TAJO) {
    return std::optional<SaiPortTraits::Attributes::SystemPortId>{
        portId + platform->getAsic()->getSystemPortIDOffset()};
  }
  return std::nullopt;
}
} // namespace

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
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER)) {
    counterIds.emplace_back(managerTable_->debugCounterManager()
                                .getMPLSLookupFailedCounterStatId());
  }
  return counterIds;
}

PortSaiId SaiPortManager::addPortImpl(const std::shared_ptr<Port>& swPort) {
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

  auto& portStore = saiStore_->get<SaiPortTraits>();
  auto saiPort = portStore.setObject(portKey, attributes, swPort->getID());
  handle->port = saiPort;
  programSerdes(saiPort, swPort, handle.get());

  loadPortQueues(handle.get());
  const auto asic = platform_->getAsic();
  if (swPort->isEnabled()) {
    portStats_.emplace(
        swPort->getID(), std::make_unique<HwPortFb303Stats>(swPort->getName()));
  }
  for (auto portQueue : swPort->getPortQueues()) {
    auto queueKey =
        std::make_pair(portQueue->getID(), portQueue->getStreamType());
    const auto& configuredQueue = handle->queues[queueKey];
    handle->configuredQueues.push_back(configuredQueue.get());
    portQueue->setReservedBytes(
        portQueue->getReservedBytes()
            ? *portQueue->getReservedBytes()
            : asic->getDefaultReservedBytes(
                  portQueue->getStreamType(), false /* not cpu port*/));
    portQueue->setScalingFactor(
        portQueue->getScalingFactor()
            ? *portQueue->getScalingFactor()
            : asic->getDefaultScalingFactor(
                  portQueue->getStreamType(), false /* not cpu port*/));
    auto pitr = portStats_.find(swPort->getID());

    if (pitr != portStats_.end()) {
      auto queueName = portQueue->getName()
          ? *portQueue->getName()
          : folly::to<std::string>("queue", portQueue->getID());
      // Port stats map is sparse, since we don't maintain/publish stats
      // for disabled ports
      pitr->second->queueChanged(portQueue->getID(), queueName);
    }
  }
  managerTable_->queueManager().ensurePortQueueConfig(
      saiPort->adapterKey(), handle->queues, swPort->getPortQueues());

  bool samplingMirror = swPort->getSampleDestination().has_value() &&
      swPort->getSampleDestination() == cfg::SampleDestination::MIRROR;
  SaiPortMirrorInfo mirrorInfo{
      swPort->getIngressMirror(), swPort->getEgressMirror(), samplingMirror};
  handle->mirrorInfo = mirrorInfo;
  handles_.emplace(swPort->getID(), std::move(handle));
  if (globalDscpToTcQosMap_) {
    // Both global maps must exist in one of them exists
    CHECK(globalTcToQueueQosMap_);
    setQosMaps(
        globalDscpToTcQosMap_->adapterKey(),
        globalTcToQueueQosMap_->adapterKey(),
        {swPort->getID()});
  }

  addSamplePacket(swPort);
  addMirror(swPort);

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());
  return saiPort->adapterKey();
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

  if (createOnlyAttributeChanged(oldAttributes, newAttributes)) {
    XLOG(INFO) << "Create only attribute (e.g. lane, speed etc.) changed for "
               << oldPort->getID();
    removePort(oldPort);
    addPort(newPort);
    return;
  }

  SaiPortTraits::AdapterHostKey portKey{
      GET_ATTR(Port, HwLaneList, newAttributes)};
  auto& portStore = saiStore_->get<SaiPortTraits>();
  auto saiPort = portStore.setObject(portKey, newAttributes, newPort->getID());
  programSerdes(saiPort, newPort, existingPort);
  // if vlan changed update it, this is important for rx processing
  if (newPort->getIngressVlan() != oldPort->getIngressVlan()) {
    concurrentIndices_->vlanIds.insert_or_assign(
        PortDescriptorSaiId(saiPort->adapterKey()), newPort->getIngressVlan());
    XLOG(INFO) << "changed vlan on port " << newPort->getID()
               << ": old vlan: " << oldPort->getIngressVlan()
               << ", new vlan: " << newPort->getIngressVlan();
  }
  if (newPort->getProfileID() != oldPort->getProfileID()) {
    auto platformPort = platform_->getPort(newPort->getID());
    platformPort->setCurrentProfile(newPort->getProfileID());
  }

  changeSamplePacket(oldPort, newPort);
  changeMirror(oldPort, newPort);

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
    const std::shared_ptr<Port>& swPort,
    bool /* lineSide */) const {
  bool adminState =
      swPort->getAdminState() == cfg::PortState::ENABLED ? true : false;
  auto portID = swPort->getID();
  auto platformPort = platform_->getPort(portID);
  auto speed = swPort->getSpeed();
  auto hwLaneList = platformPort->getHwPortLanes(speed);
  auto globalFlowControlMode = utility::getSaiPortPauseMode(swPort->getPause());
  auto internalLoopbackMode =
      utility::getSaiPortInternalLoopbackMode(swPort->getLoopbackMode());

  // Now use profileConfig from SW port as the source of truth
  auto portProfileConfig = swPort->getProfileConfig();
  if (!portProfileConfig.medium_ref()) {
    throw FbossError(
        "Missing medium info in profile ",
        apache::thrift::util::enumNameSafe(swPort->getProfileID()));
  }
  auto transmitterTech = *portProfileConfig.medium_ref();
  auto mediaType = utility::getSaiPortMediaType(transmitterTech, speed);
  auto enableFec =
      (speed >= cfg::PortSpeed::HUNDREDG) || !platformPort->shouldDisableFEC();
  SaiPortTraits::Attributes::FecMode fecMode;
  if (!enableFec) {
    fecMode = SAI_PORT_FEC_MODE_NONE;
  } else {
    fecMode = utility::getSaiPortFecMode(*portProfileConfig.fec_ref());
  }
  std::optional<SaiPortTraits::Attributes::InterfaceType> interfaceType{};
  // TODO(joseph5wu) Maybe provide a new function to convert interfaceType from
  // profile config to sai interface type
  if (auto saiInterfaceType =
          platform_->getInterfaceType(transmitterTech, speed)) {
    interfaceType = saiInterfaceType.value();
  }

  uint16_t vlanId = swPort->getIngressVlan();
  auto systemPortId = getSystemPortId(platform_, swPort->getID());
  return SaiPortTraits::CreateAttributes {
    hwLaneList, static_cast<uint32_t>(speed), adminState, fecMode,
        internalLoopbackMode, mediaType, globalFlowControlMode, vlanId,
        swPort->getMaxFrameSize(), std::nullopt, std::nullopt, std::nullopt,
        interfaceType, std::nullopt,
        std::nullopt, // Ingress Mirror Session
        std::nullopt, // Egress Mirror Session
        std::nullopt, // Ingress Sample Packet
        std::nullopt, // Egress Sample Packet
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
        std::nullopt, // Ingress mirror sample session
        std::nullopt, // Egress mirror sample session
#endif
        std::nullopt, // Ingress MacSec ACL
        std::nullopt, // Egress MacSec ACL
        systemPortId, // System Port Id
        std::nullopt // PTP Mode
  };
}

/*
 * This is a temporary routine to fix AFE trim issue in TAJO.
 * TAJO provided a new extension attribute to set AFE mode to adaptive
 * per port.
 * 1) Apply only on 25G copper port
 * 2) Attribute to use: SAI_PORT_SERDES_ATTR_EXT_RX_AFE_ADAPTIVE_ENABLE
 * 3) Enable adaptive mode only if its not enabled in the SDK.
 */
void SaiPortManager::enableAfeAdaptiveMode(PortID portId) {
  SaiPortHandle* portHandle = getPortHandle(portId);
  if (!portHandle) {
    XLOG(INFO) << "afe adaptive mode not enabled: failed to find port"
               << portId;
    return;
  }
  std::optional<SaiPortTraits::Attributes::MediaType> mediaTypeAttr{};
  auto mediaType = SaiApiTable::getInstance()->portApi().getAttribute(
      portHandle->port->adapterKey(), mediaTypeAttr);
  // Return if media type is not copper
  if (!(mediaType == SAI_PORT_MEDIA_TYPE_COPPER ||
        mediaType == SAI_PORT_MEDIA_TYPE_UNKNOWN)) {
    XLOG(INFO)
        << "afe adaptive mode not enabled: media type do not match for port: "
        << portId;
    return;
  }

  if (!portHandle->serdes) {
    XLOG(INFO) << "afe adaptive mode not enabled: failed to find serdes on port"
               << portId;
    return;
  }

  auto serdesId = portHandle->serdes->adapterKey();
  std::optional<SaiPortSerdesTraits::Attributes::RxAfeAdaptiveEnable>
      rxAfeAdaptiveEnableAttr{};
  auto rxAfeAdaptiveEnabledList =
      SaiApiTable::getInstance()->portApi().getAttribute(
          PortSerdesSaiId(serdesId), rxAfeAdaptiveEnableAttr);
  bool afeReset = false;
  if (rxAfeAdaptiveEnabledList.has_value()) {
    for (auto afeEnabledPerLane : rxAfeAdaptiveEnabledList.value()) {
      if (afeEnabledPerLane == 0) {
        afeReset = true;
        break;
      }
    }
  }
  if (!afeReset) {
    XLOG(INFO) << "afe adaptive mode is already enabled on port: " << portId;
    return;
  }

  SaiPortSerdesTraits::Attributes::RxAfeAdaptiveEnable::ValueType
      rxAfeAdaptiveEnable;
  auto& portKey = portHandle->port->adapterHostKey();
  for (auto i = 0; i < portKey.value().size(); i++) {
    rxAfeAdaptiveEnable.push_back(1);
  }
  auto& store = saiStore_->get<SaiPortSerdesTraits>();
  SaiPortSerdesTraits::AdapterHostKey serdesKey{portHandle->port->adapterKey()};
  auto serdesAttributes = portHandle->serdes->attributes();
  std::get<std::optional<std::decay_t<decltype(
      SaiPortSerdesTraits::Attributes::RxAfeAdaptiveEnable{})>>>(
      serdesAttributes) = rxAfeAdaptiveEnable;
  portHandle->serdes.reset();
  portHandle->serdes = store.setObject(serdesKey, serdesAttributes);
  XLOG(INFO) << "Configuring afe mode to adaptive on port: " << portId;
}

} // namespace facebook::fboss
