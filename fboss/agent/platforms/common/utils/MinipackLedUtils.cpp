// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/common/utils/MinipackLedUtils.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

MinipackLed::Color MinipackLedUtils::getLedState(bool up, bool adminUp) {
  if (!up || !adminUp) {
    return MinipackLed::Color::OFF;
  }
  return MinipackLed::Color::BLUE;
}

MinipackLed::Color MinipackLedUtils::getLedExternalState(
    PortLedExternalState lfs,
    MinipackLed::Color internalState) {
  switch (lfs) {
    case PortLedExternalState::NONE:
      return internalState;
    case PortLedExternalState::CABLING_ERROR:
      return MinipackLed::Color::YELLOW;
    case PortLedExternalState::EXTERNAL_FORCE_ON:
      return MinipackLed::Color::WHITE;
    case PortLedExternalState::EXTERNAL_FORCE_OFF:
      return MinipackLed::Color::OFF;
    default:
      throw FbossError("Invalid port led external state");
      break;
  }
}
} // namespace facebook::fboss
