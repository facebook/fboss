/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/PortRemediator.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"

#include <set>

namespace {
constexpr int kPortRemedyIntervalSec = 25;
}

namespace facebook { namespace fboss {

void PortRemediator::updatePortState(cfg::PortState newPortState) {
  // TODO - Post t17304538, if Port::state == POWER_DOWN reflects
  // Admin down always, then just use SwitchState to to determine
  // which ports to ignore.
  std::set<int> configEnabledPorts;
  for (const auto& cfgPort : sw_->getConfig().ports) {
    if (cfgPort.state != cfg::PortState::POWER_DOWN &&
        cfgPort.state != cfg::PortState::DOWN) {
      configEnabledPorts.insert(cfgPort.logicalID);
    }
  }
  const auto ports = sw_->getState()->getPorts();
  // Does port need to be flapped
  auto portNeedsUpdate = [&configEnabledPorts](
      const std::shared_ptr<Port>& port) {
    if (port &&
        configEnabledPorts.find(port->getID()) != configEnabledPorts.end()) {
      return !port->getOperState();
    }
    return false;
  };
  auto updateFn = [=](const std::shared_ptr<SwitchState>& state) {
    std::shared_ptr<SwitchState> newState{state};

    for (int i = 1; i < ports->numPorts(); i++) {
      const auto port = ports->getPortIf(PortID(i));
      if (portNeedsUpdate(port)) {
        const auto newPort = port->modify(&newState);
        newPort->setState(newPortState);
      }
    }
    return newState;
  };
  for (int i = 1; i < ports->numPorts(); ++i) {
    if (portNeedsUpdate(ports->getPortIf(PortID(i)))) {
      sw_->updateStateBlocking("PortRemediator: flap port", updateFn);
      return;
    }
  }
}

void PortRemediator::timeoutExpired() noexcept {
  updatePortState(cfg::PortState::DOWN);
  updatePortState(cfg::PortState::UP);
  scheduleTimeout(interval_);
}

PortRemediator::PortRemediator(SwSwitch* swSwitch)
    : AsyncTimeout(swSwitch->getBackgroundEVB()),
      sw_(swSwitch),
      interval_(kPortRemedyIntervalSec) {
  // Schedule the port remedy handler to run
  bool ret = sw_->getBackgroundEVB()->runInEventBaseThread(
      PortRemediator::start, (void*)this);
  if (!ret) {
    // Throw error from the constructor because this object is unusable if we
    // cannot start it in event base.
    throw FbossError("Failed to start PortRemediator");
  }
}

PortRemediator::~PortRemediator() {
  int stopPortRemediator =
      sw_->getBackgroundEVB()->runImmediatelyOrRunInEventBaseThreadAndWait(
          PortRemediator::stop, (void*)this);
  // At this point, PortRemediator must not be running.
  if (!stopPortRemediator) {
    throw FbossError("Failed to stop port remediation handler");
  }
}

}} // facebook::fboss
