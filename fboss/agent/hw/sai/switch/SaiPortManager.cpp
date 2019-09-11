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
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

SaiPortManager::SaiPortManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable), platform_(platform) {}

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
  handle->queues = managerTable_->queueManager().createQueues(
      saiPort->adapterKey(), swPort->getPortQueues());
  handles_.emplace(swPort->getID(), std::move(handle));
  portSaiIds_.emplace(saiPort->adapterKey(), swPort->getID());
  return saiPort->adapterKey();
}

void SaiPortManager::removePort(PortID swId) {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Attempted to remove non-existent port: ", swId);
  }
  portSaiIds_.erase(itr->second->port->adapterKey());
  handles_.erase(itr);
}

void SaiPortManager::changePort(const std::shared_ptr<Port>& swPort) {
  SaiPortHandle* existingPort = getPortHandle(swPort->getID());
  if (!existingPort) {
    throw FbossError("Attempted to change non-existent port ");
  }
  SaiPortTraits::CreateAttributes attributes = attributesFromSwPort(swPort);
  SaiPortTraits::AdapterHostKey portKey{GET_ATTR(Port, HwLaneList, attributes)};
  auto& portStore = SaiStore::getInstance()->get<SaiPortTraits>();
  portStore.setObject(portKey, attributes);
  existingPort->queues = managerTable_->queueManager().createQueues(
      existingPort->port->adapterKey(), swPort->getPortQueues());
}

SaiPortTraits::CreateAttributes SaiPortManager::attributesFromSwPort(
    const std::shared_ptr<Port>& swPort) const {
  bool adminState;

  switch (swPort->getAdminState()) {
    case cfg::PortState::DISABLED:
      adminState = false;
      break;
    case cfg::PortState::ENABLED:
      adminState = true;
      break;
    default:
      adminState = false;
      XLOG(INFO) << "Invalid port admin state!";
      break;
  }
  uint32_t speed;
  switch (swPort->getSpeed()) {
    case cfg::PortSpeed::TWENTYFIVEG:
      speed = static_cast<uint32_t>(swPort->getSpeed());
      break;
    case cfg::PortSpeed::HUNDREDG:
      speed = static_cast<uint32_t>(swPort->getSpeed());
      break;
    default:
      speed = 0;
      XLOG(INFO) << "Invalid port speed!";
  }

  auto platformPort = platform_->getPort(swPort->getID());
  auto hwLaneList = platformPort->getHwPortLanes(swPort->getSpeed());

  auto fecMode = SAI_PORT_FEC_MODE_NONE;
  if (!platformPort->shouldDisableFEC() &&
      swPort->getFEC() == cfg::PortFEC::ON) {
    auto fec = platformPort->getFecMode(swPort->getSpeed());
    if (fec == phy::FecMode::CL91 || fec == phy::FecMode::CL74) {
      fecMode = SAI_PORT_FEC_MODE_FC;
    } else if (fec == phy::FecMode::RS528 || fec == phy::FecMode::RS544) {
      fecMode = SAI_PORT_FEC_MODE_RS;
    }
  }

  auto pause = swPort->getPause();
  auto globalFlowControlMode = SAI_PORT_FLOW_CONTROL_MODE_DISABLE;
  if (pause.tx && pause.rx) {
    globalFlowControlMode = SAI_PORT_FLOW_CONTROL_MODE_BOTH_ENABLE;
  } else if (pause.tx) {
    globalFlowControlMode = SAI_PORT_FLOW_CONTROL_MODE_TX_ONLY;
  } else if (pause.rx) {
    globalFlowControlMode = SAI_PORT_FLOW_CONTROL_MODE_RX_ONLY;
  }

  auto internalLoopbackMode = SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE;
  switch (swPort->getLoopbackMode()) {
    case cfg::PortLoopbackMode::NONE:
      internalLoopbackMode = SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE;
      break;
    case cfg::PortLoopbackMode::PHY:
      internalLoopbackMode = SAI_PORT_INTERNAL_LOOPBACK_MODE_PHY;
      break;
    case cfg::PortLoopbackMode::MAC:
      internalLoopbackMode = SAI_PORT_INTERNAL_LOOPBACK_MODE_MAC;
      break;
  }

  auto mediaType = SAI_PORT_MEDIA_TYPE_UNKNOWN;
  switch (platformPort->getTransmitterTech()) {
    case TransmitterTechnology::COPPER:
      mediaType = SAI_PORT_MEDIA_TYPE_COPPER;
      break;
    case TransmitterTechnology::OPTICAL:
      mediaType = SAI_PORT_MEDIA_TYPE_FIBER;
      break;
    default:
      mediaType = SAI_PORT_MEDIA_TYPE_UNKNOWN;
  }
  uint16_t vlanId = swPort->getIngressVlan();
  return SaiPortTraits::CreateAttributes{hwLaneList,
                                         speed,
                                         adminState,
                                         fecMode,
                                         internalLoopbackMode,
                                         mediaType,
                                         globalFlowControlMode,
                                         vlanId};
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

PortID SaiPortManager::getPortID(sai_object_id_t saiId) const {
  auto itr = portSaiIds_.find(saiId);
  if (itr == portSaiIds_.end()) {
    return PortID(0);
  }
  return itr->second;
}
} // namespace fboss
} // namespace facebook
