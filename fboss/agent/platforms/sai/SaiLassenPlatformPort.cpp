/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiLassenPlatformPort.h"
#include "fboss/agent/platforms/common/utils/Wedge400LedUtils.h"

namespace facebook::fboss {

uint32_t SaiLassenPlatformPort::getPhysicalLaneId(
    uint32_t chipId,
    uint32_t logicalLaneId) const {
  return (chipId << 8) + logicalLaneId;
}

bool SaiLassenPlatformPort::supportsTransceiver() const {
  return true;
}

void SaiLassenPlatformPort::linkStatusChanged(
    bool /* up */,
    bool /* adminUp */) {}

void SaiLassenPlatformPort::externalState(PortLedExternalState /* lfs */) {}

uint32_t SaiLassenPlatformPort::getCurrentLedState() const {
  return static_cast<uint32_t>(currentLedState_);
}

} // namespace facebook::fboss
