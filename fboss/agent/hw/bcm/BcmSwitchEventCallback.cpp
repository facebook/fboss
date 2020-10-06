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
#include "fboss/lib/AlertLogger.h"

#include <folly/logging/xlog.h>
#include <glog/logging.h>
#include "fboss/agent/hw/bcm/BcmSwitchEventUtils.h"

extern "C" {
#include <bcm/switch.h>

#if (!defined(BCM_VER_MAJOR))
#include <soc/opensoc.h>
#else
#include <soc/error.h>
#endif
}

namespace facebook::fboss {

namespace switch_event_helpers {

std::string getParityDataErrName(const soc_switch_data_error_t dataErr) {
  switch (dataErr) {
    case SOC_SWITCH_EVENT_DATA_ERROR_PARITY:
      return "SOC_SWITCH_EVENT_DATA_ERROR_PARITY";
    case SOC_SWITCH_EVENT_DATA_ERROR_ECC:
      return "SOC_SWITCH_EVENT_DATA_ERROR_ECC";
    case SOC_SWITCH_EVENT_DATA_ERROR_UNSPECIFIED:
      return "SOC_SWITCH_EVENT_DATA_ERROR_UNSPECIFIED";
    case SOC_SWITCH_EVENT_DATA_ERROR_FATAL:
      return "SOC_SWITCH_EVENT_DATA_ERROR_FATAL";
    case SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED:
      return "SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED";
    case SOC_SWITCH_EVENT_DATA_ERROR_LOG:
      return "SOC_SWITCH_EVENT_DATA_ERROR_LOG";
    case SOC_SWITCH_EVENT_DATA_ERROR_UNCORRECTABLE:
      return "SOC_SWITCH_EVENT_DATA_ERROR_UNCORRECTABLE";
    case SOC_SWITCH_EVENT_DATA_ERROR_AUTO_CORRECTED:
      return "SOC_SWITCH_EVENT_DATA_ERROR_AUTO_CORRECTED";
    case SOC_SWITCH_EVENT_DATA_ERROR_FAILEDTOCORRECT:
      return "SOC_SWITCH_EVENT_DATA_ERROR_FAILEDTOCORRECT";
    case SOC_SWITCH_EVENT_DATA_ERROR_ECC_1BIT_CORRECTED:
      return "SOC_SWITCH_EVENT_DATA_ERROR_ECC_1BIT_CORRECTED";
    case SOC_SWITCH_EVENT_DATA_ERROR_ECC_2BIT_CORRECTED:
      return "SOC_SWITCH_EVENT_DATA_ERROR_ECC_2BIT_CORRECTED";
    case SOC_SWITCH_EVENT_DATA_ERROR_PARITY_CORRECTED:
      return "SOC_SWITCH_EVENT_DATA_ERROR_PARITY_CORRECTED";
    default:
      return "UNKNOWN";
  }
}

} // namespace switch_event_helpers

void BcmSwitchEventUnitNonFatalErrorCallback::callback(
    const int unit,
    const bcm_switch_event_t eventID,
    const uint32_t arg1,
    const uint32_t arg2,
    const uint32_t arg3,
    void* data) {
  auto alarm = BcmSwitchEventUtils::getAlarmName(eventID);

  if (eventID != BCM_SWITCH_EVENT_PARITY_ERROR) {
    BcmSwitchEventUtils::exportEventCounters(eventID, 1, data);
  } else {
    // Parity errors can be fatal or nonfatal
    auto errName = switch_event_helpers::getParityDataErrName(
        (soc_switch_data_error_t)arg1);
    switch (arg1) {
      case SOC_SWITCH_EVENT_DATA_ERROR_UNSPECIFIED:
      case SOC_SWITCH_EVENT_DATA_ERROR_FATAL:
      case SOC_SWITCH_EVENT_DATA_ERROR_UNCORRECTABLE:
      case SOC_SWITCH_EVENT_DATA_ERROR_FAILEDTOCORRECT:
        BcmSwitchEventUtils::exportEventCounters(eventID, 1, data);

        XLOG(ERR) << AsicAlert() << "BCM Uncorrectable error on unit " << unit
                  << ": " << alarm << " (" << eventID << "), " << errName
                  << " (" << arg1 << "), " << arg2 << ", " << arg3;
        return;
      default:
        BcmSwitchEventUtils::exportEventCounters(eventID, 0, data);
    }
  }

  logNonFatalError(unit, alarm, eventID, arg1, arg2, arg3);
}

void BcmSwitchEventUnitNonFatalErrorCallback::logNonFatalError(
    int unit,
    const std::string& alarm,
    bcm_switch_event_t eventID,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3) {
  XLOG_N_PER_MS(ERR, 100, 1000)
      << AsicAlert() << "BCM non-fatal error on unit " << unit << ": " << alarm
      << " (" << eventID << ") with params " << arg1 << ", " << arg2 << ", "
      << arg3;
}

void BcmSwitchEventUnitFatalErrorCallback::callback(
    const int unit,
    const bcm_switch_event_t eventID,
    const uint32_t arg1,
    const uint32_t arg2,
    const uint32_t arg3,
    void* /* data */) {
  auto alarm = BcmSwitchEventUtils::getAlarmName(eventID);
  XLOG(ERR) << AsicAlert() << "BCM uncorrected error on unit " << unit << ": "
            << alarm << " (" << eventID << ") with params " << arg1 << ", "
            << arg2 << ", " << arg3;
}

} // namespace facebook::fboss
