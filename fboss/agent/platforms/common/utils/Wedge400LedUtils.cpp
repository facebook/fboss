// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/common/utils/Wedge400LedUtils.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

FbDomFpga::LedColor
Wedge400LedUtils::getLedState(uint8_t numLanes, bool up, bool adminUp) {
  if (!up || !adminUp) {
    return FbDomFpga::LedColor::OFF;
  }
  switch (numLanes) {
    case 8:
    // TODO: T70244223. Clarifying the right LED color for uplinks
    case 4:
      return FbDomFpga::LedColor::BLUE;
    case 2:
      return FbDomFpga::LedColor::PINK;
    case 1:
      return FbDomFpga::LedColor::GREEN;
    default:
      throw FbossError("Number of lanes is incorrect");
  }
}

FbDomFpga::LedColor Wedge400LedUtils::getLedExternalState(
    PortLedExternalState lfs) {
  switch (lfs) {
    case PortLedExternalState::CABLING_ERROR:
      return FbDomFpga::LedColor::YELLOW;
      break;
    case PortLedExternalState::EXTERNAL_FORCE_ON:
      return FbDomFpga::LedColor::WHITE;
      break;
    case PortLedExternalState::EXTERNAL_FORCE_OFF:
      return FbDomFpga::LedColor::OFF;
      break;
    default:
      throw FbossError("Invalid port led external state");
      break;
  }
}
} // namespace facebook::fboss
