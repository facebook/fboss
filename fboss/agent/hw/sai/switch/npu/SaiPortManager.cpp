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
  handle->serdes = programSerdes(saiPort, swPort);

  handle->bridgePort = managerTable_->bridgeManager().addBridgePort(
      SaiPortDescriptor(swPort->getID()),
      PortDescriptorSaiId(saiPort->adapterKey()));
  loadPortQueues(handle.get());
  const auto asic = platform_->getAsic();
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
  }
  managerTable_->queueManager().ensurePortQueueConfig(
      saiPort->adapterKey(), handle->queues, swPort->getPortQueues());

  bool samplingMirror = swPort->getSampleDestination().has_value() &&
      swPort->getSampleDestination() == cfg::SampleDestination::MIRROR;
  SaiPortMirrorInfo mirrorInfo{
      swPort->getIngressMirror(), swPort->getEgressMirror(), samplingMirror};
  handle->mirrorInfo = mirrorInfo;
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

} // namespace facebook::fboss
