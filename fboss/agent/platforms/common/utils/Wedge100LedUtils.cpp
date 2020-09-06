// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

int Wedge100LedUtils::getPortIndex(std::optional<ChannelID> /*channel*/) {
  // TODO: implement this
  throw FbossError("getLEDOffset is unimplemented");
  return 0;
}

Wedge100LedUtils::LedColor Wedge100LedUtils::getLEDColor(
    PortID port,
    int numberOfLanes,
    bool up,
    bool adminUp) {
  if (!up || !adminUp) {
    return Wedge100LedUtils::LedColor::OFF;
  }

  switch (numberOfLanes) {
    case 4: // Quaid
      return LedColor::BLUE;
    case 2: // DUAL
      return LedColor::MAGENTA;
    case 1: // Single
      return LedColor::GREEN;
  }

  throw FbossError("Unable to determine LED color for port ", port);
}

Wedge100LedUtils::LedColor Wedge100LedUtils::getLEDColor(
    PortLedExternalState externalState,
    Wedge100LedUtils::LedColor currentColor) {
  Wedge100LedUtils::LedColor color = Wedge100LedUtils::LedColor::OFF;
  switch (externalState) {
    case PortLedExternalState::NONE:
      color = currentColor;
      break;
    case PortLedExternalState::CABLING_ERROR:
      color = Wedge100LedUtils::LedColor::YELLOW;
      break;
    case PortLedExternalState::EXTERNAL_FORCE_ON:
      color = Wedge100LedUtils::LedColor::WHITE;
      break;
    case PortLedExternalState::EXTERNAL_FORCE_OFF:
      color = Wedge100LedUtils::LedColor::OFF;
      break;
  }
  return color;
}

} // namespace facebook::fboss
