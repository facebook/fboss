/*
 *  Copyright (c) 2026-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiChenabPlatformPort.h"

namespace facebook::fboss {

uint32_t SaiChenabPlatformPort::getCurrentLedState() const {
  return currentLedState_;
}

uint32_t SaiChenabPlatformPort::getPhysicalLaneId(
    uint32_t /*chipId*/,
    uint32_t logicalLane) const {
  return logicalLane;
}

void SaiChenabPlatformPort::portChanged(
    std::shared_ptr<Port> /*newPort*/,
    std::shared_ptr<Port> /*oldPort*/) {}

void SaiChenabPlatformPort::linkStatusChanged(bool /*up*/, bool /*adminUp*/) {}

bool SaiChenabPlatformPort::supportsTransceiver() const {
  // SaiChenabPlatformPort::supportsTransceiver() only affects LinkTest today:
  // SaiPlatformPort::getTransceiverMapping() is the only caller, and that path
  // is only reached through LinkTest's platform mapping consistency check.
  // Production transceiver index reporting reads PlatformMapping directly.
  return true;
}

} // namespace facebook::fboss
