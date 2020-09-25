/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiWedge400CPlatformPort.h"
#include "fboss/agent/platforms/common/utils/Wedge400LedUtils.h"

namespace facebook::fboss {

uint32_t SaiWedge400CPlatformPort::getPhysicalLaneId(
    uint32_t chipId,
    uint32_t logicalLaneId) const {
  return (chipId << 8) + logicalLaneId;
}

bool SaiWedge400CPlatformPort::supportsTransceiver() const {
  return true;
}

void SaiWedge400CPlatformPort::linkStatusChanged(bool up, bool adminUp) {
  currentLedState_ = Wedge400LedUtils::getLedState(
      getHwPortLanes(getCurrentProfile()).size(), up, adminUp);
  setLedStatus(currentLedState_);
}

void SaiWedge400CPlatformPort::externalState(PortLedExternalState lfs) {
  currentLedState_ =
      (lfs == PortLedExternalState::NONE
           ? currentLedState_
           : Wedge400LedUtils::getLedExternalState(lfs));
  setLedStatus(currentLedState_);
}

uint32_t SaiWedge400CPlatformPort::getCurrentLedState() const {
  return static_cast<uint32_t>(currentLedState_);
}

} // namespace facebook::fboss
