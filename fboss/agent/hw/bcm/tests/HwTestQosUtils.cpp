/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostUtils.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss::utility {

void disableTTLDecrements_Deprecated(
    HwSwitch* hw,
    RouterID routerId,
    InterfaceID /*intf*/,
    const folly::IPAddress& nhop) {
  auto bcmHw = static_cast<BcmSwitch*>(hw);
  auto vrfId = bcmHw->getBcmVrfId(routerId);
  auto bcmHostKey = BcmHostKey(vrfId, nhop);
  setTTLDecrement(bcmHw, bcmHostKey, true /* no decrement */);
}

void disableTTLDecrements_Deprecated(
    HwSwitch* /*hw*/,
    const PortDescriptor& /*port*/) {
  throw FbossError("Port disable decrement not supported on BRCM ASICs");
}

} // namespace facebook::fboss::utility
