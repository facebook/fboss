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
void SaiPortManager::addRemovedHandle(const PortID& portID) {
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
void SaiPortManager::removeRemovedHandleIf(const PortID& portID) {
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
           std::get<std::optional<std::decay_t<
               decltype(SaiPortSerdesTraits::Attributes::IDriver{})>>>(
               fromStore)));
}

std::shared_ptr<SaiPort> SaiPortManager::createPortWithBasicAttributes(
    const std::shared_ptr<Port>& swPort) {
  // Only below 7 basic port attributes from create_port() will always be
  // honored during flexport per CS00012328771. Other attributes might be reset
  // when the next new flexed port associated with the same PM core is created.
  // FBOSS need to re-program all non-basic attributes after flexport is done.
  // SAI_PORT_ATTR_TYPE
  // SAI_PORT_ATTR_HW_LANE_LIST
  // SAI_PORT_ATTR_SPEED
  // SAI_PORT_ATTR_ADMIN_STATE
  // SAI_PORT_ATTR_FEC_MODE
  // SAI_PORT_ATTR_LOOPBACK_MODE
  // SAI_PORT_ATTR_LINK_TRAINING_ENABLE
  auto portId = swPort->getID();
  XLOG(DBG2) << "add new port " << portId << " with basic attribtues only";
  SaiPortTraits::CreateAttributes attributes =
      attributesFromSwPort(swPort, false, true);
  SaiPortTraits::AdapterHostKey portKey{
#if defined(BRCM_SAI_SDK_DNX)
      GET_ATTR(Port, Type, attributes),
#endif
      GET_ATTR(Port, HwLaneList, attributes)};
  auto& portStore = saiStore_->get<SaiPortTraits>();
  return portStore.setObject(portKey, attributes, portId);
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
      std::map<PortID, std::shared_ptr<SaiPort>> portsWithBasicAttributes;
      for (auto portId : ports) {
        if (pendingNewPorts_.find(portId) != pendingNewPorts_.end()) {
          portsWithBasicAttributes[portId] =
              createPortWithBasicAttributes(pendingNewPorts_[portId]);
        }
      }
      for (auto portId : ports) {
        if (pendingNewPorts_.find(portId) != pendingNewPorts_.end()) {
          if (portsWithBasicAttributes.find(portId) ==
              portsWithBasicAttributes.end()) {
            throw FbossError(
                "sai port object with basic attribtues not created yet for port",
                portId);
          }
          XLOG(DBG2) << "update new port " << portId << " with all attributes";
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

void SaiPortManager::changePortFlowletConfig(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  if (!FLAGS_flowletSwitchingEnable ||
      !platform_->getAsic()->isSupported(HwAsic::Feature::ARS)) {
    return;
  }

  auto portHandle = getPortHandle(newPort->getID());
  if (!portHandle) {
    throw FbossError(
        "Cannot change flowlet cfg on non existent port: ", newPort->getID());
  }

  if (oldPort->getPortFlowletConfig() != newPort->getPortFlowletConfig()) {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    bool arsEnable = false;
    uint16_t scalingFactor = 0;
    uint16_t loadPastWeight = 0;
    uint16_t loadFutureWeight = 0;
    auto newPortFlowletCfg = newPort->getPortFlowletConfig();
    if (newPortFlowletCfg.has_value()) {
      /*
       * Sum of old and new weights cannot go beyond 100
       * This is not a problem with native impl since both weights are applied
       * with a single API call. An example transtion is
       * Load  : 60 -> 70
       * Queue : 40 -> 30
       * (70 + 40) > 100
       * Reset both the weights in the SDK once and re-apply new values below
       */
      portHandle->port->setOptionalAttribute(
          SaiPortTraits::Attributes::ArsPortLoadPastWeight{0});
      portHandle->port->setOptionalAttribute(
          SaiPortTraits::Attributes::ArsPortLoadFutureWeight{0});

      auto newPortFlowletCfgPtr = newPortFlowletCfg.value();
      arsEnable = true;
      scalingFactor = newPortFlowletCfgPtr->getScalingFactor();
      loadPastWeight = newPortFlowletCfgPtr->getLoadWeight();
      loadFutureWeight = newPortFlowletCfgPtr->getQueueWeight();
    }
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::ArsEnable{arsEnable});
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::ArsPortLoadScalingFactor{scalingFactor});
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::ArsPortLoadPastWeight{loadPastWeight});
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::ArsPortLoadFutureWeight{loadFutureWeight});
#endif
  } else {
    XLOG(DBG4) << "Port flowlet setting unchanged for " << newPort->getName();
  }
}

void SaiPortManager::clearPortFlowletConfig(const PortID& /* unused */) {}

} // namespace facebook::fboss
