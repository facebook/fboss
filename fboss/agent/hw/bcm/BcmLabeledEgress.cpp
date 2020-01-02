// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"

#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/state/Interface.h"

namespace facebook::fboss {

BcmWarmBootCache::EgressId2EgressCitr BcmLabeledEgress::findEgress(
    opennsl_vrf_t vrf,
    opennsl_if_t intfId,
    const folly::IPAddress& ip) const {
  auto* bcmIntf = hw_->getIntfTable()->getBcmIntf(intfId);

  return hw_->getWarmBootCache()->findEgressFromLabeledHostKey(
      BcmLabeledHostKey(vrf, getLabel(), ip, bcmIntf->getInterface()->getID()));
}

} // namespace facebook::fboss
