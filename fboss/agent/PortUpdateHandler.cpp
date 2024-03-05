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

#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {
namespace {
struct BwInfo {
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

void PortUpdateHandler::stateUpdated(const StateDelta& delta) {
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
              portStats->setPortName(newPort->getName());
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
      },
      [&](const std::shared_ptr<Port>& newPort) {
        sw_->portStats(newPort->getID())->setPortStatus(newPort->isUp());
      },
      [&](const std::shared_ptr<Port>& oldPort) {
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
  for (const auto& [matcher, portMap] :
       std::as_const(*delta.newState()->getPorts())) {
    auto switchInfo = sw_->getSwitchInfoTable().getSwitchInfo(
        HwSwitchMatcher(matcher).switchId());
    if (switchInfo.switchType() != cfg::SwitchType::VOQ) {
      continue;
    }
    auto& bwInfo = swIndexToBwInfo[*switchInfo.switchIndex()];
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
  for (const auto& [swIndex, bwInfo] : swIndexToBwInfo) {
    XLOG(DBG2) << " Switch Index: " << swIndex
               << " Nif Mbps: " << bwInfo.nifBwMbps
               << " Fabric Mbps: " << bwInfo.fabricBwMbps;
  }
}
} // namespace facebook::fboss
