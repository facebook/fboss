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
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss::utility {

void disableTTLDecrements(
    HwSwitch* hw,
    RouterID routerId,
    InterfaceID /*intf*/,
    const folly::IPAddress& nhop) {
  auto bcmHw = static_cast<BcmSwitch*>(hw);
  auto vrfId = bcmHw->getBcmVrfId(routerId);
  auto bcmHostKey = BcmHostKey(vrfId, nhop);
  BcmHostIf* bcmHost;
  if (hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HOSTTABLE)) {
    bcmHost = bcmHw->getHostTable()->getBcmHostIf(bcmHostKey);
  } else {
    bcmHost = bcmHw->routeTable()->getBcmHostIf(bcmHostKey);
  }
  CHECK(bcmHost) << "failed to find host for " << bcmHostKey.str();

  bcm_if_t id = bcmHost->getEgressId();
  bcm_l3_egress_t egr;
  auto rv = bcm_l3_egress_get(bcmHw->getUnit(), bcmHost->getEgressId(), &egr);
  bcmCheckError(rv, "failed bcm_l3_egress_get");

  uint32_t flags = BCM_L3_REPLACE | BCM_L3_WITH_ID | BCM_L3_KEEP_TTL;
  rv = bcm_l3_egress_create(bcmHw->getUnit(), flags, &egr, &id);
  bcmCheckError(rv, "failed bcm_l3_egress_create");
}

void disableTTLDecrements(HwSwitch* /*hw*/, const PortDescriptor& /*port*/) {
  throw FbossError("Port disable decrement not supported on BRCM ASICs");
}

void enableTtlZeroPacketForwarding(HwSwitch* /*hw*/) {
  throw FbossError("TTL0 packet forwarding not supported!");
}
} // namespace facebook::fboss::utility
