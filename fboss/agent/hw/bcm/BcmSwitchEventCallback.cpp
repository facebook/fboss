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
#include "fboss/agent/hw/bcm/BcmStats.h"
#include <glog/logging.h>

namespace facebook { namespace fboss {

namespace switch_event_helpers {

std::string getAlarmName(const opennsl_switch_event_t eventID) {
  switch (eventID) {
    case OPENNSL_SWITCH_EVENT_STABLE_FULL:
      return "OPENNSL_SWITCH_EVENT_STABLE_FULL";
    case OPENNSL_SWITCH_EVENT_STABLE_ERROR:
      return "OPENNSL_SWITCH_EVENT_STABLE_ERROR";
    case OPENNSL_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN:
      return "OPENNSL_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN";
    case OPENNSL_SWITCH_EVENT_WARM_BOOT_DOWNGRADE:
      return "OPENNSL_SWITCH_EVENT_WARM_BOOT_DOWNGRADE";
    case OPENNSL_SWITCH_EVENT_PARITY_ERROR:
      return "OPENNSL_SWITCH_EVENT_PARITY_ERROR";
    default:
      return "UNKNOWN";
  }
}

void exportEventCounters(const opennsl_switch_event_t eventID) {
  if (eventID == OPENNSL_SWITCH_EVENT_PARITY_ERROR) {
      BcmStats::get()->parityError();
  }
}

} // switch_event_helpers

void BcmSwitchEventUnitFatalErrorCallback::callback(const int unit,
    const opennsl_switch_event_t eventID, const uint32_t arg1,
    const uint32_t arg2, const uint32_t arg3) {
  auto alarm = switch_event_helpers::getAlarmName(eventID);
  LOG(FATAL) << "BCM Fatal error on unit " << unit << ": " << alarm
             << " (" << eventID << ") with params "
             << arg1 << ", " << arg2 << ", " << arg3;
}

void BcmSwitchEventUnitNonFatalErrorCallback::callback(const int unit,
    const opennsl_switch_event_t eventID, const uint32_t arg1,
    const uint32_t arg2, const uint32_t arg3) {

  switch_event_helpers::exportEventCounters(eventID);

  auto alarm = switch_event_helpers::getAlarmName(eventID);
  LOG(ERROR) << "BCM non-fatal error on unit " << unit << ": " << alarm
             << " (" << eventID << ") with params "
             << arg1 << ", " << arg2 << ", " << arg3;
}

}} // facebook::fboss
