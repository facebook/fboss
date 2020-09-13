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
#include <bcm/init.h>
#include <bcm/port.h>
}

namespace facebook::fboss {

cfg::PortSpeed BcmPort::getMaxSpeed() const {
  int speed;
  auto rv = bcm_port_speed_max(hw_->getUnit(), port_, &speed);
  bcmCheckError(rv, "Failed to get max speed for port ", port_);
  return cfg::PortSpeed(speed);
}

uint8_t BcmPort::determinePipe() const {
  // almost certainly open sourced since we updated bcm...
  bcm_info_t info;
  auto rv = bcm_info_get(unit_, &info);
  bcmCheckError(rv, "failed to get unit info");

  bcm_port_config_t portConfig;
  bcm_port_config_t_init(&portConfig);
  rv = bcm_port_config_get(unit_, &portConfig);
  bcmCheckError(rv, "failed to get port configuration");

  for (int i = 0; i < info.num_pipes; ++i) {
    if (BCM_PBMP_MEMBER(portConfig.per_pipe[i], port_)) {
      return i;
    }
  }

  throw FbossError("Port ", port_, " not associated w/ any pipe");
}

} // namespace facebook::fboss
