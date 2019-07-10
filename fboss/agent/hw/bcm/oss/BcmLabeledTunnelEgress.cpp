// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"

namespace facebook {
namespace fboss {

void BcmLabeledTunnelEgress::prepareEgressObject(
    opennsl_if_t /*intfId*/,
    opennsl_port_t /*port*/,
    const folly::Optional<folly::MacAddress>& /*mac*/,
    RouteForwardAction /*action*/,
    opennsl_l3_egress_t* /*eObj*/) const {
  CHECK(0); // no MPLS in OSS
}

} // namespace fboss
} // namespace facebook
