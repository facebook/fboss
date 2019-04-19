/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmUnit.h"

#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/bcm/BcmError.h"

#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

#include <chrono>

extern "C" {
#include <opennsl/init.h>
#include <opennsl/switch.h>
#include <sal/driver.h>
} // extern "C"

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;

namespace facebook {
namespace fboss {

void BcmUnit::writeWarmBootState(const folly::dynamic& switchState) {
  steady_clock::time_point begin = steady_clock::now();
  XLOG(INFO) << " [Exit] Syncing BRCM switch state to file";
  // Force the device to write out its warm boot state
  auto rv = opennsl_switch_control_set(unit_, opennslSwitchControlSync, 1);
  bcmCheckError(rv, "Unable to sync state for L2 warm boot");

  steady_clock::time_point bcmWarmBootSyncDone = steady_clock::now();
  XLOG(INFO)
      << "[Exit] BRCM warm boot sync time "
      << duration_cast<duration<float>>(bcmWarmBootSyncDone - begin).count();
  // Now write our state to file
  XLOG(INFO) << " [Exit] Syncing FBOSS switch state to file";
  if (!warmBootHelper()->storeWarmBootState(switchState)) {
    XLOG(FATAL) << "Unable to write switch state JSON to file";
  }
  steady_clock::time_point fbossWarmBootSyncDone = steady_clock::now();
  XLOG(INFO) << "[Exit] Fboss warm boot sync time "
             << duration_cast<duration<float>>(
                    fbossWarmBootSyncDone - bcmWarmBootSyncDone)
                    .count();
}
} // namespace fboss
} // namespace facebook
