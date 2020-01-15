/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTrunk.h"

namespace facebook::fboss {

int BcmTrunk::getEnabledMemberPortsCountHwNotLocked(
    int /* unit */,
    bcm_trunk_t /* trunk */,
    bcm_port_t /* port */) {
  return 0;
}

bcm_gport_t BcmTrunk::asGPort(bcm_trunk_t /* trunk */) {
  return static_cast<bcm_gport_t>(0);
}
bool BcmTrunk::isValidTrunkPort(bcm_gport_t /* gPort */) {
  return false;
}

int BcmTrunk::rtag7() {
  return 0;
}

void BcmTrunk::suppressTrunkInternalFlood(
    const std::shared_ptr<AggregatePort>& aggPort) {
  return;
}

} // namespace facebook::fboss
