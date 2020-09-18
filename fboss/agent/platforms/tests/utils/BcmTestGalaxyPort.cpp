/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestGalaxyPort.h"

#include "fboss/agent/platforms/common/utils/GalaxyLedUtils.h"
#include "fboss/agent/platforms/tests/utils/BcmTestGalaxyPlatform.h"
#include "fboss/agent/platforms/wedge/utils/BcmLedUtils.h"

namespace facebook::fboss {

BcmTestGalaxyPort::BcmTestGalaxyPort(PortID id, BcmTestGalaxyPlatform* platform)
    : BcmTestPort(id, platform) {}

void BcmTestGalaxyPort::linkStatusChanged(bool up, bool adminUp) {
  uint32_t portData = BcmLedUtils::getGalaxyPortStatus(0, getPortID());
  GalaxyLedUtils::getDesiredLEDState(&portData, up, adminUp);
  BcmLedUtils::setGalaxyPortStatus(0, getPortID(), portData);
}

} // namespace facebook::fboss
