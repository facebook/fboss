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

DEFINE_bool(skip_led_programming, false, "Skip programming LED");

namespace facebook::fboss {

void SaiWedge400CPlatformPort::linkStatusChanged(bool up, bool adminUp) {
  if (FLAGS_skip_led_programming) {
    // LED programming should be skipped
    return;
  }

  currentLedState_ = Wedge400LedUtils::getLedState(
      getHwPortLanes(getCurrentProfile()).size(), up, adminUp);
  setLedStatus(currentLedState_);
}

void SaiWedge400CPlatformPort::externalState(PortLedExternalState lfs) {
  if (FLAGS_skip_led_programming) {
    // LED programming should be skipped
    return;
  }
  currentLedState_ =
      Wedge400LedUtils::getLedExternalState(lfs, currentLedState_);
  setLedStatus(currentLedState_);
}

uint32_t SaiWedge400CPlatformPort::getCurrentLedState() const {
  return static_cast<uint32_t>(currentLedState_);
}
} // namespace facebook::fboss
