// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"

namespace facebook {
namespace fboss {
BcmWarmBootCache::EgressId2EgressCitr BcmLabeledEgress::findEgress(
    opennsl_vrf_t /*vrf*/,
    opennsl_if_t /*intfId*/,
    const folly::IPAddress& /*ip*/) const {
  // TODO(pshaikh) : support warmboot for labeled tunnel egress
  return hw_->getWarmBootCache()->egressId2Egress_end();
}

} // namespace fboss
} // namespace facebook
