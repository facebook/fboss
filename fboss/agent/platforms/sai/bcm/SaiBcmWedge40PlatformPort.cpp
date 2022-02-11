// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmWedge40PlatformPort.h"

#include "fboss/agent/platforms/common/utils/Wedge40LedUtils.h"

namespace facebook::fboss {

void SaiBcmWedge40PlatformPort::linkStatusChanged(bool up, bool adminUp) {
  uint32_t index = Wedge40LedUtils::getPortIndex(getPortID());
  auto led = Wedge40LedUtils::getLEDProcessorNumber(getPortID());
  if (!led) {
    // We only show link status for the first 64 ports.
    XLOG(WARNING) << "port " << getPortID()
                  << " is not supported for LED set operation";
    return;
  }
  // write to LED
  setLEDState(
      led.value(),
      Wedge40LedUtils::getPortOffset(index),
      static_cast<uint32_t>(Wedge40LedUtils::getDesiredLEDState(up, adminUp)));
}

} // namespace facebook::fboss
