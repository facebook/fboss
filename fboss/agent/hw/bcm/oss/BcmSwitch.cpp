/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitch.h"

/*
 * Stubbed out.
 */
namespace facebook::fboss {

void BcmSwitch::printDiagCmd(const std::string& /*cmd*/) const {}

std::shared_ptr<SwitchState> BcmSwitch::stateChangedWithOperDeltaLocked(
    const StateDelta& legacyDelta,
    const std::lock_guard<std::mutex>& lock) {
  return stateChangedLocked(legacyDelta, lock);
}
} // namespace facebook::fboss
