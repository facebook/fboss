// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

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
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  if (!FLAGS_flowletSwitchingEnable) {
    return;
  }

  auto portHandle = getPortHandle(newPort->getID());
  if (!portHandle) {
    throw FbossError(
        "Cannot change flowlet cfg on non existent port: ", newPort->getID());
  }

  if (oldPort->getPortFlowletConfig() != newPort->getPortFlowletConfig()) {
    bool arsEnable = false;
    auto newPortFlowletCfg = newPort->getPortFlowletConfig();
    if (newPortFlowletCfg.has_value()) {
      auto newPortFlowletCfgPtr = newPortFlowletCfg.value();
      arsEnable = true;
    }
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::ArsEnable{arsEnable});
  } else {
    XLOG(DBG4) << "Port flowlet setting unchanged for " << newPort->getName();
  }
}

void SaiPortManager::clearPortFlowletConfig(const PortID& portID) {
  if (!FLAGS_flowletSwitchingEnable) {
    return;
  }

  auto portHandle = getPortHandle(portID);
  if (!portHandle) {
    throw FbossError(
        "Cannot change flowlet cfg on non existent port: ", portID);
  }

  bool arsEnable = false;
  portHandle->port->setOptionalAttribute(
      SaiPortTraits::Attributes::ArsEnable{arsEnable});
}

void SaiPortManager::programPfcDurationCounterEnable(
    const std::shared_ptr<Port>& /* swPort */,
    const std::optional<cfg::PortPfc>& /* newPfc */,
    const std::optional<cfg::PortPfc>& /* oldPfc */) {}

const std::vector<sai_stat_id_t>& SaiPortManager::getSupportedPfcDurationStats(
    const PortID& /* portId */) {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

} // namespace facebook::fboss
