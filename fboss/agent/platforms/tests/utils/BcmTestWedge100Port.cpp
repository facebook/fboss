/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestWedge100Port.h"

#include "fboss/agent/platforms/tests/utils/BcmTestWedge100Platform.h"

namespace facebook::fboss {

BcmTestWedge100Port::BcmTestWedge100Port(
    PortID id,
    BcmTestWedge100Platform* platform)
    : BcmTestPort(id, platform) {}

std::vector<phy::PinConfig> BcmTestWedge100Port::getIphyPinConfigs(
    cfg::PortProfileID profileID) const {
  // For Wedge100 getPortIphyPinConfigs needs a cable length to be able to
  // match platform port config overrides. Hardcode cable length for tests so we
  // don't need qsfp service
  return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
      getPortID(), profileID, 1.0);
}

} // namespace facebook::fboss
