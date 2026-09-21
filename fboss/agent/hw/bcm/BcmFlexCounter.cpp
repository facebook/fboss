/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmFlexCounter.h"

#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/flexctr.h>
}

namespace facebook::fboss {
void BcmFlexCounter::destroy(int unit, uint32_t counterID) {
  if (counterID == 0) {
    return;
  }
  auto rv = bcm_flexctr_action_destroy(unit, counterID);
  bcmCheckError(rv, "Failed to remove FlexCounter:", counterID);
  XLOG(DBG1) << "Successfully removed FlexCounter:" << counterID;
}
} // namespace facebook::fboss
