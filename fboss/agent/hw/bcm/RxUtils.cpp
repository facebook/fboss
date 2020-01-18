// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/RxUtils.h"

namespace {
const char* const kRxReasonNames[] = BCM_RX_REASON_NAMES_INITIALIZER;
}

namespace facebook::fboss {

std::string RxUtils::describeReasons(bcm_rx_reasons_t reasons) {
  std::string result;
  for (int n = 0; n < bcmRxReasonCount; ++n) {
    if (BCM_RX_REASON_GET(reasons, n)) {
      if (!result.empty()) {
        result.append(",");
      }
      result.append(kRxReasonNames[n]);
    }
  }
  return result;
}

} // namespace facebook::fboss
