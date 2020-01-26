/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPort.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

cfg::PortSpeed BcmPort::getMaxSpeed() const {
  int speed;
  auto unit = hw_->getUnit();
  auto rv = bcm_port_speed_max(hw_->getUnit(), port_, &speed);
  bcmCheckError(rv, "Failed to get max speed for port ", port_);
  return cfg::PortSpeed(speed);
}

} // namespace facebook::fboss
