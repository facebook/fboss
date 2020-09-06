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

int Wedge100LedUtils::getPipe(PortID port) {
  return static_cast<int>(port) / 34;
}

int Wedge100LedUtils::getPortIndex(PortID port) {
  int pipe = getPipe(port);
  int offset = static_cast<int>(port) % 34;
  if (pipe == 0) {
    --offset;
  }
  int index = offset;
  if (pipe == 3 || pipe == 2) {
    index += 32;
  }
  return index;
}

bool Wedge100LedUtils::isTop(std::optional<TransceiverID> tcvrID) {
  if (tcvrID.has_value()) {
    return !((*tcvrID) & 0x1);
  }
  return false;
}

} // namespace facebook::fboss
