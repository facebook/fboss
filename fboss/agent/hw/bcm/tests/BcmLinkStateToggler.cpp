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
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/TestEnsembleIf.h"

#include <memory>

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

BcmSwitch* BcmLinkStateToggler::getHw() {
  return static_cast<BcmSwitch*>(getHwSwitchEnsemble()->getHwSwitch());
}

void BcmLinkStateToggler::invokeLinkScanIfNeeded(PortID port, bool isUp) {
  auto* platform = dynamic_cast<BcmPlatform*>(getHw()->getPlatform());
  CHECK(platform);
  if (!platform->hasLinkScanCapability()) {
    /* if platform doesn't support link-scan, such as fake then mimic linkscan
     * by specifically invoking hwSwitch's link scan callback */
    bcm_port_info_t portInfo;
    portInfo.linkstatus =
        isUp ? BCM_PORT_LINK_STATUS_UP : BCM_PORT_LINK_STATUS_DOWN;
    getHw()->linkscanCallback(getHw()->getUnit(), port, &portInfo);
  }
}

void BcmLinkStateToggler::setPortPreemphasis(
    const std::shared_ptr<Port>& port,
    int preemphasis) {
  auto portID = port->getID();
  auto platformPort =
      getHw()->getPortTable()->getBcmPort(portID)->getPlatformPort();
  int rv;
  if (!platformPort->shouldUsePortResourceAPIs()) {
    rv = bcm_port_phy_control_set(
        getHw()->getUnit(),
        portID,
        BCM_PORT_PHY_CONTROL_PREEMPHASIS,
        preemphasis);
  } else {
    bcm_port_phy_tx_t tx;
    bcm_port_phy_tx_t_init(&tx);
    rv = bcm_port_phy_tx_get(getHw()->getUnit(), portID, &tx);
    bcmCheckError(rv, "Failed to get port tx settings");
    tx.pre = preemphasis & 0xf; // 0-3 bits
    tx.main = (preemphasis & 0x3f0) >> 4; // 4-9 bits
    tx.post = (preemphasis & 0x7c00) >> 10; // 10-14 bits
    rv = bcm_port_phy_tx_set(getHw()->getUnit(), portID, &tx);
  }
  bcmCheckError(rv, "Failed to set port preemphasis");
}

void BcmLinkStateToggler::setLinkTraining(
    const std::shared_ptr<Port>& /* port */,
    bool /* enable */) {
  // At the moment, we don't have any use case to set link training on
  // BcmSwitch. We could consider adding it if needed later.
  CHECK(false) << "Setting Link Training is not supported";
}

} // namespace facebook::fboss
