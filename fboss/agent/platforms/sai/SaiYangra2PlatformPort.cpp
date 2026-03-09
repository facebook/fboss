/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiYangra2PlatformPort.h"

namespace facebook::fboss {

uint32_t SaiYangra2PlatformPort::getCurrentLedState() const {
  return static_cast<uint32_t>(currentLedState_);
}
uint32_t SaiYangra2PlatformPort::getPhysicalLaneId(
    uint32_t /*chipId*/,
    uint32_t logicalLane) const {
  return logicalLane;
}
void SaiYangra2PlatformPort::portChanged(
    std::shared_ptr<Port> /*newPort*/,
    std::shared_ptr<Port> /*oldPort*/) {}
void SaiYangra2PlatformPort::linkStatusChanged(bool /*up*/, bool /*adminUp*/) {}
bool SaiYangra2PlatformPort::supportsTransceiver() const {
  return false;
}

} // namespace facebook::fboss
