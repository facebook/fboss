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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {

bool operator==(
    const opennsl_l3_egress_t& lhs,
    const opennsl_l3_egress_t& rhs) {
  bool sameMacs = !memcmp(lhs.mac_addr, rhs.mac_addr, sizeof(lhs.mac_addr));
  bool lhsTrunk = lhs.flags & OPENNSL_L3_TGID;
  bool rhsTrunk = rhs.flags & OPENNSL_L3_TGID;
  bool sameTrunks = lhsTrunk && rhsTrunk && lhs.trunk == rhs.trunk;
  bool samePhysicalPorts = !lhsTrunk && !rhsTrunk && rhs.port == lhs.port;
  bool samePorts = sameTrunks || samePhysicalPorts;
  return sameMacs && samePorts && rhs.intf == lhs.intf &&
      rhs.flags == lhs.flags;
}

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
opennsl_mpls_label_t getLabel(const opennsl_l3_egress_t& /*egress*/) {
  return 0xffffffff;
}

} // namespace facebook::fboss
