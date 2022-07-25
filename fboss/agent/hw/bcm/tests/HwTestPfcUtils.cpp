/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestPfcUtils.h"
#include <gtest/gtest.h>
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

// Gets the PFC enabled/disabled status for RX/TX from HW
void getPfcEnabledStatus(
    const HwSwitch* hw,
    const PortID& portId,
    bool& pfcRx,
    bool& pfcTx) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto bcmPort = bcmSwitch->getPortTable()->getBcmPort(portId);
  int rx = 0;
  int tx = 0;
  bcmPort->getProgrammedPfcState(&rx, &tx);
  pfcRx = rx ? true : false;
  pfcTx = tx ? true : false;
  XLOG(DBG0) << "Read PFC rx " << rx << " tx " << tx;
}

} // namespace facebook::fboss::utility
