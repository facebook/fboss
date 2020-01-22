/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmLinkStateToggler.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"
#include "fboss/agent/state/Port.h"

#include <memory>

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

void BcmLinkStateToggler::invokeLinkScanIfNeeded(PortID port, bool isUp) {
  if (!static_cast<BcmTestPlatform*>(hw_->getPlatform())
           ->hasLinkScanCapability()) {
    bcm_port_info_t portInfo;
    portInfo.linkstatus =
        isUp ? BCM_PORT_LINK_STATUS_UP : BCM_PORT_LINK_STATUS_DOWN;
    hw_->linkscanCallback(hw_->getUnit(), port, &portInfo);
  }
}

void BcmLinkStateToggler::setPortPreemphasis(
    const std::shared_ptr<Port>& port,
    int preemphasis) {
  auto rv = bcm_port_phy_control_set(
      hw_->getUnit(),
      port->getID(),
      BCM_PORT_PHY_CONTROL_PREEMPHASIS,
      preemphasis);
  bcmCheckError(rv, "Failed to set port preemphasis");
}

} // namespace facebook::fboss
