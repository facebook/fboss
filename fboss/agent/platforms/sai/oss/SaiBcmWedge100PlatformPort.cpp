// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmWedge100PlatformPort.h"

namespace facebook::fboss {

void SaiBcmWedge100PlatformPort::linkStatusChanged(
    bool /*up*/,
    bool /*adminUp*/) {}

void SaiBcmWedge100PlatformPort::externalState(PortLedExternalState) {}

std::optional<std::tuple<uint32_t, uint32_t>>
SaiBcmWedge100PlatformPort::getLedAndIndex(uint32_t /*phyPortId*/) {
  return std::nullopt;
}

bool SaiBcmWedge100PlatformPort::useCompactMode() const {
  return false;
}

void SaiBcmWedge100PlatformPort::compactLEDStateChange(uint32_t /*status*/) {}
} // namespace facebook::fboss
