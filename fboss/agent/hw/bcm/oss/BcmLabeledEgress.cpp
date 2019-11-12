// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"

extern "C" {
#include <opennsl/l3.h>
}

namespace facebook {
namespace fboss {

void BcmLabeledEgress::prepareEgressObject(
    opennsl_if_t /*intfId*/,
    opennsl_port_t /*port*/,
    const std::optional<folly::MacAddress>& /*mac*/,
    RouteForwardAction /*action*/,
    opennsl_l3_egress_t* /*eObj*/) const {
  CHECK(0); // no MPLS in OSS
}

} // namespace fboss
} // namespace facebook
