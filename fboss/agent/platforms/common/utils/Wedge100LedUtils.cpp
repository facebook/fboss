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
    bool /*up*/,
    bool /*adminUp*/) {
  throw FbossError("getLEDColor is unimplemented");

  return Wedge100LedUtils::LedColor::OFF;
}

Wedge100LedUtils::LedColor getLEDColor(PortLedExternalState /*externalState*/) {
  throw FbossError("getLEDColor is unimplemented");
  return Wedge100LedUtils::LedColor::OFF;
}

} // namespace facebook::fboss
