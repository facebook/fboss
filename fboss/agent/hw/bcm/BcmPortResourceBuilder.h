/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

class BcmPortResourceBuilder {
 public:
  BcmPortResourceBuilder(
      BcmSwitch* hw,
      BcmPort* controllingPort,
      BcmPortGroup::LaneMode desiredLaneMode);
  ~BcmPortResourceBuilder() {}

  void removePorts(const std::vector<BcmPort*>& ports);
  /*
   * In this addPorts, we'll filter out subsumed ports since the new FlexPort
   * apis actually needs us to remove subsumed ports to release physical lanes
   * for the controlling port to reach higher speed.
   * Return: the ports that will be actually created in the hardware
   */
  std::vector<std::shared_ptr<Port>> addPorts(
      const std::vector<std::shared_ptr<Port>>& ports);
  void program();

 private:
  std::vector<std::shared_ptr<Port>> filterSubSumedPorts(
      const std::vector<std::shared_ptr<Port>>& ports);

  int getBaseLane(std::shared_ptr<Port> port);

  void setPortSpecificControls(bcm_port_t bcmPort, bool enable);

  BcmSwitch* hw_;
  BcmPort* controllingPort_{nullptr};
  BcmPortGroup::LaneMode desiredLaneMode_;
  int numRemovedPorts_{0};
  int numAddedPorts_{0};
  bcm_port_t basePhysicalPort_{-1};
  std::vector<bcm_port_resource_t> portResources_;
};

} // namespace facebook::fboss
