// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

namespace facebook::fboss {

void SaiPortManager::addRemovedHandle(const PortID& /*portID*/) {}

void SaiPortManager::removeRemovedHandleIf(const PortID& /*portID*/) {}

bool SaiPortManager::checkPortSerdesAttributes(
    const SaiPortSerdesTraits::CreateAttributes& fromStore,
    const SaiPortSerdesTraits::CreateAttributes& fromSwPort) {
  return fromSwPort == fromStore;
}

void SaiPortManager::changePortByRecreate(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  removePort(oldPort);
  addPort(newPort);
}

void SaiPortManager::changePortFlowletConfig(
    const std::shared_ptr<Port>& /* unused */,
    const std::shared_ptr<Port>& /* unused */) {}

} // namespace facebook::fboss
