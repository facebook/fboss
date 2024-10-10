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

#if defined(BRCM_SAI_SDK_DNX)
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiportextensions.h>
#else
#include <saiportextensions.h>
#endif
#endif

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

#if defined(BRCM_SAI_SDK_DNX)
sai_int32_t getPortTypeFromCfg(const cfg::PortType& cfgPortType) {
  switch (cfgPortType) {
    case cfg::PortType::MANAGEMENT_PORT:
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
      return SAI_PORT_TYPE_MGMT;
#endif
    case cfg::PortType::EVENTOR_PORT:
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
      return SAI_PORT_TYPE_EVENTOR;
#endif
    case cfg::PortType::INTERFACE_PORT:
      return SAI_PORT_TYPE_LOGICAL;
    case cfg::PortType::FABRIC_PORT:
      return SAI_PORT_TYPE_FABRIC;
    case cfg::PortType::CPU_PORT:
      return SAI_PORT_TYPE_CPU;
    case cfg::PortType::RECYCLE_PORT:
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      return SAI_PORT_TYPE_RECYCLE;
#else
      throw FbossError("RECYCLE_PORT is not supported");
#endif
  }

  throw FbossError(
      "Invalid port type", apache::thrift::util::enumNameSafe(cfgPortType));
}
#endif

SaiPortTraits::AdapterHostKey getPortAdapterHostKeyFromAttr(
    const SaiPortTraits::CreateAttributes& attributes) {
  SaiPortTraits::AdapterHostKey portKey{
#if defined(BRCM_SAI_SDK_DNX)
      GET_ATTR(Port, Type, attributes),
#endif
      GET_ATTR(Port, HwLaneList, attributes)};

  return portKey;
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
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
      if (platform_->getAsic()->isSupported(
              HwAsic::Feature::FABRIC_LINK_DOWN_CELL_DROP_COUNTER)) {
        counterIds.emplace_back(SAI_PORT_STAT_IF_IN_LINK_DOWN_CELL_DROP);
      }
#endif
      return counterIds;
    }
    if (getPortType(port) == cfg::PortType::RECYCLE_PORT) {
      if (platform_->getAsic()->isSupported(
              HwAsic::Feature::SAI_PORT_ETHER_STATS)) {
        return std::vector<sai_stat_id_t>{
            SAI_PORT_STAT_ETHER_STATS_RX_NO_ERRORS};
      } else {
        return std::vector<sai_stat_id_t>{
            SAI_PORT_STAT_IF_IN_UCAST_PKTS,
        };
      }
    }
    std::set<sai_stat_id_t> countersToFilter;
    if (!platform_->getAsic()->isSupported(HwAsic::Feature::ECN)) {
      countersToFilter.insert(SAI_PORT_STAT_ECN_MARKED_PACKETS);
    }
    if (!platform_->getAsic()->isSupported(
            HwAsic::Feature::PORT_WRED_COUNTER)) {
      countersToFilter.insert(SAI_PORT_STAT_WRED_DROPPED_PACKETS);
    }
    if (getPortType(port) == cfg::PortType::MANAGEMENT_PORT &&
        platform_->getAsic()->getAsicType() ==
            cfg::AsicType::ASIC_TYPE_TOMAHAWK5) {
      // TODO(daiweix): follow-up with brcm why this basic stats
      // does not work for TH5 management port
      countersToFilter.insert(SAI_PORT_STAT_IF_IN_OCTETS);
    }
    counterIds.reserve(SaiPortTraits::CounterIdsToRead.size() + 1);
    std::copy_if(
        SaiPortTraits::CounterIdsToRead.begin(),
        SaiPortTraits::CounterIdsToRead.end(),
        std::back_inserter(counterIds),
        [&countersToFilter](auto statId) {
          return countersToFilter.find(statId) == countersToFilter.end();
        });
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::BLACKHOLE_ROUTE_DROP_COUNTER)) {
      counterIds.emplace_back(managerTable_->debugCounterManager()
                                  .getPortL3BlackHoleCounterStatId());
    }
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER)) {
#if !defined(TAJO_SDK_VERSION_1_42_8)
      counterIds.emplace_back(managerTable_->debugCounterManager()
                                  .getMPLSLookupFailedCounterStatId());
#endif
    }
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::ANY_ACL_DROP_COUNTER)) {
      counterIds.emplace_back(
          managerTable_->debugCounterManager().getAclDropCounterStatId());
    }
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::EGRESS_FORWARDING_DROP_COUNTER)) {
      counterIds.emplace_back(
          managerTable_->debugCounterManager().getEgressForwardingDropStatId());
    }
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::ANY_TRAP_DROP_COUNTER)) {
      counterIds.emplace_back(
          managerTable_->debugCounterManager().getTrapDropCounterStatId());
    }
    if (platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
      counterIds.reserve(
          counterIds.size() + SaiPortTraits::PfcCounterIdsToRead.size());
      std::copy(
          SaiPortTraits::PfcCounterIdsToRead.begin(),
          SaiPortTraits::PfcCounterIdsToRead.end(),
          std::back_inserter(counterIds));

      if (platform_->getAsic()->isSupported(
              HwAsic::Feature::PFC_XON_TO_XOFF_COUNTER)) {
        counterIds.reserve(
            counterIds.size() +
            SaiPortTraits::PfcXonToXoffCounterIdsToRead.size());
        std::copy(
            SaiPortTraits::PfcXonToXoffCounterIdsToRead.begin(),
            SaiPortTraits::PfcXonToXoffCounterIdsToRead.end(),
            std::back_inserter(counterIds));
      }
    }
    // ETHER stats used on j3 sim
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::SAI_PORT_ETHER_STATS)) {
      counterIds.emplace_back(SAI_PORT_STAT_ETHER_STATS_TX_NO_ERRORS);
      counterIds.emplace_back(SAI_PORT_STAT_ETHER_STATS_RX_NO_ERRORS);
    }
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::PQP_ERROR_EGRESS_DROP_COUNTER)) {
      counterIds.emplace_back(
          SAI_PORT_STAT_OUT_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS);
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

  SaiPortTraits::AdapterHostKey portKey{
      getPortAdapterHostKeyFromAttr(attributes)};
  auto handle = std::make_unique<SaiPortHandle>();

  auto& portStore = saiStore_->get<SaiPortTraits>();
  auto saiPort = portStore.setObject(portKey, attributes, swPort->getID());
  handle->port = saiPort;
  programSerdes(saiPort, swPort, handle.get());

  if (swPort->isEnabled()) {
    HwBasePortFb303Stats::QueueId2Name queueId2Name{};
    portStats_.emplace(
        swPort->getID(),
        std::make_unique<HwPortFb303Stats>(
            swPort->getName(), queueId2Name, swPort->getPfcPriorities()));
  }

  bool samplingMirror = swPort->getSampleDestination().has_value() &&
      swPort->getSampleDestination() == cfg::SampleDestination::MIRROR;
  SaiPortMirrorInfo mirrorInfo{
      swPort->getIngressMirror(), swPort->getEgressMirror(), samplingMirror};
  handle->mirrorInfo = mirrorInfo;
  handles_.emplace(swPort->getID(), std::move(handle));
  setQosPolicy(swPort->getID(), swPort->getQosPolicy());

  addSamplePacket(swPort);
  addNode(swPort);
  addPfc(swPort);
  programPfcBuffers(swPort);

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());

  // set the lower 32-bit of SaiId as Hardware logical port ID
  auto portSaiId = saiPort->adapterKey();
  uint32_t hwLogicalPortId = static_cast<uint32_t>(portSaiId);
  XLOG(DBG2) << "swPort ID: " << swPort->getID()
             << " hwLogicalPort ID: " << hwLogicalPortId;
  platformPort->setHwLogicalPortId(hwLogicalPortId);
  auto asicPrbs = swPort->getAsicPrbs();
  if (asicPrbs.enabled().value()) {
    initAsicPrbsStats(swPort);
  }
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
      getPortAdapterHostKeyFromAttr(newAttributes)};
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
  auto oldAsicPrbsEnabled = oldPort->getAsicPrbs().enabled().value();
  auto newAsicPrbsEnabled = newPort->getAsicPrbs().enabled().value();
  if (newPort->getProfileID() != oldPort->getProfileID()) {
    auto platformPort = platform_->getPort(newPort->getID());
    platformPort->setCurrentProfile(newPort->getProfileID());
    if (oldAsicPrbsEnabled == newAsicPrbsEnabled && newAsicPrbsEnabled) {
      updatePrbsStatsEntryRate(newPort);
    }
  }

  changeMirror(oldPort, newPort);
  changeSamplePacket(oldPort, newPort);
  changePfc(oldPort, newPort);
  changeRxLaneSquelch(oldPort, newPort);
  changeZeroPreemphasis(oldPort, newPort);
  changeTxEnable(oldPort, newPort);
  programPfcBuffers(newPort);

  if (newPort->isEnabled()) {
    if (!oldPort->isEnabled()) {
      // Port transitioned from disabled to enabled, setup port stats
      HwBasePortFb303Stats::QueueId2Name queueId2Name{};
      portStats_.emplace(
          newPort->getID(),
          std::make_unique<HwPortFb303Stats>(
              newPort->getName(), queueId2Name, newPort->getPfcPriorities()));
    } else if (oldPort->getName() != newPort->getName()) {
      // Port was already enabled, but Port name changed - update stats
      portStats_.find(newPort->getID())
          ->second->portNameChanged(newPort->getName());
    }
    if (oldPort->getPfc() != newPort->getPfc()) {
      portStats_.find(newPort->getID())
          ->second->pfcPriorityChanged(newPort->getPfcPriorities());
    }
  } else if (oldPort->isEnabled()) {
    // Port transitioned from enabled to disabled, remove stats
    portStats_.erase(newPort->getID());
  }
  changeQueue(
      newPort,
      oldPort->getPortQueues()->impl(),
      newPort->getPortQueues()->impl());
  changeQosPolicy(oldPort, newPort);
  if (oldAsicPrbsEnabled != newAsicPrbsEnabled) {
    if (newAsicPrbsEnabled) {
      initAsicPrbsStats(newPort);
    } else {
      auto portAsicPrbsStatsItr = portAsicPrbsStats_.find(newPort->getID());
      if (portAsicPrbsStatsItr == portAsicPrbsStats_.end()) {
        throw FbossError(
            "Asic prbs lane error map not initialized for port ",
            newPort->getID());
      }
      portAsicPrbsStats_.erase(newPort->getID());
    }
  }
  if (newPort->isUp() != oldPort->isUp() && !newPort->isUp()) {
    resetCableLength(newPort->getID());
  }
}

void SaiPortManager::attributesFromSaiStore(
    SaiPortTraits::CreateAttributes& attributes) {
  SaiPortTraits::AdapterHostKey portKey{
      getPortAdapterHostKeyFromAttr(attributes)};

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
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::IngressSampleMirrorSession{});
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::EgressSampleMirrorSession{});
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
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::QosTcToPriorityGroupMap{});
  getAndSetAttribute(
      port->attributes(),
      attributes,
      SaiPortTraits::Attributes::QosPfcPriorityToQueueMap{});
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    getAndSetAttribute(
        port->attributes(),
        attributes,
        SaiPortTraits::Attributes::DisableTtlDecrement{});
  }
}

SaiPortTraits::CreateAttributes SaiPortManager::attributesFromSwPort(
    const std::shared_ptr<Port>& swPort,
    bool /* lineSide */,
    bool basicAttributeOnly) const {
  bool adminState =
      swPort->getAdminState() == cfg::PortState::ENABLED ? true : false;

#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
  std::optional<bool> isDrained = std::nullopt;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::PORT_FABRIC_ISOLATE)) {
    if (swPort->getPortType() == cfg::PortType::FABRIC_PORT) {
      isDrained = swPort->getPortDrainState() == cfg::PortDrainState::DRAINED;
    } else if (swPort->getPortDrainState() == cfg::PortDrainState::DRAINED) {
      throw FbossError(
          "Cannot isolate/drain a non-fabric port ", swPort->getID());
    }
  }
#endif

  auto portID = swPort->getID();
  auto platformPort = platform_->getPort(portID);
  auto speed = swPort->getSpeed();
  auto hwLaneList = platformPort->getHwPortLanes(swPort->getProfileID());
  if (!hwLaneListIsPmdLaneList_) {
    // On Tomahawk4, HwLaneList means physical port list instead of pmd lane
    // list for now. One physical port maps to two pmd lanes. So, do the
    // conversion here PMD lanes ==> physical ports, e.g.
    // [1,2,3,4] ==> [1,2]
    // [5,6,7,8] ==> [3,4]
    // ......
    // If only has one lane (e.g. 10G case), map to one physical port, e.g.
    // [1] ==> [1]
    // [5] ==> [3]
    // ......
    std::vector<uint32_t> pportList;
    for (int i = 0; i < std::max(1, (int)hwLaneList.size() / 2); i++) {
      pportList.push_back((hwLaneList[i * 2] + 1) / 2);
    }
    hwLaneList = pportList;
  }
  auto globalFlowControlMode = utility::getSaiPortPauseMode(swPort->getPause());
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  auto loopbackMode =
      utility::getSaiPortLoopbackMode(swPort->getLoopbackMode());
#else
  auto internalLoopbackMode =
      utility::getSaiPortInternalLoopbackMode(swPort->getLoopbackMode());
#endif

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
    auto enableFec = (speed >= cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG) ||
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
  std::optional<SaiPortTraits::Attributes::LinkTrainingEnable>
      linkTrainingEnable;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::LINK_TRAINING)) {
    linkTrainingEnable = false;
  }

  std::optional<bool> fdrEnable;
#if defined(BRCM_SAI_SDK_GTE_10_0) || defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  if (swPort->getPortType() == cfg::PortType::INTERFACE_PORT && adminState &&
      platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_FEC_CODEWORDS_STATS)) {
    fdrEnable = true;
  }
#endif
  auto ptpStatusOpt = managerTable_->switchManager().getPtpTcEnabled();
  uint16_t vlanId = swPort->getIngressVlan();
  auto systemPortId = getSystemPortId(platform_, swPort->getID());

  // Skip setting MTU for fabric ports if not supported
  std::optional<SaiPortTraits::Attributes::Mtu> mtu{};
  if (swPort->getPortType() != cfg::PortType::FABRIC_PORT ||
      platform_->getAsic()->isSupported(HwAsic::Feature::FABRIC_PORT_MTU)) {
    mtu = swPort->getMaxFrameSize();
  }
  std::optional<SaiPortTraits::Attributes::PrbsPolynomial> prbsPolynomial =
      std::nullopt;
  std::optional<SaiPortTraits::Attributes::PrbsConfig> prbsConfig =
      std::nullopt;
  if (platform_->getAsic()->isSupported(HwAsic::Feature::SAI_PRBS)) {
    auto asicPrbs = swPort->getAsicPrbs();
    prbsConfig = getSaiPortPrbsConfig(asicPrbs.enabled().value());
    if (asicPrbs.enabled().value()) {
      prbsPolynomial =
          static_cast<sai_uint32_t>(asicPrbs.polynominal().value());
    }
  }
  std::optional<SaiPortTraits::Attributes::DisableTtlDecrement> disableTtl{};
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE) &&
      swPort->getTTLDisableDecrement()) {
    disableTtl = SaiPortTraits::Attributes::DisableTtlDecrement{
        swPort->getTTLDisableDecrement().value()};
  }
  std::optional<SaiPortTraits::Attributes::PktTxEnable> pktTxEnable{};
  if (auto txEnable = swPort->getTxEnable()) {
    pktTxEnable = SaiPortTraits::Attributes::PktTxEnable{txEnable.value()};
  }
  auto portPfcInfo = getPortPfcAttributes(swPort);

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  std::optional<SaiPortTraits::Attributes::ArsEnable> arsEnable = std::nullopt;
  std::optional<SaiPortTraits::Attributes::ArsPortLoadScalingFactor>
      arsPortLoadScalingFactor = std::nullopt;
  std::optional<SaiPortTraits::Attributes::ArsPortLoadPastWeight>
      arsPortLoadPastWeight = std::nullopt;
  std::optional<SaiPortTraits::Attributes::ArsPortLoadFutureWeight>
      arsPortLoadFutureWeight = std::nullopt;
  if (FLAGS_flowletSwitchingEnable &&
      platform_->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) {
    auto flowletCfg = swPort->getPortFlowletConfig();
    if (swPort->getFlowletConfigName().has_value() &&
        swPort->getPortFlowletConfig().has_value()) {
      auto flowletCfgPtr = swPort->getPortFlowletConfig().value();
      arsEnable = true;
      arsPortLoadScalingFactor = flowletCfgPtr->getScalingFactor();
      arsPortLoadPastWeight = flowletCfgPtr->getLoadWeight();
      arsPortLoadFutureWeight = flowletCfgPtr->getQueueWeight();
    }
  }
#endif

  std::optional<SaiPortTraits::Attributes::ReachabilityGroup>
      reachabilityGroup{};
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  if (auto reachabilityGroupId = swPort->getReachabilityGroupId()) {
    reachabilityGroup = SaiPortTraits::Attributes::ReachabilityGroup{
        reachabilityGroupId.value()};
  }
#endif

  if (basicAttributeOnly) {
    return SaiPortTraits::CreateAttributes{
#if defined(BRCM_SAI_SDK_DNX)
        getPortTypeFromCfg(swPort->getPortType()),
#endif
        hwLaneList,
        static_cast<uint32_t>(speed),
        adminState,
        fecMode,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
        std::nullopt,
        std::nullopt,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
        std::nullopt, // Port Fabric Isolate
#endif
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        disableTtl,
        std::nullopt,
        pktTxEnable, /* PktTxEnable */
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
#if !defined(TAJO_SDK)
        std::nullopt,
        std::nullopt,
#endif
        std::nullopt,
        std::nullopt,
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
        std::nullopt,
#endif
        std::nullopt, // Link Training Enable
        std::nullopt, // FDR Enable
        std::nullopt, // Rx Squelch Enable
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
        std::nullopt, // PFC Deadlock Detection Interval
        std::nullopt, // PFC Deadlock Recovery Interval
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
        std::nullopt, // ARS enable
        std::nullopt, // ARS scaling factor
        std::nullopt, // ARS port load past weight
        std::nullopt, // ARS port load future weight
#endif
        std::nullopt, // Reachability Group
    };
  }
  return SaiPortTraits::CreateAttributes{
#if defined(BRCM_SAI_SDK_DNX)
      getPortTypeFromCfg(swPort->getPortType()),
#endif
      hwLaneList,
      static_cast<uint32_t>(speed),
      adminState,
      fecMode,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      std::nullopt,
      std::nullopt,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
      isDrained,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      loopbackMode,
#else
      internalLoopbackMode,
#endif
      mediaType,
      globalFlowControlMode,
      vlanId,
      mtu,
      std::nullopt,
      std::nullopt,
      disableTtl,
      interfaceType,
      std::nullopt,
      std::nullopt, // Ingress Mirror Session
      std::nullopt, // Egress Mirror Session
      std::nullopt, // Ingress Sample Packet
      std::nullopt, // Egress Sample Packet
      std::nullopt, // Ingress mirror sample session
      std::nullopt, // Egress mirror sample session
      prbsPolynomial, // PRBS Polynomial
      prbsConfig, // PRBS Config
      std::nullopt, // Ingress MacSec ACL
      std::nullopt, // Egress MacSec ACL
      systemPortId, // System Port Id
      ptpStatusOpt, // PTP Mode, can be std::nullopt
      portPfcInfo.pfcMode, // PFC Mode
      portPfcInfo.pfcTxRx, // PFC Priorities
#if !defined(TAJO_SDK)
      portPfcInfo.pfcRx, // PFC Rx Priorities
      portPfcInfo.pfcTx, // PFC Tx Priorities
#endif
      std::nullopt, // TC to Priority Group map
      std::nullopt, // PFC Priority to Queue map
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
      interFrameGap, // Inter Frame Gap
#endif
      linkTrainingEnable,
      fdrEnable, // FDR Enable,
      std::nullopt, // Rx Lane Squelch Enable
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      std::nullopt, // PFC Deadlock Detection Interval
      std::nullopt, // PFC Deadlock Recovery Interval
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      arsEnable, // ARS enable
      arsPortLoadScalingFactor, // ARS scaling factor
      arsPortLoadPastWeight, // ARS port load past weight
      arsPortLoadFutureWeight, // ARS port load future weight
#endif
      reachabilityGroup,
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
  auto hwLaneListSize =
      GET_ATTR(Port, HwLaneList, portHandle->port->adapterHostKey()).size();
  for (auto i = 0; i < hwLaneListSize; i++) {
    rxAfeAdaptiveEnable.push_back(1);
  }
  auto& store = saiStore_->get<SaiPortSerdesTraits>();
  SaiPortSerdesTraits::AdapterHostKey serdesKey{portHandle->port->adapterKey()};
  auto serdesAttributes = portHandle->serdes->attributes();
  std::get<std::optional<std::decay_t<
      decltype(SaiPortSerdesTraits::Attributes::RxAfeAdaptiveEnable{})>>>(
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
      !platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_PORT_SERDES_PROGRAMMING) ||
      swPort->getPortType() == cfg::PortType::RECYCLE_PORT ||
      swPort->getPortType() == cfg::PortType::EVENTOR_PORT) {
    return;
  }

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
#if !defined(BRCM_SAI_SDK_XGS_AND_DNX)
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
  auto numPmdLanes =
      GET_ATTR(Port, HwLaneList, saiPort->adapterHostKey()).size();
  if (!hwLaneListIsPmdLaneList_) {
    // On Tomahawk4, HwLaneList means physical port list instead of pmd lane
    // list for now. One physical port maps to two pmd lanes, except for one
    // lane use case like 10G.
    if (static_cast<cfg::PortSpeed>(GET_ATTR(
            Port, Speed, saiPort->attributes())) >= cfg::PortSpeed::FORTYG) {
      numPmdLanes *= 2;
    }
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

  if (((platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA ||
        platform_->getAsic()->getAsicType() ==
            cfg::AsicType::ASIC_TYPE_TOMAHAWK5) &&
       platform_->getHwSwitch()->getBootType() == BootType::COLD_BOOT) &&
      swPort->getAdminState() == cfg::PortState::ENABLED) {
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
  SaiPortSerdesTraits::Attributes::TxFirPre3::ValueType txPre3;
  SaiPortSerdesTraits::Attributes::TxFirPre2::ValueType txPre2;
  SaiPortSerdesTraits::Attributes::TxFirPost2::ValueType txPost2;
  SaiPortSerdesTraits::Attributes::TxFirPost3::ValueType txPost3;
  SaiPortSerdesTraits::Attributes::TxLutMode::ValueType txLutMode;
  SaiPortSerdesTraits::Attributes::RxCtleCode::ValueType rxCtleCode;
  SaiPortSerdesTraits::Attributes::RxDspMode::ValueType rxDspMode;
  SaiPortSerdesTraits::Attributes::RxAfeTrim::ValueType rxAfeTrim;
  SaiPortSerdesTraits::Attributes::RxAcCouplingByPass::ValueType
      rxAcCouplingByPass;
  // TX Params
  SaiPortSerdesTraits::Attributes::TxDiffEncoderEn::ValueType txDiffEncoderEn;
  SaiPortSerdesTraits::Attributes::TxDigGain::ValueType txDigGain;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff0::ValueType txFfeCoeff0;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff1::ValueType txFfeCoeff1;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff2::ValueType txFfeCoeff2;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff3::ValueType txFfeCoeff3;
  SaiPortSerdesTraits::Attributes::TxFfeCoeff4::ValueType txFfeCoeff4;
  SaiPortSerdesTraits::Attributes::TxDriverSwing::ValueType txDriverSwing;
  // RX Params
  SaiPortSerdesTraits::Attributes::RxInstgBoost1Start::ValueType
      rxInstgBoost1Start;
  SaiPortSerdesTraits::Attributes::RxInstgBoost1Step::ValueType
      rxInstgBoost1Step;
  SaiPortSerdesTraits::Attributes::RxInstgBoost1Stop::ValueType
      rxInstgBoost1Stop;
  SaiPortSerdesTraits::Attributes::RxInstgBoost2OrHrStart::ValueType
      rxInstgBoost2OrHrStart;
  SaiPortSerdesTraits::Attributes::RxInstgBoost2OrHrStep::ValueType
      rxInstgBoost2OrHrStep;
  SaiPortSerdesTraits::Attributes::RxInstgBoost2OrHrStop::ValueType
      rxInstgBoost2OrHrStop;
  SaiPortSerdesTraits::Attributes::RxInstgC1Start1p7::ValueType
      rxInstgC1Start1p7;
  SaiPortSerdesTraits::Attributes::RxInstgC1Step1p7::ValueType rxInstgC1Step1p7;
  SaiPortSerdesTraits::Attributes::RxInstgC1Stop1p7::ValueType rxInstgC1Stop1p7;
  SaiPortSerdesTraits::Attributes::RxInstgDfeStart1p7::ValueType
      rxInstgDfeStart1p7;
  SaiPortSerdesTraits::Attributes::RxInstgDfeStep1p7::ValueType
      rxInstgDfeStep1p7;
  SaiPortSerdesTraits::Attributes::RxInstgDfeStop1p7::ValueType
      rxInstgDfeStop1p7;
  SaiPortSerdesTraits::Attributes::RxEnableScanSelection::ValueType
      rxEnableScanSelection;
  SaiPortSerdesTraits::Attributes::RxInstgScanUseSrSettings::ValueType
      rxInstgScanUseSrSettings;
  SaiPortSerdesTraits::Attributes::RxCdrCfgOvEn::ValueType rxCdrCfgOvEn;
  SaiPortSerdesTraits::Attributes::RxCdrTdet1stOrdStepOvVal::ValueType
      rxCdrTdet1stOrdStepOvVal;
  SaiPortSerdesTraits::Attributes::RxCdrTdet2ndOrdStepOvVal::ValueType
      rxCdrTdet2ndOrdStepOvVal;
  SaiPortSerdesTraits::Attributes::RxCdrTdetFineStepOvVal::ValueType
      rxCdrTdetFineStepOvVal;

  // Now use pinConfigs from SW port as the source of truth
  auto numExpectedTxLanes = 0;
  auto numExpectedRxLanes = 0;
  for (const auto& pinConfig : pinConfigs) {
    if (auto tx = pinConfig.tx()) {
      ++numExpectedTxLanes;
      if (platform_->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_YUBA) {
        if (auto firPre1 = tx->firPre1()) {
          txPre1.push_back(*firPre1);
        }
        if (auto firPre2 = tx->firPre2()) {
          txPre2.push_back(*firPre2);
        }
        if (auto firPre3 = tx->firPre3()) {
          txPre3.push_back(*firPre3);
        }
        if (auto firMain = tx->firMain()) {
          txMain.push_back(*firMain);
        }
        if (auto firPost1 = tx->firPost1()) {
          txPost1.push_back(*firPost1);
        }
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
        if (auto driverSwing = tx->driverSwing()) {
          txDriverSwing.push_back(driverSwing.value());
        }
      } else {
        txPre1.push_back(*tx->pre());
        txMain.push_back(*tx->main());
        txPost1.push_back(*tx->post());
        if (FLAGS_sai_configure_six_tap &&
            platform_->getAsic()->isSupported(
                HwAsic::Feature::SAI_CONFIGURE_SIX_TAP)) {
          txPost2.push_back(*tx->post2());
          txPost3.push_back(*tx->post3());
          txPre2.push_back(*tx->pre2());
          if (platform_->getAsic()->getAsicVendor() ==
              HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
            if (auto lutMode = tx->lutMode()) {
              txLutMode.push_back(*lutMode);
            }
          }
        }
        if (auto pre3 = tx->pre3()) {
          txPre3.push_back(*pre3);
        }

        if (auto driveCurrent = tx->driveCurrent()) {
          txIDriver.push_back(driveCurrent.value());
        }
      }
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
      // RX Params
      if (auto instgBoost1Start = rx->instgBoost1Start()) {
        rxInstgBoost1Start.push_back(instgBoost1Start.value());
      }
      if (auto instgBoost1Step = rx->instgBoost1Step()) {
        rxInstgBoost1Step.push_back(instgBoost1Step.value());
      }
      if (auto instgBoost1Stop = rx->instgBoost1Stop()) {
        rxInstgBoost1Stop.push_back(instgBoost1Stop.value());
      }
      if (auto instgBoost2OrHrStart = rx->instgBoost2OrHrStart()) {
        rxInstgBoost2OrHrStart.push_back(instgBoost2OrHrStart.value());
      }
      if (auto instgBoost2OrHrStep = rx->instgBoost2OrHrStep()) {
        rxInstgBoost2OrHrStep.push_back(instgBoost2OrHrStep.value());
      }
      if (auto instgBoost2OrHrStop = rx->instgBoost2OrHrStop()) {
        rxInstgBoost2OrHrStop.push_back(instgBoost2OrHrStop.value());
      }
      if (auto instgC1Start1p7 = rx->instgC1Start1p7()) {
        rxInstgC1Start1p7.push_back(instgC1Start1p7.value());
      }
      if (auto instgC1Step1p7 = rx->instgC1Step1p7()) {
        rxInstgC1Step1p7.push_back(instgC1Step1p7.value());
      }
      if (auto instgC1Stop1p7 = rx->instgC1Stop1p7()) {
        rxInstgC1Stop1p7.push_back(instgC1Stop1p7.value());
      }
      if (auto instgDfeStart1p7 = rx->instgDfeStart1p7()) {
        rxInstgDfeStart1p7.push_back(instgDfeStart1p7.value());
      }
      if (auto instgDfeStep1p7 = rx->instgDfeStep1p7()) {
        rxInstgDfeStep1p7.push_back(instgDfeStep1p7.value());
      }
      if (auto instgDfeStop1p7 = rx->instgDfeStop1p7()) {
        rxInstgDfeStop1p7.push_back(instgDfeStop1p7.value());
      }
      if (auto enableScanSelection = rx->enableScanSelection()) {
        rxEnableScanSelection.push_back(enableScanSelection.value());
      }
      if (auto instgScanUseSrSettings = rx->instgScanUseSrSettings()) {
        rxInstgScanUseSrSettings.push_back(instgScanUseSrSettings.value());
      }
      if (auto cdrCfgOvEn = rx->cdrCfgOvEn()) {
        rxCdrCfgOvEn.push_back(cdrCfgOvEn.value());
      }
      if (auto cdrTdet1stOrdStepOvVal = rx->cdrTdet1stOrdStepOvVal()) {
        rxCdrTdet1stOrdStepOvVal.push_back(cdrTdet1stOrdStepOvVal.value());
      }
      if (auto cdrTdet2ndOrdStepOvVal = rx->cdrTdet2ndOrdStepOvVal()) {
        rxCdrTdet2ndOrdStepOvVal.push_back(cdrTdet2ndOrdStepOvVal.value());
      }
      if (auto cdrTdetFineStepOvVal = rx->cdrTdetFineStepOvVal()) {
        rxCdrTdetFineStepOvVal.push_back(cdrTdetFineStepOvVal.value());
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

  if (FLAGS_sai_configure_six_tap &&
      platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_CONFIGURE_SIX_TAP)) {
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPre2{}, txPre2);
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPost2{}, txPost2);
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPost3{}, txPost3);
    if (platform_->getAsic()->getAsicVendor() ==
        HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
      setTxRxAttr(
          attrs, SaiPortSerdesTraits::Attributes::TxLutMode{}, txLutMode);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::TxDiffEncoderEn{},
          txDiffEncoderEn);
      setTxRxAttr(
          attrs, SaiPortSerdesTraits::Attributes::TxDigGain{}, txDigGain);
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
          SaiPortSerdesTraits::Attributes::TxDriverSwing{},
          txDriverSwing);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgBoost1Start{},
          rxInstgBoost1Start);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgBoost1Step{},
          rxInstgBoost1Step);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgBoost1Stop{},
          rxInstgBoost1Stop);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgBoost2OrHrStart{},
          rxInstgBoost2OrHrStart);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgBoost2OrHrStep{},
          rxInstgBoost2OrHrStep);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgBoost2OrHrStop{},
          rxInstgBoost2OrHrStop);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgC1Start1p7{},
          rxInstgC1Start1p7);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgC1Step1p7{},
          rxInstgC1Step1p7);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgC1Stop1p7{},
          rxInstgC1Stop1p7);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgDfeStart1p7{},
          rxInstgDfeStart1p7);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgDfeStep1p7{},
          rxInstgDfeStep1p7);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgDfeStop1p7{},
          rxInstgDfeStop1p7);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxEnableScanSelection{},
          rxEnableScanSelection);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxInstgScanUseSrSettings{},
          rxInstgScanUseSrSettings);
      setTxRxAttr(
          attrs, SaiPortSerdesTraits::Attributes::RxCdrCfgOvEn{}, rxCdrCfgOvEn);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxCdrTdet1stOrdStepOvVal{},
          rxCdrTdet1stOrdStepOvVal);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxCdrTdet2ndOrdStepOvVal{},
          rxCdrTdet2ndOrdStepOvVal);
      setTxRxAttr(
          attrs,
          SaiPortSerdesTraits::Attributes::RxCdrTdetFineStepOvVal{},
          rxCdrTdetFineStepOvVal);
    }
  }

  if (!txPre3.empty()) {
    setTxRxAttr(attrs, SaiPortSerdesTraits::Attributes::TxFirPre3{}, txPre3);
  }

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
      auto rxDspModeFromStore = std::get<std::optional<std::decay_t<
          decltype(SaiPortSerdesTraits::Attributes::RxDspMode{})>>>(
          serdes->attributes());
      auto rxAfeTrimFromStore = std::get<std::optional<std::decay_t<
          decltype(SaiPortSerdesTraits::Attributes::RxAfeTrim{})>>>(
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
  }
  return attrs;
}
} // namespace facebook::fboss
