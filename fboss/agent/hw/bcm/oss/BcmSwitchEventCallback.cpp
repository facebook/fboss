/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitchEventCallback.h"

#include <glog/logging.h>

namespace facebook { namespace fboss {

void BcmSwitchEventUnitNonFatalErrorCallback::logNonFatalError(
    int unit,
    const std::string& alarm,
    opennsl_switch_event_t eventID,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3) {
  LOG(ERROR) << "BCM non-fatal error on unit " << unit << ": " << alarm << " ("
             << eventID << ") with params " << arg1 << ", " << arg2 << ", "
             << arg3;
}
}} // facebook::fboss
