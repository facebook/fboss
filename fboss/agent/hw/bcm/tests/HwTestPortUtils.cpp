/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestPortUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss::utility {

void setPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    cfg::PortLoopbackMode lbMode) {
  auto rv = bcm_port_loopback_set(
      static_cast<const BcmSwitch*>(hw)->getUnit(),
      port,
      utility::fbToBcmLoopbackMode(lbMode));
  facebook::fboss::bcmCheckError(
      rv, "failed to set loopback mode state for port");
}

void setCreditWatchdogAndPortTx(const HwSwitch* hw, PortID port, bool enable) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hw);

  auto bcmPortId = bcmSwitch->getPortTable()->getBcmPort(port)->getBcmPortId();
  auto rv = bcm_port_control_set(
      bcmSwitch->getUnit(), bcmPortId, bcmPortControlTxEnable, enable ? 1 : 0);
  bcmCheckError(rv, "failed to disable TX");
}

int getLoopbackMode(cfg::PortLoopbackMode lbMode) {
  return fbToBcmLoopbackMode(lbMode);
}

void enableTransceiverProgramming(bool /*enable*/) {}
} // namespace facebook::fboss::utility
