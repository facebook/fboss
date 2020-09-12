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

Wedge100Port::Wedge100Port(
    PortID id,
    Wedge100Platform* platform,
    std::optional<FrontPanelResources> frontPanel)
    : WedgePort(id, platform, frontPanel) {}

std::vector<phy::PinConfig> Wedge100Port::getIphyPinConfigs(
    cfg::PortProfileID profileID) const {
  if (!supportsTransceiver()) {
    return {};
  }
  folly::EventBase evb;
  if (auto cable = getCableInfo(&evb).getVia(&evb)) {
    if (auto cableLength = cable->length_ref()) {
      auto cableMeters = std::max(1.0, std::min(3.0, *cableLength));
      return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
          getPortID(), profileID, cableMeters);
    }
  }
  // if cable length was not found, try to get config without it
  return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
      getPortID(), profileID);
}

bool Wedge100Port::isTop() {
  return Wedge100LedUtils::isTop(getTransceiverID());
}

} // namespace facebook::fboss
