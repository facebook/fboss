// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

// Hack as this SAI implementation does support port removal. Port can only be
// removed iff they're followed by add operation. So put removed ports in
// removedHandles_ so port does not get destroyed by invoking SAI's remove port
// API. However their dependent objects such as bridge ports, fdb entries  etc.
// must act as if the port has been removed. So notify those objects to mimic
// port removal.
void SaiPortManager::addRemovedHandle(PortID portID) {
  auto itr = handles_.find(portID);
  CHECK(itr != handles_.end());
  itr->second->queues.clear();
  itr->second->configuredQueues.clear();
  itr->second->bridgePort.reset();
  itr->second->serdes.reset();
  // instead of removing port, retain it
  removedHandles_.emplace(itr->first, std::move(itr->second));
}

// Before adding port, remove the port from removedHandles_. This also deletes
// the port and invokes SAI's remove port API. This is acceptable, because port
// will be added again with new lanes later.
void SaiPortManager::removeRemovedHandleIf(PortID portID) {
  removedHandles_.erase(portID);
}

// On Sai TH3 we don't program IDriver and Preemphasis, hence the default values
// are zero in SaiStore. However, across warmboot Brcm-sai would give us some
// other default values, causing fboss to reprogram the serdes and thus causing
// link flaps. Since the SDK is computing different values based on ASIC, port
// speed and port type, we don't have a default value fixed per ASIC. This is a
// temporary workaround to skip preemphasis and idriver if we're not using them,
// but need to check what are the default values of them.
bool SaiPortManager::checkPortSerdesAttributes(
    const SaiPortSerdesTraits::CreateAttributes& fromStore,
    const SaiPortSerdesTraits::CreateAttributes& fromSwPort) {
  auto checkSerdesAttribute =
      [](auto type, auto& attrs1, auto& attrs2) -> bool {
    return (
        (std::get<std::optional<std::decay_t<decltype(type)>>>(attrs1)) ==
        (std::get<std::optional<std::decay_t<decltype(type)>>>(attrs2)));
  };
  auto iDriver = std::get<std::optional<
      std::decay_t<decltype(SaiPortSerdesTraits::Attributes::IDriver{})>>>(
      fromSwPort);
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
      (!iDriver.has_value() ||
       iDriver ==
           std::get<std::optional<std::decay_t<decltype(
               SaiPortSerdesTraits::Attributes::IDriver{})>>>(fromStore)));
}

void SaiPortManager::changePortByRecreate(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  if (platform_->getAsic()->isSupported(HwAsic::Feature::SAI_PORT_VCO_CHANGE) &&
      static_cast<SaiBcmPlatform*>(platform_)->needPortVcoChange(
          oldPort->getSpeed(),
          *(oldPort->getProfileConfig().fec()),
          newPort->getSpeed(),
          *(newPort->getProfileConfig().fec()))) {
    // To change to a different VCO, we need to remove all the ports in the
    // Port Macro and re-create the removed ports with the new speed.
    auto portHandle = getPortHandle(newPort->getID());
    // disable port
    SaiPortTraits::Attributes::AdminState adminDisable{false};
    SaiApiTable::getInstance()->portApi().setAttribute(
        portHandle->port->adapterKey(), adminDisable);
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
          XLOG(DBG2) << "add new port " << portId;
          addPort(pendingNewPorts_[portId]);
          pendingNewPorts_.erase(portId);
          // port should already be enabled
        }
      }
    }
  } else {
    removePort(oldPort);
    addPort(newPort);
  }
}

} // namespace facebook::fboss
