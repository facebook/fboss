// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {
class SaiPlatform;

void SaiPortManager::addRemovedHandle(PortID /*portID*/) {}

void SaiPortManager::removeRemovedHandleIf(PortID /*portID*/) {}

bool SaiPortManager::checkPortSerdesAttributes(
    const SaiPortSerdesTraits::CreateAttributes& fromStore,
    const SaiPortSerdesTraits::CreateAttributes& fromSwPort) {
  auto checkSerdesAttribute =
      [](auto type, auto& attrs1, auto& attrs2) -> bool {
    return (
        (std::get<std::optional<std::decay_t<decltype(type)>>>(attrs1)) ==
        (std::get<std::optional<std::decay_t<decltype(type)>>>(attrs2)));
  };
  return (
      (std::get<SaiPortSerdesTraits::Attributes::PortId>(fromStore) ==
       std::get<SaiPortSerdesTraits::Attributes::PortId>(fromSwPort)) &&
      (checkSerdesAttribute(
          SaiPortSerdesTraits::Attributes::TxFirPre1{},
          fromSwPort,
          fromStore)) &&
      (checkSerdesAttribute(
          SaiPortSerdesTraits::Attributes::TxFirMain{},
          fromSwPort,
          fromStore)) &&
      (checkSerdesAttribute(
          SaiPortSerdesTraits::Attributes::TxFirPost1{},
          fromSwPort,
          fromStore)) &&
      (checkSerdesAttribute(
          SaiPortSerdesTraits::Attributes::RxCtleCode{},
          fromSwPort,
          fromStore)) &&
      (checkSerdesAttribute(
          SaiPortSerdesTraits::Attributes::RxDspMode{},
          fromSwPort,
          fromStore)) &&
      (checkSerdesAttribute(
          SaiPortSerdesTraits::Attributes::RxAfeTrim{},
          fromSwPort,
          fromStore)) &&
      (checkSerdesAttribute(
          SaiPortSerdesTraits::Attributes::RxAcCouplingByPass{},
          fromSwPort,
          fromStore)));
}

void SaiPortManager::changePortByRecreate(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  // If YUBA, disable port before recreating
  if (platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) {
    SaiPortTraits::Attributes::AdminState adminDisable{false};
    SaiPortHandle* portHandle = getPortHandle(oldPort->getID());
    SaiApiTable::getInstance()->portApi().setAttribute(
        portHandle->port->adapterKey(), adminDisable);
  }

  // If YUBA and new or old port has 100G, we will delete/remove both/all ports
  // in the group and add/create back both/all ports in the group.
  if ((platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) &&
      (newPort->getProfileID() ==
           cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL ||
       oldPort->getProfileID() ==
           cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL)) {
    removePort(oldPort);
    pendingNewPorts_[newPort->getID()] = newPort;
    bool allPortsInGroupRemoved = true;
    auto& platformPortEntry =
        platform_->getPort(oldPort->getID())->getPlatformPortEntry();
    auto controllingPort =
        PortID(*platformPortEntry.mapping()->controllingPort());
    auto ports = platform_->getAllPortsInGroup(controllingPort);
    XLOG(DBG2) << "Port " << oldPort->getID() << "'s controlling port is "
               << controllingPort;
    for (auto portId : ports) {
      if (handles_.find(portId) != handles_.end()) {
        XLOG(DBG2) << "Port " << portId
                   << " in the same group but not removed yet";
        allPortsInGroupRemoved = false;
      }
    }
    if (allPortsInGroupRemoved) {
      for (auto portId : ports) {
        removeRemovedHandleIf(portId);
      }
      XLOG(DBG2)
          << "All old ports in the same group removed, add new ports back";

      for (auto portId : ports) {
        if (pendingNewPorts_.find(portId) != pendingNewPorts_.end()) {
          addPort(pendingNewPorts_[portId]);
          pendingNewPorts_.erase(portId);
        }
      }
    }
  } else {
    removePort(oldPort);
    addPort(newPort);
  }
}

} // namespace facebook::fboss
