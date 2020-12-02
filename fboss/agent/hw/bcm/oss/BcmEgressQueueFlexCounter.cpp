/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmEgressQueueFlexCounter.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {
BcmEgressQueueFlexCounter::BcmEgressQueueFlexCounter(
    BcmSwitch* hw,
    int /* numPorts */,
    int /* numQueuesPerPort */,
    bool /* isForCPU */)
    : BcmFlexCounter(hw->getUnit()), hw_(hw) {
  throw FbossError("OSS doesn't support creating egress queue flex counter");
}

void BcmEgressQueueFlexCounter::attach(bcm_gport_t /* gPort */) {
  throw FbossError(
      "OSS doesn't support attach Egress Queue FlexCounter to port");
}

BcmEgressQueueFlexCounterManager::BcmEgressQueueFlexCounterManager(
    BcmSwitch* /* hw */) {
  throw FbossError("OSS doesn't support egress queue flex counter manager");
}
} // namespace facebook::fboss
