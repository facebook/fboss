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

#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge100Platform.h"
#include "fboss/agent/platforms/wedge/utils/BcmLedUtils.h"

namespace facebook::fboss {

BcmTestWedge100Port::BcmTestWedge100Port(
    const PortID& id,
    BcmTestWedge100Platform* platform)
    : BcmTestPort(id, platform) {}

void BcmTestWedge100Port::linkStatusChanged(bool up, bool adminUp) {
  // Using non-compact mode with single lane mode
  auto bcmPort = getBcmPort();

  auto color = Wedge100LedUtils::getDesiredLEDState(
      BcmTestPort::numberOfLanes(), up, adminUp);
  facebook::fboss::BcmLedUtils::setWedge100PortStatus(
      0, bcmPort->getPortID(), static_cast<uint32_t>(color));
}

} // namespace facebook::fboss
