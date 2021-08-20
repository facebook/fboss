// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
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

  concurrentIndices_->portIds.emplace(saiPort->adapterKey(), swPort->getID());
  concurrentIndices_->portSaiIds.emplace(
      swPort->getID(), saiPort->adapterKey());
  concurrentIndices_->vlanIds.emplace(
      PortDescriptorSaiId(saiPort->adapterKey()), swPort->getIngressVlan());
  XLOG(INFO) << "added port " << swPort->getID() << " with vlan "
             << swPort->getIngressVlan();

  // set platform port's speed
  auto platformPort = platform_->getPort(swPort->getID());
  platformPort->setCurrentProfile(swPort->getProfileID());
  return saiPort->adapterKey();
}

SaiPortTraits::CreateAttributes SaiPortManager::attributesFromSwPort(
    const std::shared_ptr<Port>& swPort,
    bool /* lineSide */) const {
  bool adminState =
      swPort->getAdminState() == cfg::PortState::ENABLED ? true : false;
  auto profileID = swPort->getProfileID();
  auto portID = swPort->getID();
  auto platformPort = platform_->getPort(portID);
  auto portProfileConfig = platformPort->getPortProfileConfig(profileID);
  auto speed = *portProfileConfig.speed_ref();
  auto hwLaneList = platformPort->getHwPortLanes(speed);
  auto globalFlowControlMode = utility::getSaiPortPauseMode(swPort->getPause());
  auto internalLoopbackMode =
      utility::getSaiPortInternalLoopbackMode(swPort->getLoopbackMode());

  auto transmitterTech = platformPort->getTransmitterTech();
  if (transmitterTech == TransmitterTechnology::UNKNOWN) {
    if (portProfileConfig.iphy_ref()->medium_ref()) {
      transmitterTech = *portProfileConfig.iphy_ref()->medium_ref();
      XLOG(DBG2) << "Port: " << swPort->getID()
                 << " TransmitterTech fallback to: "
                 << static_cast<int>(transmitterTech);
    }
  }

  auto mediaType = utility::getSaiPortMediaType(transmitterTech, speed);
  auto enableFec =
      (speed >= cfg::PortSpeed::HUNDREDG) || !platformPort->shouldDisableFEC();

  SaiPortTraits::Attributes::FecMode fecMode;
  if (!enableFec) {
    fecMode = SAI_PORT_FEC_MODE_NONE;
  } else {
    auto phyFecMode = platform_->getPhyFecMode(
        PlatformPortProfileConfigMatcher(profileID, portID));
    fecMode = utility::getSaiPortFecMode(phyFecMode);
  }
  uint16_t vlanId = swPort->getIngressVlan();

  std::optional<SaiPortTraits::Attributes::InterfaceType> interfaceType{};
  if (auto saiInterfaceType =
          platform_->getInterfaceType(transmitterTech, speed)) {
    interfaceType = saiInterfaceType.value();
  }
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

} // namespace facebook::fboss
