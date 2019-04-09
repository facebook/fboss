// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"

namespace facebook {
namespace fboss {

int BcmLabeledTunnelEgress::createEgress(
    int /*unit*/,
    uint32_t /*flags*/,
    opennsl_l3_egress_t* /*egr*/) {
  return 0;
}

} // namespace fboss
} // namespace facebook
