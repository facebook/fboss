/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/PortUpdateHandler.h"
#include "fboss/agent/LldpManager.h"

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchInfoTable.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchSettings.h"

namespace facebook::fboss {
namespace {
struct BwInfo {
  cfg::AsicType asicType;
  uint32_t fabricBwMbps{0};
  uint32_t nifBwMbps{0};
};
} // namespace

PortUpdateHandler::PortUpdateHandler(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "PortUpdateHandler");
}
PortUpdateHandler::~PortUpdateHandler() {
  sw_->unregisterStateObserver(this);
}

void PortUpdateHandler::disableIfLooped(
    const std::shared_ptr<Port>& newPort,
    const std::shared_ptr<SwitchState>& newState) {
  if (!FLAGS_disable_looped_fabric_ports) {
    XLOG(DBG2) << " Port loop detection disabled";
    return;
  }
  if (newPort->getPortType() != cfg::PortType::FABRIC_PORT) {
    // Not a fabric port, nothing to do
    return;
  }
  if (!newPort->getLedPortExternalState() ||
      newPort->getLedPortExternalState() !=
          PortLedExternalState::CABLING_ERROR_LOOP_DETECTED) {
    // No loop detected, nothing to do
    return;
  }

  if (newPort->isDrained()) {
    // Port is drained nothing to do
    return;
  }

  auto portScope = sw_->getScopeResolver()->scope(newPort);
  if (newState->getSwitchSettings()
          ->getSwitchSettings(portScope)
          ->isSwitchDrained()) {
    // Switch is drained, nothing to do
    return;
  }
  if (sw_->getSwitchInfoTable()
          .getSwitchInfo(portScope.switchId())
          .switchType() != cfg::SwitchType::FABRIC) {
    // Not a fabric switch, nothing to do
    return;
  }
  if (newPort->getAdminState() == cfg::PortState::DISABLED) {
    // Port already disabled, nothing to do
    return;
  }
  XLOG(DBG2) << " Loop detected on : " << newPort->getName()
             << ", disabling port ";

  auto portId = newPort->getID();
  sw_->updateState(
      folly::sformat("Disable undrained, looped port: {}", newPort->getName()),
      [portId](const std::shared_ptr<SwitchState>& in) {
        auto out = in->clone();
        auto curPort = out->getPorts()->getNode(portId);
        auto port = curPort->modify(&out);
        port->setAdminState(cfg::PortState::DISABLED);
        port->addError(PortError::ERROR_DISABLE_LOOP_DETECTED);
        return out;
      });
}

void PortUpdateHandler::checkNewlyUndrained(const StateDelta& delta) {
  bool newlyUndrained{false};
  DeltaFunctions::forEachChanged(
      delta.getSwitchSettingsDelta(),
      [&](const std::shared_ptr<SwitchSettings>& oldSwitchSettings,
          const std::shared_ptr<SwitchSettings>& newSwitchSettings) {
        newlyUndrained |=
            (!newSwitchSettings->isSwitchDrained() &&
             newSwitchSettings->isSwitchDrained() !=
                 oldSwitchSettings->isSwitchDrained());
      },
      [&](const std::shared_ptr<SwitchSettings>& newSwitchSettings) {
        newlyUndrained |= !newSwitchSettings->isSwitchDrained();
      },
      [](const std::shared_ptr<SwitchSettings>& oldSwitchSettings) {});
  if (newlyUndrained) {
    auto state = delta.newState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& [_, port] : std::as_const(*portMap.second)) {
        disableIfLooped(port, state);
      }
    }
  }
}
void PortUpdateHandler::clearErrorDisableLoopDetected(
    const std::shared_ptr<Port>& newPort,
    const std::shared_ptr<SwitchState>& newState) {
  const auto& errors = newPort->getActiveErrors();
  if (std::find(
          errors.begin(),
          errors.end(),
          PortError::ERROR_DISABLE_LOOP_DETECTED) == errors.end()) {
    return;
  }
  XLOG(DBG2) << " Clearing error disable loop detected on : "
             << newPort->getName();

  auto portId = newPort->getID();
  sw_->updateState(
      folly::sformat(
          "Clear error disable loop detected on: {}", newPort->getName()),
      [portId](const std::shared_ptr<SwitchState>& in) {
        auto out = in->clone();
        auto curPort = out->getPorts()->getNode(portId);
        if (!curPort->isEnabled()) {
          return std::shared_ptr<SwitchState>();
        }
        auto port = curPort->modify(&out);
        port->removeError(PortError::ERROR_DISABLE_LOOP_DETECTED);
        return out;
      });
}

void PortUpdateHandler::stateUpdated(const StateDelta& delta) {
  checkNewlyUndrained(delta);
  DeltaFunctions::forEachChanged(
      delta.getPortsDelta(),
      [&](const std::shared_ptr<Port>& oldPort,
          const std::shared_ptr<Port>& newPort) {
        if ((oldPort->isUp() && !newPort->isUp()) ||
            (oldPort->isEnabled() && !newPort->isEnabled())) {
          if (sw_->getLldpMgr()) {
            sw_->getLldpMgr()->portDown(newPort->getID());
          }
        }
        if (oldPort->getName() != newPort->getName()) {
          for (SwitchStats& switchStats : sw_->getAllThreadsSwitchStats()) {
            // only update the portName when the portStats exists
            PortStats* portStats = switchStats.port(newPort->getID());
            if (portStats) {
              portStats = switchStats.updatePortName(
                  newPort->getID(), newPort->getName());
              portStats->setPortStatus(newPort->isUp());
            }
          }
        }
        if (oldPort->isUp() != newPort->isUp()) {
          for (SwitchStats& switchStats : sw_->getAllThreadsSwitchStats()) {
            PortStats* portStats = switchStats.port(newPort->getID());
            if (portStats) {
              portStats->setPortStatus(newPort->isUp());
            }
          }
          sw_->publishPhyInfoSnapshots(oldPort->getID());
        }
        disableIfLooped(newPort, delta.newState());
        // Clear error
        if (newPort->isEnabled() && !oldPort->isEnabled()) {
          clearErrorDisableLoopDetected(newPort, delta.newState());
        }
      },
      [&](const std::shared_ptr<Port>& newPort) {
        sw_->portStats(newPort->getID())->setPortStatus(newPort->isUp());
        disableIfLooped(newPort, delta.newState());
        if (newPort->isEnabled()) {
          // For WB case where all ports will show up as newly added
          clearErrorDisableLoopDetected(newPort, delta.newState());
        }
      },
      [&](const std::shared_ptr<Port>& oldPort) {
        sw_->portStats(oldPort->getID())->clearPortStatusCounter();
        sw_->portStats(oldPort->getID())->clearPortActiveStatusCounter();
        for (SwitchStats& switchStats : sw_->getAllThreadsSwitchStats()) {
          switchStats.deletePortStats(oldPort->getID());
        }
        if (sw_->getLldpMgr()) {
          sw_->getLldpMgr()->portDown(oldPort->getID());
        }
      });
  if (delta.getPortsDelta().begin() == delta.getPortsDelta().end()) {
    return;
  }
  computeFabricOverdrainPct(delta);
}

void PortUpdateHandler::computeFabricOverdrainPct(const StateDelta& delta) {
  std::map<int, BwInfo> swIndexToBwInfo;
  std::map<int, cfg::SwitchInfo> switchIndexToSwitchInfo;
  for (const auto& [matcher, portMap] :
       std::as_const(*delta.newState()->getPorts())) {
    auto switchInfo = sw_->getSwitchInfoTable().getSwitchInfo(
        HwSwitchMatcher(matcher).switchId());
    if (switchInfo.switchType() != cfg::SwitchType::VOQ) {
      continue;
    }
    auto& bwInfo = swIndexToBwInfo[*switchInfo.switchIndex()];
    bwInfo.asicType = *switchInfo.asicType();
    for (const auto& [_, port] : std::as_const(*portMap)) {
      if (!port->isUp()) {
        continue;
      }
      if (port->getPortType() == cfg::PortType::INTERFACE_PORT ||
          port->getPortType() == cfg::PortType::MANAGEMENT_PORT) {
        bwInfo.nifBwMbps += static_cast<uint32_t>(port->getSpeed());
      } else if (port->getPortType() == cfg::PortType::FABRIC_PORT) {
        if (port->isActive().value_or(false)) {
          switch (port->getSpeed()) {
            case cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG:
              bwInfo.fabricBwMbps +=
                  static_cast<uint32_t>(cfg::PortSpeed::FIFTYG);
              break;
            case cfg::PortSpeed::HUNDREDANDSIXPOINTTWOFIVEG:
              bwInfo.fabricBwMbps +=
                  static_cast<uint32_t>(cfg::PortSpeed::HUNDREDG);
              break;
            default:
              bwInfo.fabricBwMbps += static_cast<uint32_t>(port->getSpeed());
              break;
          }
        }
      }
    }
  }
  auto getFabricOverheadMultiplier = [](cfg::AsicType asicType) {
    switch (asicType) {
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
        return 1.12;
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
        return 1.06;
      case cfg::AsicType::ASIC_TYPE_MOCK:
      case cfg::AsicType::ASIC_TYPE_FAKE:
        // Mimicing J3 overhead
        return 1.10;
      default:
        throw FbossError(
            "Unhandled asic type: ",
            apache::thrift::util::enumNameSafe(asicType));
    }
  };
  for (const auto& [swIndex, bwInfo] : swIndexToBwInfo) {
    double fabricOverdrainPct = 0;
    double fabricOverheadMultiplier =
        getFabricOverheadMultiplier(bwInfo.asicType);
    if (bwInfo.nifBwMbps && bwInfo.fabricBwMbps) {
      fabricOverdrainPct =
          ((bwInfo.nifBwMbps * fabricOverheadMultiplier - bwInfo.fabricBwMbps) *
           100.0) /
          bwInfo.nifBwMbps;
    } else if (bwInfo.nifBwMbps) {
      fabricOverdrainPct = 100.0;
    } else {
      // NIF BW == 0, no question of being overdrained
    }
    XLOG(DBG2) << " Switch Index: " << swIndex
               << " Nif Mbps: " << bwInfo.nifBwMbps
               << " Fabric Mbps: " << bwInfo.fabricBwMbps
               << " Fabric overdrain pct: " << fabricOverdrainPct;
    int fabricOverdrainNormalized =
        std::max(0, static_cast<int>(std::ceil(fabricOverdrainPct)));
    XLOG(DBG2) << " Fabric overdrain pct reported: "
               << fabricOverdrainNormalized;
    sw_->stats()->setFabricOverdrainPct(swIndex, fabricOverdrainNormalized);
  }
}
} // namespace facebook::fboss
