/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmError.h"

#include "fboss/agent/state/Port.h"

extern "C" {
#include <opennsl/port.h>
}

namespace facebook { namespace fboss {

BcmPortGroup::BcmPortGroup(BcmSwitch *hw,
                           BcmPort* controllingPort,
                           std::vector<BcmPort*> allPorts)
    : hw_(hw),
      controllingPort_(controllingPort),
      allPorts_(std::move(allPorts)) {
  // The number of ports in a group should always be 4.
  CHECK(allPorts_.size() == 4);

  // get the number of active lanes
  auto activeLanes = retrieveActiveLanes();
  switch (activeLanes) {
    case 1:
      laneMode_ = LaneMode::QUAD;
      break;
    case 2:
      laneMode_ = LaneMode::DUAL;
      break;
    case 4:
      laneMode_ = LaneMode::SINGLE;
      break;
    default:
      throw FbossError("Unexpected number of lanes retrieved for bcm port ",
                       controllingPort_->getBcmPortId());
  }

}

BcmPortGroup::~BcmPortGroup() {}

BcmPortGroup::LaneMode BcmPortGroup::getDesiredLaneMode(
    const std::shared_ptr<SwitchState>& state) const {
  auto desiredMode = LaneMode::QUAD;
  for (auto bcmPort : allPorts_) {
    auto lane = getLane(bcmPort);
    auto swPort = bcmPort->getSwitchStatePort(state);

    // Make sure the ports support the configured speed.
    // We check this even if the port is disabled.
    if (!bcmPort->supportsSpeed(swPort->getSpeed())) {
      throw FbossError("Port ", swPort->getID(), " does not support speed ",
                       static_cast<int>(swPort->getSpeed()));
    }

    if (!swPort->isDisabled()) {
      auto neededMode = neededLaneMode(lane, swPort->getSpeed());

      if (neededMode != desiredMode) {
        desiredMode = neededMode;
      }
      VLOG(3) << "Port " << swPort->getID() << " enabled with speed " <<
        static_cast<int>(swPort->getSpeed());
    }
  }
  return desiredMode;
}

uint8_t BcmPortGroup::getLane(const BcmPort* bcmPort) const {
  return bcmPort->getBcmPortId() - controllingPort_->getBcmPortId();
}

BcmPortGroup::LaneMode BcmPortGroup::neededLaneMode(
    uint8_t lane, cfg::PortSpeed speed) const {
  auto speedPerLane = controllingPort_->maxLaneSpeed();
  auto neededLanes =
    static_cast<uint32_t>(speed) / static_cast<uint32_t>(speedPerLane);
  if (neededLanes == 1) {
    return LaneMode::QUAD;
  } else if (neededLanes == 2) {
    if (lane != 0 && lane != 2) {
      throw FbossError("Only lanes 0 or 2 can be enabled in DUAL mode");
    }
    return LaneMode::DUAL;
  } else if (neededLanes > 2 && neededLanes <= 4) {
    if (lane != 0) {
      throw FbossError("Only lane 0 can be enabled in SINGLE mode");
    }
    return LaneMode::SINGLE;
  } else {
    throw FbossError("Cannot support speed ", speed);
  }
}

bool BcmPortGroup::validConfiguration(
    const std::shared_ptr<SwitchState>& state) const {
  try {
    getDesiredLaneMode(state);
  } catch (const std::exception& ex) {
    return false;
  }
  return true;
}

void BcmPortGroup::reconfigureIfNeeded(
  const std::shared_ptr<SwitchState>& state
) {
  // This logic is a bit messy. We could encode some notion of port
  // groups into the swith state somehow so it is easy to generate
  // deltas for these. For now, we need pass around the SwitchState
  // object and get the relevant ports manually.
  auto desiredLaneMode = getDesiredLaneMode(state);

  if (desiredLaneMode != laneMode_) {
    reconfigure(state, desiredLaneMode);
  }
}

void BcmPortGroup::reconfigure(
  const std::shared_ptr<SwitchState>& state,
  LaneMode newLaneMode
) {
  // The logic for this follows the steps required for flex-port support
  // outlined in the sdk documentation.
  VLOG(1) << "Reconfiguring port " << controllingPort_->getBcmPortId()
          << " from " << laneMode_ << " active ports to " << newLaneMode
          << " active ports";

  // disable all group members

  // remove all ports from the counter DMA and linkscan bitmaps

  // set the opennslPortControlLanes setting

  // enable ports

  // add ports to the counter DMA + linkscan
}

}} // namespace facebook::fboss
