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

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwitchStats.h"

namespace facebook { namespace fboss {

PortUpdateHandler::PortUpdateHandler(SwSwitch* sw)
  : AutoRegisterStateObserver(sw, "PortUpdateHandler"),
    sw_(sw) {
}

void PortUpdateHandler::stateUpdated(const StateDelta& delta) {
  // For now, the stateUpdated here is used to update those counters which
  // uses port name as their key.
  DeltaFunctions::forEachChanged(
      delta.getPortsDelta(),
      [&](const std::shared_ptr<Port>& oldPort,
          const std::shared_ptr<Port>& newPort) {
        if (oldPort->getName() == newPort->getName()) {
          return;
        }
        // clear old portStats counter
        PortStats* portStats = sw_->stats()->port(oldPort->getID());
        portStats->clearPortStatusCounter();
        // and then update new portStatus
        sw_->upatePortStats(newPort->getID(), newPort->getName());
        portStats = sw_->stats()->port(newPort->getID());
        portStats->setPortStatusCounter(newPort->isUp());
      },
      [&](const std::shared_ptr<Port>& newPort) {
        sw_->upatePortStats(newPort->getID(), newPort->getName());
        PortStats* portStats = sw_->stats()->port(newPort->getID());
        portStats->setPortStatusCounter(newPort->isUp());
      },
      [&](const std::shared_ptr<Port>& oldPort) {
        PortStats* portStats = sw_->stats()->port(oldPort->getID());
        portStats->clearPortStatusCounter();
        sw_->deletePortStats(oldPort->getID());
      });
}

}} // facebook::fboss
