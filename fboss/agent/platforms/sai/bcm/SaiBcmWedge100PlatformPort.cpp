// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmWedge100PlatformPort.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"

#include <folly/logging/xlog.h>
#include <tuple>

namespace facebook::fboss {

void SaiBcmWedge100PlatformPort::linkStatusChanged(bool up, bool adminUp) {
  // TODO(pshaikh): add support for compact mode LED
  uint32_t phyPortId = getPhysicalPortId();
  auto ledAndIndex = getLedAndIndex(phyPortId);
  if (!ledAndIndex) {
    return;
  }

  auto [led, index] = ledAndIndex.value();
  auto lanes = getHwPortLanes(getCurrentProfile());

  try {
    uint32_t status =
        static_cast<uint32_t>(Wedge100LedUtils::getDesiredLEDState(
            static_cast<int>(lanes.size()), up, adminUp));
    setLEDState(led, index, status);
  } catch (const FbossError& ex) {
    XLOG(ERR) << "exception while getting LED state for port: "
              << static_cast<PortID>(phyPortId) << ":" << ex.what();
    throw;
  }
}

void SaiBcmWedge100PlatformPort::externalState(PortLedExternalState lfs) {
  uint32_t phyPortId = getPhysicalPortId();
  auto ledAndIndex = getLedAndIndex(phyPortId);
  if (!ledAndIndex) {
    return;
  }

  auto [led, index] = ledAndIndex.value();

  uint32_t status = static_cast<uint32_t>(Wedge100LedUtils::getDesiredLEDState(
      lfs, static_cast<Wedge100LedUtils::LedColor>(getCurrentLedState())));
  setLEDState(led, index, status);
}

std::optional<std::tuple<uint32_t, uint32_t>>
SaiBcmWedge100PlatformPort::getLedAndIndex(uint32_t phyPortId) {
  PortID port = static_cast<PortID>(phyPortId);
  int index = Wedge100LedUtils::getPortIndex(port);
  auto led = Wedge100LedUtils::getLEDProcessorNumber(port);
  if (!led) {
    return std::nullopt;
  }
  return std::make_tuple(led.value(), index);
}
} // namespace facebook::fboss
