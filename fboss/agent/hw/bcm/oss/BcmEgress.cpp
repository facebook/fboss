/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmEgress.h"

namespace facebook { namespace fboss {

bool BcmEcmpEgress::removeEgressIdHwNotLocked(
    int /*unit*/,
    opennsl_if_t /*ecmpId*/,
    opennsl_if_t /*toRemove*/) {
  return false; // Not supported in opennsl;
}

bool BcmEcmpEgress::addEgressIdHwLocked(
    int /*unit*/,
    opennsl_if_t /*ecmpId*/,
    const Paths& /*egressIdInSw*/,
    opennsl_if_t /*toAdd*/) {
  return false; // Not supported in opennsl
}

}}
