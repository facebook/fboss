/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiFakePlatformPort.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

uint32_t SaiFakePlatformPort::getPhysicalLaneId(
    uint32_t chipId,
    uint32_t logicalLane) const {
  return chipId * 4 + logicalLane;
}

bool SaiFakePlatformPort::supportsTransceiver() const {
  return false;
}

std::vector<phy::PinConfig> SaiFakePlatformPort::getIphyPinConfigs(
    cfg::PortProfileID profileID) const {
  return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
      getPortID(), profileID);
}

uint32_t SaiFakePlatformPort::getCurrentLedState() const {
  return 0;
}

} // namespace facebook::fboss
