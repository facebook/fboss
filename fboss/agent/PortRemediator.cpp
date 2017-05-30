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

namespace {
constexpr int kPortRemedyIntervalSec = 25;
}

namespace facebook { namespace fboss {

void PortRemediator::updatePortState(
    const boost::container::flat_set<PortID>& portIds,
    cfg::PortState newPortState,
    bool preventCoalescing) {
  if (portIds.empty()) {
    return;
  }
  auto updateFn =
      [=](const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};
        auto portMap = state->getPorts();
        for (const auto& portId : portIds) {
          auto port = portMap->getPortIf(portId);
          if (!port) {
            continue;
          }
          const auto newPort = port->modify(&newState);
          newPort->setState(newPortState);
        }
        return newState;
      };
  auto name = folly::sformat(
      "PortRemediator: flap {} down but enabled ports ({})",
      portIds.size(),
      newPortState == cfg::PortState::UP ? "up" : "down");
  if (preventCoalescing) {
    sw_->updateStateNoCoalescing(name, updateFn);
  } else {
    sw_->updateState(name, updateFn);
  }
}

boost::container::flat_set<PortID>
PortRemediator::getUnexpectedDownPorts() const {
  // TODO - Post t17304538, if Port::state == POWER_DOWN reflects
  // Admin down always, then just use SwitchState to to determine
  // which ports to ignore.
  boost::container::flat_set<PortID> unexpectedDownPorts;
  boost::container::flat_set<int> configEnabledPorts;
  for (const auto& cfgPort : sw_->getConfig().ports) {
    if (cfgPort.state != cfg::PortState::POWER_DOWN &&
        cfgPort.state != cfg::PortState::DOWN) {
      configEnabledPorts.insert(cfgPort.logicalID);
    }
  }
  const auto portMap = sw_->getState()->getPorts();
  for (const auto& port : *portMap) {
    if (port &&
        configEnabledPorts.find(port->getID()) != configEnabledPorts.end() &&
        !(port->getOperState())) {
      unexpectedDownPorts.insert(port->getID());
    }
  }
  return unexpectedDownPorts;
}

void PortRemediator::timeoutExpired() noexcept {
  auto unexpectedDownPorts = getUnexpectedDownPorts();
  if (!unexpectedDownPorts.empty()) {
    // First update cannot use coalescing or else the port
    // won't actually flap
    updatePortState(unexpectedDownPorts, cfg::PortState::DOWN, true);
    updatePortState(unexpectedDownPorts, cfg::PortState::UP, false);
  }
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
