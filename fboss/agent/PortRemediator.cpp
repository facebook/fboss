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
#include "fboss/agent/Platform.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"

namespace {
constexpr int kPortRemedyIntervalSec = 25;

using facebook::fboss::SwSwitch;
using facebook::fboss::SwitchState;
using facebook::fboss::PortID;
using facebook::fboss::cfg::PortState;
void updatePortState(
    SwSwitch* sw,
    PortID portId,
    PortState newPortState) {
  auto updateFn =
      [portId, newPortState](
          const std::shared_ptr<SwitchState>& state) {
        std::shared_ptr<SwitchState> newState{state};
        auto port = state->getPorts()->getPortIf(portId);
        if (!port) {
          LOG(WARNING) << "Could not flap port " << portId
                       << " not found in port map";
          return newState;
        }
        const auto newPort = port->modify(&newState);
        newPort->setState(newPortState);
        return newState;
      };
  auto name = folly::sformat(
      "PortRemediator: flap down but enabled port {} ({})",
      static_cast<uint16_t>(portId),
      newPortState == PortState::UP ? "up" : "down");
    sw->updateStateNoCoalescing(name, updateFn);
}
}

namespace facebook { namespace fboss {

boost::container::flat_set<PortID>
PortRemediator::getUnexpectedDownPorts() const {
  boost::container::flat_set<PortID> unexpectedDownPorts;
  const auto portMap = sw_->getState()->getPorts();
  for (const auto& port : *portMap) {
    if (port && !port->isAdminDisabled() && !port->getOperState()) {
      unexpectedDownPorts.insert(port->getID());
    }
  }
  return unexpectedDownPorts;
}

void PortRemediator::timeoutExpired() noexcept {
  auto unexpectedDownPorts = getUnexpectedDownPorts();
  auto platform = sw_->getPlatform();
  for (const auto& portId : unexpectedDownPorts) {
    auto platformPort = platform->getPlatformPort(portId);
    if (!platformPort->supportsTransceiver()) {
      continue;
    }
    auto infoFuture = platformPort->getTransceiverInfo();
    infoFuture.via(sw_->getBackgroundEVB())
        .then([platformPort](TransceiverInfo info) {
          if (info.present) {
            platformPort->customizeTransceiver();
          }
        });
  }
  scheduleTimeout(interval_);
}

PortRemediator::PortRemediator(SwSwitch* swSwitch)
    : AsyncTimeout(swSwitch->getBackgroundEVB()),
      sw_(swSwitch),
      interval_(kPortRemedyIntervalSec) {
};

void PortRemediator::init() {
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
