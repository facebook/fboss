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

DEFINE_bool(
    sai_use_interface_type_for_medium,
    false,
    "Use interface type in platform mapping to derive the medium "
    "instead of deriving it from the medium field");

namespace facebook::fboss {

namespace {
std::optional<SaiPortTraits::Attributes::SystemPortId> getSystemPortId(
    const SaiPlatform* platform,
    PortID portId) {
  if (platform->getAsic()->getAsicVendor() ==
      HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
    return std::optional<SaiPortTraits::Attributes::SystemPortId>{
        portId + platform->getAsic()->getSystemPortIDOffset()};
  }
  return std::nullopt;
}
} // namespace

void SaiPortManager::fillInSupportedStats(PortID port) {
  auto getSupportedStats = [this, port]() {
    std::vector<sai_stat_id_t> counterIds;
    auto portType = port2PortType_.find(port)->second;
    if (portType == cfg::PortType::FABRIC_PORT) {
      counterIds = std::vector<sai_stat_id_t>{
          SAI_PORT_STAT_IF_IN_OCTETS,
          SAI_PORT_STAT_IF_IN_ERRORS,
          SAI_PORT_STAT_IF_OUT_OCTETS,
          SAI_PORT_STAT_IF_IN_FEC_CORRECTABLE_FRAMES,
          SAI_PORT_STAT_IF_IN_FEC_NOT_CORRECTABLE_FRAMES,
      };
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
      counterIds.emplace_back(managerTable_->debugCounterManager()
                                  .getPortL3BlackHoleCounterStatId());
    }
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER)) {
      counterIds.emplace_back(managerTable_->debugCounterManager()
                                  .getMPLSLookupFailedCounterStatId());
    }
    return counterIds;
  };
  port2SupportedStats_.emplace(port, getSupportedStats());
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
  attributesFromSaiStore(attributes);

  /**
   * Remove port handles if the new HwLaneList contains hw lanes that are used
   * by the removed handles. E.g. Port1 (HwLaneList[1,2]), Port2
   * (HwLaneList[3,4]) are removed to create new Port2 (HwLaneList[1,2,3,4]).
   * Port1 would be added to removedHandles_, but not removed by the SDK yet.
   * Therefore, check new hw lanes and remove ports in SDK accordingly.
   */
  auto newHwLanes = GET_ATTR(Port, HwLaneList, attributes);
  for (auto& [removedPortId, removedHandle] : removedHandles_) {
    auto removedHwLanes =
        GET_ATTR(Port, HwLaneList, removedHandle->port->attributes());
    for (auto removedHwLane : removedHwLanes) {
      if (std::find(newHwLanes.begin(), newHwLanes.end(), removedHwLane) !=
          newHwLanes.end()) {
        removeRemovedHandleIf(removedPortId);
        break;
      }
    }
  }

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
  addPfc(swPort);

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());

  // set the lower 32-bit of SaiId as Hardware logical port ID
  auto portSaiId = saiPort->adapterKey();
  uint32_t hwLogicalPortId = static_cast<uint32_t>(portSaiId);
  platformPort->setHwLogicalPortId(hwLogicalPortId);
  return portSaiId;
}

void SaiPortManager::changePortImpl(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  SaiPortHandle* existingPort = getPortHandle(newPort->getID());
  if (!existingPort) {
    throw FbossError("Attempted to change non-existent port ");
  }
  SaiPortTraits::CreateAttributes oldAttributes = attributesFromSwPort(oldPort);
  SaiPortTraits::CreateAttributes newAttributes = attributesFromSwPort(newPort);

  if (createOnlyAttributeChanged(oldAttributes, newAttributes)) {
    XLOG(DBG2) << "Create only attribute (e.g. lane, speed etc.) changed for "
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
    XLOG(DBG2) << "changed vlan on port " << newPort->getID()
               << ": old vlan: " << oldPort->getIngressVlan()
               << ", new vlan: " << newPort->getIngressVlan();
  }
  if (newPort->getProfileID() != oldPort->getProfileID()) {
    auto platformPort = platform_->getPort(newPort->getID());
    platformPort->setCurrentProfile(newPort->getProfileID());
  }

  changeSamplePacket(oldPort, newPort);
  changeMirror(oldPort, newPort);
  changePfc(oldPort, newPort);

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

void SaiPortManager::attributesFromSaiStore(
    SaiPortTraits::CreateAttributes& attributes) {
  SaiPortTraits::AdapterHostKey portKey{GET_ATTR(Port, HwLaneList, attributes)};
  auto& store = saiStore_->get<SaiPortTraits>();
  std::shared_ptr<SaiPort> port = store.get(portKey);
  if (!port) {
    return;
  }
  auto getAndSetAttribute = [](auto& storeAttrs, auto& swAttrs, auto type) {
    std::get<std::optional<std::decay_t<decltype(type)>>>(swAttrs) =
        std::get<std::optional<std::decay_t<decltype(type)>>>(storeAttrs);
  };
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::IngressMirrorSession{});
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::EgressMirrorSession{});
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::IngressSampleMirrorSession{});
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::EgressSampleMirrorSession{});
#endif
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::IngressSamplePacketEnable{});
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::EgressSamplePacketEnable{});
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::QosDscpToTcMap{});
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::QosTcToQueueMap{});
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
  if (!portProfileConfig.medium()) {
    throw FbossError(
        "Missing medium info in profile ",
        apache::thrift::util::enumNameSafe(swPort->getProfileID()));
  }
  auto transmitterTech = *portProfileConfig.medium();
  sai_port_media_type_t mediaType;
  if (FLAGS_sai_use_interface_type_for_medium) {
    // We are currently programming different media type for BCM-SAI
    // implementation vs Native SDK implementation. That's because for
    // native-bcm implementation, we derive the media type from interfaceType in
    // platform mapping. However, we use the 'medium' field from platform
    // mapping for the SAI implementation. This causes us to program a
    // sub-optimal media type with SAI (and cause issues like S280146) as for
    // certain profiles, the medium type is defined as TransmitterTech.OPTICAL
    // whereas the interface type is InterfaceType.KR4 and they both currently
    // lead to different SAI_PORT_MEDIA_TYPE. Since changing the media type is a
    // disruptive change, we are guarding this with a flag for now as that will
    // help us stage the roll out of this change. Eventually, we'll get rid of
    // this flag and make deriving the media type from interfaceType as default
    if (!portProfileConfig.interfaceType()) {
      throw FbossError(
          "Missing interfaceType in profile ",
          apache::thrift::util::enumNameSafe(swPort->getProfileID()));
    }
    mediaType = utility::getSaiPortMediaFromInterfaceType(
        *portProfileConfig.interfaceType());
  } else {
    mediaType = utility::getSaiPortMediaType(transmitterTech, speed);
  }
  auto enableFec =
      (speed >= cfg::PortSpeed::HUNDREDG) || !platformPort->shouldDisableFEC();
  SaiPortTraits::Attributes::FecMode fecMode;
  if (!enableFec) {
    fecMode = SAI_PORT_FEC_MODE_NONE;
  } else {
    fecMode = utility::getSaiPortFecMode(*portProfileConfig.fec());
  }
  std::optional<SaiPortTraits::Attributes::InterfaceType> interfaceType{};
  // TODO(joseph5wu) Maybe provide a new function to convert interfaceType from
  // profile config to sai interface type
  if (auto saiInterfaceType =
          platform_->getInterfaceType(transmitterTech, speed)) {
    interfaceType = saiInterfaceType.value();
  }

  auto ptpStatusOpt = managerTable_->switchManager().getPtpTcEnabled();
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
        ptpStatusOpt, // PTP Mode, can be std::nullopt
        std::nullopt, // PFC Mode
        std::nullopt, // PFC Priorities
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
    XLOG(DBG2) << "afe adaptive mode not enabled: failed to find port"
               << portId;
    return;
  }
  std::optional<SaiPortTraits::Attributes::MediaType> mediaTypeAttr{};
  auto mediaType = SaiApiTable::getInstance()->portApi().getAttribute(
      portHandle->port->adapterKey(), mediaTypeAttr);
  // Return if media type is not copper
  if (!(mediaType == SAI_PORT_MEDIA_TYPE_COPPER ||
        mediaType == SAI_PORT_MEDIA_TYPE_UNKNOWN)) {
    XLOG(DBG2)
        << "afe adaptive mode not enabled: media type do not match for port: "
        << portId;
    return;
  }

  if (!portHandle->serdes) {
    XLOG(DBG2) << "afe adaptive mode not enabled: failed to find serdes on port"
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
    XLOG(DBG2) << "afe adaptive mode is already enabled on port: " << portId;
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
  XLOG(DBG2) << "Configuring afe mode to adaptive on port: " << portId;
}

} // namespace facebook::fboss
