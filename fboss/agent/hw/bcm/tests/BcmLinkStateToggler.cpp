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
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/state/Port.h"

#include <memory>

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

void BcmLinkStateToggler::invokeLinkScanIfNeeded(PortID port, bool isUp) {
  if (!static_cast<BcmTestPlatform*>(hw_->getPlatform())
           ->hasLinkScanCapability()) {
    /* if platform doesn't support link-scan, such as fake then mimic linkscan
     * by specifically invoking hwSwitch's link scan callback */
    bcm_port_info_t portInfo;
    portInfo.linkstatus =
        isUp ? BCM_PORT_LINK_STATUS_UP : BCM_PORT_LINK_STATUS_DOWN;
    hw_->linkscanCallback(hw_->getUnit(), port, &portInfo);
  }
}

void BcmLinkStateToggler::setPortPreemphasis(
    const std::shared_ptr<Port>& port,
    int preemphasis) {
  auto portID = port->getID();
  auto platformPort =
      hw_->getPortTable()->getBcmPort(portID)->getPlatformPort();
  int rv;
  if (!platformPort->shouldUsePortResourceAPIs()) {
    rv = bcm_port_phy_control_set(
        hw_->getUnit(), portID, BCM_PORT_PHY_CONTROL_PREEMPHASIS, preemphasis);
  } else {
    bcm_port_phy_tx_t tx;
    bcm_port_phy_tx_t_init(&tx);
    rv = bcm_port_phy_tx_get(hw_->getUnit(), portID, &tx);
    bcmCheckError(rv, "Failed to get port tx settings");
    tx.pre = preemphasis & 0xf; // 0-3 bits
    tx.main = (preemphasis & 0x3f0) >> 4; // 4-9 bits
    tx.post = (preemphasis & 0x7c00) >> 10; // 10-14 bits
    rv = bcm_port_phy_tx_set(hw_->getUnit(), portID, &tx);
  }
  bcmCheckError(rv, "Failed to set port preemphasis");
}

} // namespace facebook::fboss
