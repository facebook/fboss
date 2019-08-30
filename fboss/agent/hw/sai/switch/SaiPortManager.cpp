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

bool SaiPortManager::isValidPortConfig(const std::shared_ptr<Port>& swPort) {
  auto platformPort = platform_->getPort(swPort->getID());
  // TODO: Revisit this. Add a test with different speed and the
  // subsumed port list is not right in agentconfigfactory.
  /*
   * getSubsumedPorts returns a list of PortID from the platform by
   * reading the agent config given the port speed. This would return
   * false if a port has been already created from the subsumed port list.
   */
  auto subsumedPorts = platformPort->getSubsumedPorts(swPort->getSpeed());
  for (auto portId : subsumedPorts) {
    if (ports_.find(portId) != ports_.end()) {
      return false;
    }
  }
  return true;
}

sai_object_id_t SaiPortManager::getSaiPortImpl(
    const std::vector<uint32_t>& hwLanes,
    cfg::PortSpeed speed) {
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto& switchApi = apiTable_->switchApi();
  auto numPorts = switchApi.getAttribute(
      SwitchApiParameters::Attributes::PortNumber(), switchId);
  if (numPorts == 0) {
    return SAI_NULL_OBJECT_ID;
  }
  std::vector<sai_object_id_t> portList;
  portList.resize(numPorts);
  auto portIds = switchApi.getAttribute(
      SwitchApiParameters::Attributes::PortList(portList), switchId);
  // TODO: finding a portId everytime a port is added is
  // expensive. ports from config * ports configured. Other approach is to
  // map these object ids to portIDs using the platform config during init.
  for (auto portId : portIds) {
    auto attributes = attributesFromSaiPort(portId);
    // TODO: should we consider admin state ? The attribute list
    // constructed here vs the attributes used by the addPort will differ
    // if the admin state differs.
    if (attributes.hwLaneList == hwLanes and
        static_cast<cfg::PortSpeed>(attributes.speed) == speed) {
      XLOG(DBG2) << "Consolidated sai port id: " << portId;
      return portId;
    }
  }
  return SAI_NULL_OBJECT_ID;
}

sai_object_id_t SaiPortManager::getSaiPortIf(
    const std::vector<uint32_t>& hwLanes,
    cfg::PortSpeed speed) {
  return getSaiPortImpl(hwLanes, speed);
}

sai_object_id_t SaiPortManager::getSaiPort(
    const std::vector<uint32_t>& hwLanes,
    cfg::PortSpeed speed) {
  auto saiPortId = getSaiPortImpl(hwLanes, speed);
  if (saiPortId == SAI_NULL_OBJECT_ID) {
    throw FbossError("failed to find sai port");
  }
  return saiPortId;
}

sai_object_id_t SaiPortManager::addPort(const std::shared_ptr<Port>& swPort) {
  SaiPort* existingPort = getPort(swPort->getID());
  if (existingPort) {
    throw FbossError(
        "Attempted to add port which already exists: ",
        swPort->getID(),
        " SAI id: ",
        existingPort->id());
  }
  if (!swPort->isEnabled()) {
    return SAI_NULL_OBJECT_ID;
  }
  if (!isValidPortConfig(swPort)) {
    throw FbossError("Invalid port configuration ", swPort->getID());
  }
  PortApiParameters::Attributes attributes = attributesFromSwPort(swPort);
  auto saiPortId = getSaiPortIf(attributes.hwLaneList, swPort->getSpeed());
  std::unique_ptr<SaiPort> saiPort;
  if (saiPortId != SAI_NULL_OBJECT_ID) {
    saiPort = std::make_unique<SaiPort>(
        apiTable_, managerTable_, attributes, saiPortId);
  } else {
    saiPort = std::make_unique<SaiPort>(apiTable_, managerTable_, attributes);
  }
  sai_object_id_t saiId = saiPort->id();
  ports_.emplace(std::make_pair(swPort->getID(), std::move(saiPort)));
  portSaiIds_.emplace(std::make_pair(saiId, swPort->getID()));
  SaiPort* newPort = getPort(swPort->getID());
  newPort->createQueue(swPort->getPortQueues());
  return saiId;
}

void SaiPortManager::removePort(PortID swId) {
  auto itr = ports_.find(swId);
  if (itr == ports_.end()) {
    throw FbossError("Attempted to remove non-existent port: ", swId);
  }
  portSaiIds_.erase(itr->second->id());
  ports_.erase(itr);
}

void SaiPortManager::changePort(const std::shared_ptr<Port>& swPort) {
  SaiPort* existingPort = getPort(swPort->getID());
  if (!existingPort) {
    throw FbossError("Attempted to change non-existent port ");
  }
  PortApiParameters::Attributes desiredAttributes =
      attributesFromSwPort(swPort);
  if (existingPort->attributes() == desiredAttributes) {
    return;
  }
  existingPort->setAttributes(desiredAttributes);
  existingPort->updateQueue(swPort->getPortQueues());
}

PortApiParameters::Attributes SaiPortManager::attributesFromSwPort(
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
  return PortApiParameters::Attributes({hwLaneList, speed, adminState});
}

PortApiParameters::Attributes SaiPortManager::attributesFromSaiPort(
    const sai_object_id_t saiPortId) const {
  auto& portApi = apiTable_->portApi();
  auto speed =
      portApi.getAttribute(PortApiParameters::Attributes::Speed(), saiPortId);
  std::vector<uint32_t> ls;
  ls.resize(8);
  auto hwLaneList = portApi.getAttribute(
      PortApiParameters::Attributes::HwLaneList(ls), saiPortId);
  auto adminState = portApi.getAttribute(
      PortApiParameters::Attributes::AdminState(), saiPortId);
  return PortApiParameters::Attributes({hwLaneList, speed, adminState});
}

// private const getter for use by const and non-const getters
SaiPort* SaiPortManager::getPortImpl(PortID swId) const {
  auto itr = ports_.find(swId);
  if (itr == ports_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiPort for " << swId;
  }
  return itr->second.get();
}

const SaiPort* SaiPortManager::getPort(PortID swId) const {
  return getPortImpl(swId);
}

SaiPort* SaiPortManager::getPort(PortID swId) {
  return getPortImpl(swId);
}

PortID SaiPortManager::getPortID(sai_object_id_t saiId) const {
  auto itr = portSaiIds_.find(saiId);
  if (itr == portSaiIds_.end()) {
    return PortID(0);
  }
  return itr->second;
}

SaiPort::SaiPort(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const PortApiParameters::Attributes& attributes)
    : apiTable_(apiTable),
      managerTable_(managerTable),
      attributes_(attributes) {
  auto& portApi = apiTable_->portApi();
  auto switchId = managerTable->switchManager().getSwitchSaiId();
  id_ = portApi.create(attributes_.attrs(), switchId);
  bridgePort_ = managerTable_->bridgeManager().addBridgePort(id_);
}

SaiPort::SaiPort(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const PortApiParameters::Attributes& attributes,
    sai_object_id_t id)
    : apiTable_(apiTable),
      managerTable_(managerTable),
      attributes_(attributes),
      id_(id) {
  bridgePort_ = managerTable_->bridgeManager().addBridgePort(id_);
}

void SaiPort::setAttributes(
    const PortApiParameters::Attributes& desiredAttributes) {
  auto& portApi = apiTable_->portApi();
  if (attributes_.adminState != desiredAttributes.adminState) {
    if (desiredAttributes.adminState) {
      portApi.setAttribute(
          PortApiParameters::Attributes::AdminState{
              desiredAttributes.adminState.value()},
          id_);
    }
  }
  if (attributes_.speed != desiredAttributes.speed) {
    portApi.setAttribute(
        PortApiParameters::Attributes::Speed{desiredAttributes.speed}, id_);
  }
  if (attributes_.hwLaneList != desiredAttributes.hwLaneList) {
    portApi.setAttribute(
        PortApiParameters::Attributes::HwLaneList{desiredAttributes.hwLaneList},
        id_);
  }
}

SaiPort::~SaiPort() {
  bridgePort_.reset();
  apiTable_->portApi().remove(id_);
}

bool SaiPort::operator==(const SaiPort& other) const {
  return attributes_ == other.attributes();
}
bool SaiPort::operator!=(const SaiPort& other) const {
  return !(*this == other);
}

void SaiPort::setPortVlan(VlanID vlanId) {
  vlanId_ = vlanId;
}

VlanID SaiPort::getPortVlan() const {
  return vlanId_;
}

void SaiPort::createQueue(const QueueConfig& portQueues) {
  PortApiParameters::Attributes::QosNumberOfQueues numQueuesAttr;
  auto numQueues = apiTable_->portApi().getAttribute(numQueuesAttr, id());
  for (const auto& portQueue : portQueues) {
    auto queueIter = queues_.find(portQueue->getID());
    if (queueIter != queues_.end()) {
      throw FbossError("failed to find queue id ", portQueue->getID());
    }
    auto queue =
        managerTable_->queueManager().createQueue(id(), numQueues, *portQueue);
    queues_.emplace(std::make_pair(portQueue->getID(), std::move(queue)));
  }
}

void SaiPort::updateQueue(const QueueConfig& portQueues) const {
  for (const auto& portQueue : portQueues) {
    auto queueIter = queues_.find(portQueue->getID());
    if (queueIter == queues_.end()) {
      // TOOD(srikrishnagopu): throw here instead ?
      continue;
    }
    SaiQueue* queue = queueIter->second.get();
    queue->updatePortQueue(*portQueue);
  }
}
} // namespace fboss
} // namespace facebook
