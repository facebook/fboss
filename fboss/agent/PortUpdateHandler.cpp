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
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

PortUpdateHandler::PortUpdateHandler(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "PortUpdateHandler"), sw_(sw) {}

void PortUpdateHandler::stateUpdated(const StateDelta& delta) {
  // For now, the stateUpdated is only used to update the portName of PortStats
  // for all threads.
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
}

} // namespace facebook::fboss
