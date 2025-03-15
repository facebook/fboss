/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiMinipack3NPlatformPort.h"

namespace facebook::fboss {

uint32_t SaiMinipack3NPlatformPort::getCurrentLedState() const {
  return static_cast<uint32_t>(currentLedState_);
}
uint32_t SaiMinipack3NPlatformPort::getPhysicalLaneId(
    uint32_t /*chipId*/,
    uint32_t logicalLane) const {
  return logicalLane;
}
void SaiMinipack3NPlatformPort::portChanged(
    std::shared_ptr<Port> /*newPort*/,
    std::shared_ptr<Port> /*oldPort*/) {}
void SaiMinipack3NPlatformPort::linkStatusChanged(
    bool /*up*/,
    bool /*adminUp*/) {}
bool SaiMinipack3NPlatformPort::supportsTransceiver() const {
  return false;
}

} // namespace facebook::fboss
