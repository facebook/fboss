/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmWedge400PlatformPort.h"
#include "fboss/agent/platforms/common/utils/Wedge400LedUtils.h"

namespace facebook::fboss {

void SaiBcmWedge400PlatformPort::linkStatusChanged(bool up, bool adminUp) {
  if (FLAGS_led_controlled_through_led_service) {
    return;
  }
  currentLedState_ = Wedge400LedUtils::getLedState(
      getHwPortLanes(getCurrentProfile()).size(), up, adminUp);
  setLedStatus(currentLedState_);
}

void SaiBcmWedge400PlatformPort::externalState(PortLedExternalState lfs) {
  if (FLAGS_led_controlled_through_led_service) {
    XLOG(ERR)
        << "Set LED external state for port: " << getPortID()
        << " is called, please use led_service to call this thrift function";
  }
  currentLedState_ =
      Wedge400LedUtils::getLedExternalState(lfs, currentLedState_);
  setLedStatus(currentLedState_);
}

uint32_t SaiBcmWedge400PlatformPort::getCurrentLedState() const {
  return static_cast<uint32_t>(currentLedState_);
}

} // namespace facebook::fboss
