/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestWedge40Port.h"

#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/platforms/common/utils/Wedge40LedUtils.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge40Platform.h"
#include "fboss/agent/platforms/wedge/utils/BcmLedUtils.h"

namespace facebook::fboss {

BcmTestWedge40Port::BcmTestWedge40Port(
    PortID id,
    BcmTestWedge40Platform* platform)
    : BcmTestPort(id, platform) {}

void BcmTestWedge40Port::linkStatusChanged(bool up, bool adminUp) {
  auto bcmPort = getBcmPort();
  uint32_t value =
      static_cast<uint32_t>(Wedge40LedUtils::getDesiredLEDState(up, adminUp));
  BcmLedUtils::setWedge40PortStatus(0, bcmPort->getPortID(), value);
}
} // namespace facebook::fboss
