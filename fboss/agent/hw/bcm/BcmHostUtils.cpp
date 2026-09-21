// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmHostUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {

void setTTLDecrement(
    const BcmSwitch* bcmHw,
    const BcmHostKey& bcmHostKey,
    bool noDecrement) {
  BcmHostIf* bcmHost{nullptr};
  if (bcmHw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::HOSTTABLE)) {
    bcmHost = bcmHw->getHostTable()->getBcmHostIf(bcmHostKey);
  } else {
    bcmHost = bcmHw->routeTable()->getBcmHostIf(bcmHostKey);
  }
  CHECK(bcmHost) << "failed to find host for " << bcmHostKey.str();

  bcm_if_t id = bcmHost->getEgressId();
  bcm_l3_egress_t egr;
  auto rv = bcm_l3_egress_get(bcmHw->getUnit(), bcmHost->getEgressId(), &egr);
  bcmCheckError(rv, "failed bcm_l3_egress_get");
  uint32_t flags = (BCM_L3_REPLACE | BCM_L3_WITH_ID);
  if (noDecrement) {
    flags |= (egr.flags | BCM_L3_KEEP_TTL);
  } else {
    flags |= (egr.flags & ~BCM_L3_KEEP_TTL);
  }
  rv = bcm_l3_egress_create(bcmHw->getUnit(), flags, &egr, &id);
  bcmCheckError(rv, "failed bcm_l3_egress_create");
}

} // namespace facebook::fboss
