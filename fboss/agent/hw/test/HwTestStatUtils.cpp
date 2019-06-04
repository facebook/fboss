/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestStatUtils.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/SwitchStats.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {
void updateHwSwitchStats(HwSwitch* hw) {
  try {
    SwitchStats dummy;
    hw->updateStats(&dummy);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error running updateStats: " << folly::exceptionStr(ex);
  }
}
} // namespace fboss
} // namespace facebook
