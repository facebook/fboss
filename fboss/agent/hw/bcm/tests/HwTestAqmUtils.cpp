/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/cosq.h>
}

namespace facebook::fboss::utility {
uint32_t getEgressSharedPoolLimitBytes(HwSwitch* hwSwitch) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  int sharedLimitBytes{};
  auto rv = bcm_cosq_control_get(
      bcmSwitch->getUnit(),
      bcmSwitch->getCpuGPort(),
      0,
      bcmCosqControlEgressPoolSharedLimitBytes,
      &sharedLimitBytes);
  bcmCheckError(
      rv, "Cannot get egress pool shared limit for default service pool!");
  return sharedLimitBytes;
}
} // namespace facebook::fboss::utility
