/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/wedge100/Wedge100Port.h"

#include "fboss/agent/platforms/wedge/wedge100/Wedge100Platform.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

Wedge100Port::Wedge100Port(PortID id, Wedge100Platform* platform)
    : WedgePort(id, platform) {}

std::vector<phy::PinConfig> Wedge100Port::getIphyPinConfigs(
    cfg::PortProfileID profileID) const {
  if (!supportsTransceiver()) {
    return {};
  }
  folly::EventBase evb;
  auto transceiverInfo = getTransceiverInfo(&evb);
  if (transceiverInfo) {
    if (auto cable = transceiverInfo->cable_ref()) {
      if (auto cableLength = cable->length_ref()) {
        cfg::PlatformPortConfigOverrideFactor matcher;
        matcher.cableLengths_ref() = {
            std::max(1.0, std::min(3.0, *cableLength))};
        return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
            PlatformPortProfileConfigMatcher(profileID, getPortID(), matcher));
      }
    }
  }
  return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
      PlatformPortProfileConfigMatcher(profileID, getPortID()));
}

bool Wedge100Port::isTop() {
  return Wedge100LedUtils::isTop(getTransceiverID());
}

} // namespace facebook::fboss
