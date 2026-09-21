/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiElbert8DDPhyPlatformPort.h"

namespace facebook::fboss {

uint32_t SaiElbert8DDPhyPlatformPort::getPhysicalLaneId(
    uint32_t chipId,
    uint32_t logicalLaneId) const {
  return (chipId << 8) + logicalLaneId;
}

bool SaiElbert8DDPhyPlatformPort::supportsTransceiver() const {
  return true;
}

void SaiElbert8DDPhyPlatformPort::linkStatusChanged(
    bool /* up */,
    bool /* adminUp */) {}

void SaiElbert8DDPhyPlatformPort::externalState(
    PortLedExternalState /* lfs */) {}

uint32_t SaiElbert8DDPhyPlatformPort::getCurrentLedState() const {
  return 0;
}

} // namespace facebook::fboss
