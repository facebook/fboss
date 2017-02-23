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

extern "C" {
#include <opennsl/types.h>
#include <opennsl/port.h>
}

#include "fboss/agent/types.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <mutex>
#include <boost/container/flat_map.hpp>

namespace facebook { namespace fboss {

class BcmSwitch;
class SwitchState;

/*
 * BcmPortGroup is a class that represents a group of (four) BcmPorts
 * that can be combined into the same logical port to achieve higher
 * speeds. While this can be done via a config file at startup, this
 * class also has logic to reconfigure a set of ports into either one,
 * two or four ports dynamically so that we can support different
 * configurations, without having to change the config file. This takes
 * advantage of the "Flex Port" feature provided by Broadcom.
 */
class BcmPortGroup {
 public:
  /*
   * These are the different lane configurations supported for a port group.
   *
   * Note that the sdk actually supports asymmetric 3-port
   * configurations as well. We can add those in the future should a
   * need arise.
   */
  enum LaneMode : uint8_t {
    SINGLE = 1,
    DUAL = 2,
    QUAD = 4
  };

  BcmPortGroup(BcmSwitch *hw,
               BcmPort* controllingPort,
               std::vector<BcmPort*> allPorts);
  ~BcmPortGroup();

  BcmPort* controllingPort() const {
    return controllingPort_;
  }

  const std::vector<BcmPort*>& allPorts() const {
    return allPorts_;
  }

  uint8_t maxLanes() const {
    return allPorts_.size();
  }

  void reconfigureIfNeeded(const std::shared_ptr<SwitchState>& state);

  bool validConfiguration(const std::shared_ptr<SwitchState>& state) const;

  static BcmPortGroup::LaneMode calculateDesiredLaneMode(
    const std::vector<Port*>& ports, LaneSpeeds supportedLaneSpeeds);

  LaneMode laneMode() {
    return laneMode_;
  }

 private:
  std::vector<Port*> getSwPorts(
    const std::shared_ptr<SwitchState>& state) const;
  LaneMode neededLaneMode(uint8_t lane, cfg::PortSpeed speed) const;

  uint8_t getLane(const BcmPort* bcmPort) const;
  int retrieveActiveLanes() const;
  void setActiveLanes(LaneMode desiredLaneMode);
  void reconfigureLaneMode(const std::shared_ptr<SwitchState>& state,
                   LaneMode newLaneMode);

  BcmSwitch* hw_;
  BcmPort* controllingPort_{nullptr};
  std::vector<BcmPort*> allPorts_;
  LaneMode laneMode_;
  cfg::PortSpeed portSpeed_;
};

}} // namespace facebook::fboss
