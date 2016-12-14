/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmSwitchEventCallback.h"
#include <glog/logging.h>

namespace facebook { namespace fboss {

void BcmSwitchEventUnitFatalErrorCallback::callback(const int unit,
    const opennsl_switch_event_t eventID, const uint32_t arg1,
    const uint32_t arg2, const uint32_t arg3) {
  std::string alarm;
  switch (eventID) {
    case OPENNSL_SWITCH_EVENT_STABLE_FULL:
      alarm = "OPENNSL_SWITCH_EVENT_STABLE_FULL";
      break;
    case OPENNSL_SWITCH_EVENT_STABLE_ERROR:
      alarm = "OPENNSL_SWITCH_EVENT_STABLE_ERROR";
      break;
    case OPENNSL_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN:
      alarm = "OPENNSL_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN";
      break;
    case OPENNSL_SWITCH_EVENT_WARM_BOOT_DOWNGRADE:
      alarm = "OPENNSL_SWITCH_EVENT_WARM_BOOT_DOWNGRADE";
      break;
    default:
      alarm = "UNKNOWN";
  }

  LOG(FATAL) << "BCM Fatal error on unit " << unit << ": " << alarm
             << " (" << eventID << ") with params "
             << arg1 << ", " << arg2 << ", " << arg3;
}

}}
