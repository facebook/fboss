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
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmError.h"

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

bool BcmPortGroup::validConfiguration(
  const std::shared_ptr<SwitchState>& state
) const {
  return true;
}

void BcmPortGroup::reconfigureIfNeeded(
  const std::shared_ptr<SwitchState>& state
) {
  // Check if reconfiguration is needed and reconfigure if so
}

void BcmPortGroup::reconfigure(
  const std::shared_ptr<SwitchState>& state,
  LaneMode newLaneMode
) {
  // The logic for this follows the steps required for flex-port support
  // outlined in the sdk documentation.

  // disable all group members

  // remove all ports from the counter DMA and linkscan bitmaps

  // set the opennslPortControlLanes setting

  // enable ports

  // add ports to the counter DMA + linkscan
}

}} // namespace facebook::fboss
