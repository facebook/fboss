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
    if (getPortType(port) == cfg::PortType::FABRIC_PORT) {
      counterIds = std::vector<sai_stat_id_t>{
          SAI_PORT_STAT_IF_IN_OCTETS,
          SAI_PORT_STAT_IF_IN_ERRORS,
          SAI_PORT_STAT_IF_OUT_OCTETS,
      };
      return counterIds;
    }
    if (platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_INDUS) {
      /*
       * TODO(skhare) INDUS ASIC supports only a small set of stats today.
       * Remove this check once INDUS ASIC supports querying all (most) of the
       * port stats.
       */
      counterIds = std::vector<sai_stat_id_t>{
          SAI_PORT_STAT_IF_IN_OCTETS,
          SAI_PORT_STAT_IF_IN_UCAST_PKTS,
          SAI_PORT_STAT_IF_IN_MULTICAST_PKTS,
          SAI_PORT_STAT_IF_IN_BROADCAST_PKTS,
          SAI_PORT_STAT_IF_IN_DISCARDS,
          SAI_PORT_STAT_IF_IN_ERRORS,
          SAI_PORT_STAT_PAUSE_RX_PKTS,
          SAI_PORT_STAT_IF_OUT_OCTETS,
          SAI_PORT_STAT_IF_OUT_UCAST_PKTS,
          SAI_PORT_STAT_IF_OUT_MULTICAST_PKTS,
          SAI_PORT_STAT_IF_OUT_BROADCAST_PKTS,
          SAI_PORT_STAT_IF_OUT_DISCARDS,
          SAI_PORT_STAT_IF_OUT_ERRORS,
          SAI_PORT_STAT_PAUSE_TX_PKTS,
      };
      counterIds.reserve(
          counterIds.size() + SaiPortTraits::PfcCounterIdsToRead.size());
      std::copy(
          SaiPortTraits::PfcCounterIdsToRead.begin(),
          SaiPortTraits::PfcCounterIdsToRead.end(),
          std::back_inserter(counterIds));
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
    if (platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
      counterIds.reserve(
          counterIds.size() + SaiPortTraits::PfcCounterIdsToRead.size());
      std::copy(
          SaiPortTraits::PfcCounterIdsToRead.begin(),
          SaiPortTraits::PfcCounterIdsToRead.end(),
          std::back_inserter(counterIds));
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

  if (swPort->isEnabled()) {
    portStats_.emplace(
        swPort->getID(), std::make_unique<HwPortFb303Stats>(swPort->getName()));
  }

  bool samplingMirror = swPort->getSampleDestination().has_value() &&
      swPort->getSampleDestination() == cfg::SampleDestination::MIRROR;
  SaiPortMirrorInfo mirrorInfo{
      swPort->getIngressMirror(), swPort->getEgressMirror(), samplingMirror};
  handle->mirrorInfo = mirrorInfo;
  handles_.emplace(swPort->getID(), std::move(handle));
  if (globalDscpToTcQosMap_) {
    // Both global maps must exist in one of them exists
    CHECK(globalTcToQueueQosMap_);
    auto qosMaps = getSaiIdsForQosMaps();
    setQosMaps(qosMaps, {swPort->getID()});
  }

  addSamplePacket(swPort);
  addMirror(swPort);
  addPfc(swPort);
  programPfcBuffers(swPort);

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());

  // set the lower 32-bit of SaiId as Hardware logical port ID
  auto portSaiId = saiPort->adapterKey();
  uint32_t hwLogicalPortId = static_cast<uint32_t>(portSaiId);
  platformPort->setHwLogicalPortId(hwLogicalPortId);
  return portSaiId;
}

void SaiPortManager::loadPortQueuesForAddedPort(
    const std::shared_ptr<Port>& swPort) {
  loadPortQueues(*swPort);
}
void SaiPortManager::loadPortQueuesForChangedPort(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  SaiPortTraits::CreateAttributes oldAttributes = attributesFromSwPort(oldPort);
  SaiPortTraits::CreateAttributes newAttributes = attributesFromSwPort(newPort);

  if (createOnlyAttributeChanged(oldAttributes, newAttributes)) {
    // If createOnly attributes did not change we would have
    // handled queue updates in changePortImpl itself
    loadPortQueues(*newPort);
  }
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
    changePortByRecreate(oldPort, newPort);
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
  programPfcBuffers(newPort);

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
  changeQueue(newPort, oldPort->getPortQueues(), newPort->getPortQueues());
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
  if (!hwLaneListIsPmdLaneList_) {
    // On Tomahawk4, HwLaneList means physical port list instead of pmd lane
    // list for now. One physical port maps to two pmd lanes. So, do the
    // conversion here PMD lanes ==> physical ports [1,2,3,4] ==> [1,2]
    // [5,6,7,8] ==> [3,4]
    // ......
    std::vector<uint32_t> pportList;
    for (int i = 0; i < hwLaneList.size() / 2; i++) {
      pportList.push_back(hwLaneList[i * 2 + 1] / 2);
    }
    hwLaneList = pportList;
  }
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
  std::optional<sai_port_media_type_t> mediaType;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::MEDIA_TYPE)) {
    if (FLAGS_sai_use_interface_type_for_medium) {
      // We are currently programming different media type for BCM-SAI
      // implementation vs Native SDK implementation. That's because for
      // native-bcm implementation, we derive the media type from interfaceType
      // in platform mapping. However, we use the 'medium' field from platform
      // mapping for the SAI implementation. This causes us to program a
      // sub-optimal media type with SAI (and cause issues like S280146) as for
      // certain profiles, the medium type is defined as TransmitterTech.OPTICAL
      // whereas the interface type is InterfaceType.KR4 and they both currently
      // lead to different SAI_PORT_MEDIA_TYPE. Since changing the media type is
      // a disruptive change, we are guarding this with a flag for now as that
      // will help us stage the roll out of this change. Eventually, we'll get
      // rid of this flag and make deriving the media type from interfaceType as
      // default
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
  }
  std::optional<SaiPortTraits::Attributes::FecMode> fecMode;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::FEC)) {
    auto enableFec = (speed >= cfg::PortSpeed::HUNDREDG) ||
        !platformPort->shouldDisableFEC();
    if (!enableFec) {
      fecMode = SAI_PORT_FEC_MODE_NONE;
    } else {
      fecMode = utility::getSaiPortFecMode(*portProfileConfig.fec());
    }
  }
  std::optional<SaiPortTraits::Attributes::InterfaceType> interfaceType{};
  // TODO(joseph5wu) Maybe provide a new function to convert interfaceType from
  // profile config to sai interface type
  if (auto saiInterfaceType =
          platform_->getInterfaceType(transmitterTech, speed)) {
    interfaceType = saiInterfaceType.value();
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
  std::optional<SaiPortTraits::Attributes::InterFrameGap> interFrameGap;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::MACSEC) &&
      portProfileConfig.interPacketGapBits().has_value()) {
    interFrameGap = *portProfileConfig.interPacketGapBits();
  }
#endif
  auto ptpStatusOpt = managerTable_->switchManager().getPtpTcEnabled();
  uint16_t vlanId = swPort->getIngressVlan();
  auto systemPortId = getSystemPortId(platform_, swPort->getID());
  return SaiPortTraits::CreateAttributes {
    hwLaneList, static_cast<uint32_t>(speed), adminState, fecMode,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
        std::nullopt, std::nullopt,
#endif
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
#if !defined(TAJO_SDK)
        std::nullopt, // PFC Rx Priorities
        std::nullopt, // PFC Tx Priorities
#endif
        std::nullopt, // TC to Priority Group map
        std::nullopt, // PFC Priority to Queue map
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
        interFrameGap, // Inter Frame Gap
#endif
        std::nullopt, // Link Training Enable
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

void SaiPortManager::programSerdes(
    std::shared_ptr<SaiPort> saiPort,
    std::shared_ptr<Port> swPort,
    SaiPortHandle* portHandle) {
  if (!platform_->isSerdesApiSupported() ||
      swPort->getPortType() == cfg::PortType::RECYCLE_PORT) {
    return;
  }

  auto& portKey = saiPort->adapterHostKey();
  SaiPortSerdesTraits::AdapterHostKey serdesKey{saiPort->adapterKey()};
  auto& store = saiStore_->get<SaiPortSerdesTraits>();
  // check if serdes object already exists for given port,
  // either already programmed or reloaded from adapter.
  std::shared_ptr<SaiPortSerdes> serdes = store.get(serdesKey);

  if (!serdes) {
    /* Serdes programming model is changed starting brcm-sai 6.0.0.14, where
     * Serdes objects are no longer created beforehand. Therefore, agent only
     * needs to create a new serdes object without knowing the serdes Id or
     * reloading the object to sai store.
     */
#if !defined(SAI_VERSION_7_2_0_0_ODP) && !defined(SAI_VERSION_8_2_0_0_ODP) && \
    !defined(SAI_VERSION_8_2_0_0_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_8_2_0_0_SIM_ODP) &&                                  \
    !defined(SAI_VERSION_9_0_EA_ODP) && !defined(SAI_VERSION_9_0_EA_DNX_ODP)
    // serdes is not yet programmed or reloaded from adapter
    std::optional<SaiPortTraits::Attributes::SerdesId> serdesAttr{};
    auto serdesId = SaiApiTable::getInstance()->portApi().getAttribute(
        saiPort->adapterKey(), serdesAttr);
    if (serdesId.has_value() && serdesId.value() != SAI_NULL_OBJECT_ID) {
      // but default serdes exists in the adapter.
      serdes =
          store.reloadObject(static_cast<PortSerdesSaiId>(serdesId.value()));
    }
#endif
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
    if (auto tx = pinConfig.tx()) {
      ++numExpectedTxLanes;
    }
    if (auto rx = pinConfig.rx()) {
      ++numExpectedRxLanes;
    }
  }
  auto numPmdLanes = portKey.value().size();
  if (!hwLaneListIsPmdLaneList_) {
    // On Tomahawk4, HwLaneList means physical port list instead of pmd lane
    // list for now. One physical port maps to two pmd lanes.
    numPmdLanes *= 2;
  }
  if (numExpectedTxLanes) {
    CHECK_EQ(numExpectedTxLanes, numPmdLanes)
        << "some lanes are missing for tx-settings";
  }
  if (numExpectedRxLanes) {
    CHECK_EQ(numExpectedRxLanes, numPmdLanes)
        << "some lanes are missing for rx-settings";
  }

  SaiPortSerdesTraits::CreateAttributes serdesAttributes =
      serdesAttributesFromSwPinConfigs(
          saiPort->adapterKey(), swPort->getPinConfigs(), serdes);
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

  if (platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_GARONNE) {
    /*
     * SI settings are not programmed to the hardware when the port is
     * created with admin UP. We need to explicitly toggle the admin
     * state temporarily on Garonne to bring the port up to unblock testing.
     * NOTE this will cause flaps during warmboots and cannot be deployed.
     */
    SaiPortTraits::Attributes::AdminState adminDisable{false};
    SaiApiTable::getInstance()->portApi().setAttribute(
        portHandle->port->adapterKey(), adminDisable);
    SaiPortTraits::Attributes::AdminState adminEnable{true};
    SaiApiTable::getInstance()->portApi().setAttribute(
        portHandle->port->adapterKey(), adminEnable);
  }
}

SaiPortSerdesTraits::CreateAttributes
SaiPortManager::serdesAttributesFromSwPinConfigs(
    PortSaiId portSaiId,
    const std::vector<phy::PinConfig>& pinConfigs,
    const std::shared_ptr<SaiPortSerdes>& serdes) {
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

#if defined(TAJO_SDK_VERSION_1_56_1) || defined(TAJO_SDK_VERSION_1_58_1)
  SaiPortSerdesTraits::Attributes::TxDiffEncoderEn::ValueType txDiffEncoderEn;
  SaiPortSerdesTraits::Attributes::TxDigGain::ValueType txDigGain;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff0::ValueType txFfeCoeff0;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff1::ValueType txFfeCoeff1;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff2::ValueType txFfeCoeff2;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff3::ValueType txFfeCoeff3;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff4::ValueType txFfeCoeff4;
  SaiPortSerdesTraits::Attributes::TxParityEncoderEn::ValueType
      txParityEncoderEn;
  SaiPortSerdesTraits::Attributes::TxThpEn::ValueType txThpEn;

  SaiPortSerdesTraits::Attributes::RxChannelReach::ValueType rxChannelReach;
  SaiPortSerdesTraits::Attributes::RxDiffEncoderEn::ValueType rxDiffEncoderEn;
  SaiPortSerdesTraits::Attributes::RxFbfCoefInitVal::ValueType rxFbfCoefInitVal;
  SaiPortSerdesTraits::Attributes::RxFbfLmsEnable::ValueType rxFbfLmsEnable;
  SaiPortSerdesTraits::Attributes::RxInstgScanOptimize::ValueType
      rxInstgScanOptimize;
  SaiPortSerdesTraits::Attributes::RxInstgTableEndRow::ValueType
      rxInstgTableEndRow;
  SaiPortSerdesTraits::Attributes::RxInstgTableStartRow::ValueType
      rxInstgTableStartRow;
  SaiPortSerdesTraits::Attributes::RxParityEncoderEn::ValueType
      rxParityEncoderEn;
  SaiPortSerdesTraits::Attributes::RxThpEn::ValueType rxThpEn;
#endif

  // Now use pinConfigs from SW port as the source of truth
  auto numExpectedTxLanes = 0;
  auto numExpectedRxLanes = 0;
  for (const auto& pinConfig : pinConfigs) {
    if (auto tx = pinConfig.tx()) {
      ++numExpectedTxLanes;
      txPre1.push_back(*tx->pre());
      txMain.push_back(*tx->main());
      txPost1.push_back(*tx->post());
      if (auto driveCurrent = tx->driveCurrent()) {
        txIDriver.push_back(driveCurrent.value());
      }
#if defined(TAJO_SDK_VERSION_1_56_1) || defined(TAJO_SDK_VERSION_1_58_1)
      if (auto diffEncoderEn = tx->diffEncoderEn()) {
        txDiffEncoderEn.push_back(diffEncoderEn.value());
      }
      if (auto digGain = tx->digGain()) {
        txDigGain.push_back(digGain.value());
      }
      if (auto ffeCoeff0 = tx->ffeCoeff0()) {
        txFfeCoeff0.push_back(ffeCoeff0.value());
      }
      if (auto ffeCoeff1 = tx->ffeCoeff1()) {
        txFfeCoeff1.push_back(ffeCoeff1.value());
      }
      if (auto ffeCoeff2 = tx->ffeCoeff2()) {
        txFfeCoeff2.push_back(ffeCoeff2.value());
      }
      if (auto ffeCoeff3 = tx->ffeCoeff3()) {
        txFfeCoeff3.push_back(ffeCoeff3.value());
      }
      if (auto ffeCoeff4 = tx->ffeCoeff4()) {
        txFfeCoeff4.push_back(ffeCoeff4.value());
      }
      if (auto parityEncoderEn = tx->parityEncoderEn()) {
        txParityEncoderEn.push_back(parityEncoderEn.value());
      }
      if (auto thpEn = tx->thpEn()) {
        txThpEn.push_back(thpEn.value());
      }
#endif
    }
    if (auto rx = pinConfig.rx()) {
      ++numExpectedRxLanes;
      if (auto ctlCode = rx->ctlCode()) {
        rxCtleCode.push_back(*ctlCode);
      }
      if (auto dscpMode = rx->dspMode()) {
        rxDspMode.push_back(*dscpMode);
      }
      if (auto afeTrim = rx->afeTrim()) {
        rxAfeTrim.push_back(*afeTrim);
      }
      if (auto acCouplingByPass = rx->acCouplingBypass()) {
        rxAcCouplingByPass.push_back(*acCouplingByPass);
      }
#if defined(TAJO_SDK_VERSION_1_56_1) || defined(TAJO_SDK_VERSION_1_58_1)
      if (auto channelReach = rx->channelReach()) {
        rxChannelReach.push_back(*channelReach);
      }
      if (auto diffEncoderEn = rx->diffEncoderEn()) {
        rxDiffEncoderEn.push_back(*diffEncoderEn);
      }
      if (auto fbfCoefInitVal = rx->fbfCoefInitVal()) {
        rxFbfCoefInitVal.push_back(*fbfCoefInitVal);
      }
      if (auto fbfLmsEnable = rx->fbfLmsEnable()) {
        rxFbfLmsEnable.push_back(*fbfLmsEnable);
      }
      if (auto instgScanOptimize = rx->instgScanOptimize()) {
        rxInstgScanOptimize.push_back(*instgScanOptimize);
      }
      if (auto instgTableEndRow = rx->instgTableEndRow()) {
        rxInstgTableEndRow.push_back(*instgTableEndRow);
      }
      if (auto instgTableStartRow = rx->instgTableStartRow()) {
        rxInstgTableStartRow.push_back(*instgTableStartRow);
      }
      if (auto parityEncoderEn = rx->parityEncoderEn()) {
        rxParityEncoderEn.push_back(*parityEncoderEn);
      }
      if (auto thpEn = rx->thpEn()) {
        rxThpEn.push_back(*thpEn);
      }
#endif
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

#if defined(TAJO_SDK_VERSION_1_56_1) || defined(TAJO_SDK_VERSION_1_58_1)
  setTxRxAttr(
      attrs,
      SaiPortSerdesTraits::Attributes::TxDiffEncoderEn{},
      txDiffEncoderEn);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxDigGain{}, txDigGain);
  setTxRxAttr(
      attrs, SaiPortSerdesTraits::Attributes::TxFfeCoeff0{}, txFfeCoeff0);
  setTxRxAttr(
      attrs, SaiPortSerdesTraits::Attributes::TxFfeCoeff1{}, txFfeCoeff1);
  setTxRxAttr(
      attrs, SaiPortSerdesTraits::Attributes::TxFfeCoeff2{}, txFfeCoeff2);
  setTxRxAttr(
      attrs, SaiPortSerdesTraits::Attributes::TxFfeCoeff3{}, txFfeCoeff3);
  setTxRxAttr(
      attrs, SaiPortSerdesTraits::Attributes::TxFfeCoeff4{}, txFfeCoeff4);
  setTxRxAttr(
      attrs,
      SaiPortSerdesTraits::Attributes::TxParityEncoderEn{},
      txParityEncoderEn);
  setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxThpEn{}, txThpEn);
#endif

  if (numExpectedRxLanes) {
    setTxRxAttr(
        attrs, SaiPortSerdesTraits::Attributes::RxCtleCode{}, rxCtleCode);

    /*
     * TODO (srikrishnagopu) : Remove after rolling out AFE TRIM (S249471)
     * Coldboot. The DspMode and AfeTrim values provided by vendor are swapped
     * (incorrect) for 25G serdes. With this diff stack (D31939792), it is
     * rectified but this cannot be rolled out during warmboot since this will
     * cause port flaps.
     *
     * Hence, swap the values back if its a warmboot. As we are scheduling
     * coldboots for AFE TRIM SEV S249471, utilize this to propogate the correct
     * values.
     *
     * This will help us carry on the warmboot upgrades on 400C devices while
     * we wait on coldboots to happen over a period of 3 months.
     *
     * --------------------------------------------------------------------
     *               | PIN DSP | PIN AFE | STORE DSP | STORE AFE | ACTION |
     * --------------------------------------------------------------------
     * WB before CB  |    7    |    4    |     4     |     7     |  SWAP  |
     * --------------------------------------------------------------------
     *       CB      |    7    |    4    |     -     |     -     |  SKIP  |
     * --------------------------------------------------------------------
     * WB after CB   |    7    |    4    |     7     |     4     |  SKIP  |
     * --------------------------------------------------------------------
     */
    if (platform_->getHwSwitch()->getBootType() == BootType::WARM_BOOT &&
        platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO &&
        serdes) {
      auto rxDspModeFromStore = std::get<std::optional<std::decay_t<decltype(
          SaiPortSerdesTraits::Attributes::RxDspMode{})>>>(
          serdes->attributes());
      auto rxAfeTrimFromStore = std::get<std::optional<std::decay_t<decltype(
          SaiPortSerdesTraits::Attributes::RxAfeTrim{})>>>(
          serdes->attributes());
      if (!rxDspMode.empty() && !rxAfeTrim.empty() &&
          rxDspModeFromStore.has_value() && rxAfeTrimFromStore.has_value() &&
          !rxDspModeFromStore.value().value().empty() &&
          !rxAfeTrimFromStore.value().value().empty() && rxDspMode[0] == 7 &&
          rxAfeTrim[0] == 4 && rxDspModeFromStore.value().value()[0] == 4 &&
          rxAfeTrimFromStore.value().value()[0] == 7) {
        rxDspMode.swap(rxAfeTrim);
      }
    }
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxDspMode{}, rxDspMode);
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxAfeTrim{}, rxAfeTrim);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxAcCouplingByPass{},
        rxAcCouplingByPass);
#if defined(TAJO_SDK_VERSION_1_56_1) || defined(TAJO_SDK_VERSION_1_58_1)
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxChannelReach{},
        rxChannelReach);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxDiffEncoderEn{},
        rxDiffEncoderEn);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxFbfCoefInitVal{},
        rxFbfCoefInitVal);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxFbfLmsEnable{},
        rxFbfLmsEnable);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxInstgScanOptimize{},
        rxInstgScanOptimize);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxInstgTableEndRow{},
        rxInstgTableEndRow);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxInstgTableStartRow{},
        rxInstgTableStartRow);
    setTxRxAttr(
        attrs,
        SaiPortSerdesTraits::Attributes::RxParityEncoderEn{},
        rxParityEncoderEn);
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::RxThpEn{}, rxThpEn);
#endif
  }
  return attrs;
}
} // namespace facebook::fboss
