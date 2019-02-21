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

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

SaiPortManager::SaiPortManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable)
    : apiTable_(apiTable), managerTable_(managerTable) {}

sai_object_id_t SaiPortManager::addPort(const std::shared_ptr<Port>& swPort) {
  SaiPort* existingPort = getPort(swPort->getID());
  if (existingPort) {
    throw FbossError("Attempted to add port which already exists");
  }
  PortApiParameters::Attributes attributes = attributesFromSwPort(swPort);
  auto saiPort =
      std::make_unique<SaiPort>(apiTable_, managerTable_, attributes);
  sai_object_id_t saiId = saiPort->id();
  ports_.emplace(std::make_pair(swPort->getID(), std::move(saiPort)));
  return saiId;
}

void SaiPortManager::removePort(PortID swId) {
  auto itr = ports_.find(swId);
  if (itr == ports_.end()) {
    throw FbossError("Attempted to remove non-existent port: ", swId);
  }
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
}

PortApiParameters::Attributes SaiPortManager::attributesFromSwPort(
    const std::shared_ptr<Port>& swPort) const {
  bool adminState;
  std::vector<uint32_t> hwLaneList;
  uint32_t speed;
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
  switch (swPort->getSpeed()) {
    // TODO: actual lane mapping! :)
    case cfg::PortSpeed::TWENTYFIVEG:
      speed = static_cast<uint32_t>(swPort->getSpeed());
      hwLaneList.push_back(swPort->getID());
      break;
    default:
      speed = 0;
      XLOG(INFO) << "Invalid port speed!";
  }
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

SaiPort::SaiPort(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const PortApiParameters::Attributes& attributes)
    : apiTable_(apiTable),
      managerTable_(managerTable),
      attributes_(attributes) {
  auto& portApi = apiTable_->portApi();
  id_ = portApi.create2(attributes_.attrs(), 0);
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

} // namespace fboss
} // namespace facebook
