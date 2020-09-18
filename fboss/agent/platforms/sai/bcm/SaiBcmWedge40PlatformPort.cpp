// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmWedge40PlatformPort.h"

#include "fboss/agent/platforms/common/utils/Wedge40LedUtils.h"

namespace facebook::fboss {

void SaiBcmWedge40PlatformPort::linkStatusChanged(bool up, bool adminUp) {
  PortID phyPortId = static_cast<PortID>(getPhysicalPortId());
  uint32_t index = Wedge40LedUtils::getPortIndex(phyPortId);
  auto led = Wedge40LedUtils::getLEDProcessorNumber(phyPortId);
  if (!led) {
    return;
    led = 0;
  }
  // write to LED
  setLEDState(
      led.value(),
      index,
      static_cast<uint32_t>(Wedge40LedUtils::getDesiredLEDState(up, adminUp)));
}

} // namespace facebook::fboss
