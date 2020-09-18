// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmGalaxyPlatformPort.h"
#include "fboss/agent/platforms/common/utils/GalaxyLedUtils.h"

namespace facebook::fboss {

void SaiBcmGalaxyPlatformPort::linkStatusChanged(bool up, bool adminUp) {
  PortID phyPortId = static_cast<PortID>(getPhysicalPortId());

  if (phyPortId == 129 || phyPortId == 131) {
    return;
  }
  uint32_t index = GalaxyLedUtils::getPortIndex(phyPortId);
  auto led = GalaxyLedUtils::getLEDProcessorNumber(phyPortId);
  if (!led) {
    return;
  }
  uint32_t status = getLEDState(led.value(), index);
  GalaxyLedUtils::getDesiredLEDState(&status, up, adminUp);
  setLEDState(led.value(), index, status);
}

} // namespace facebook::fboss
