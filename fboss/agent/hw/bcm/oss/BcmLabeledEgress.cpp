// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"

extern "C" {
#include <opennsl/l3.h>
}

namespace facebook {
namespace fboss {

void BcmLabeledEgress::setLabel(opennsl_l3_egress_t* /*egress*/) const {}
int BcmLabeledEgress::createEgress(
    int /*unit*/,
    uint32_t /*flags*/,
    opennsl_l3_egress_t* /*egr*/) {
  return OPENNSL_E_UNAVAIL;
}

} // namespace fboss
} // namespace facebook
