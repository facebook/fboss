// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/RxUtils.h"

extern "C" {
#ifdef INCLUDE_PKTIO
#include <bcm/pktio.h>
#include <bcm/pktio_defs.h>
#endif
}

namespace {
const char* const kRxReasonNames[] = BCM_RX_REASON_NAMES_INITIALIZER;

#ifdef INCLUDE_PKTIO
const struct {
  std::string name;
  int reason;
} kPktIORxReasonInfos[] = {BCMPKT_REASON_NAME_MAP_INIT};
#endif
} // namespace

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

std::string RxUtils::describeReasons(const BcmRxReasonsT& bcmRxReasons) {
  std::string result;

  if (!bcmRxReasons.usePktIO) {
    result = describeReasons(bcmRxReasons.reasonsUnion.reasons);
  } else {
#ifdef INCLUDE_PKTIO
    int reason_code;
    for (reason_code = BCMPKT_RX_REASON_NONE;
         reason_code < BCMPKT_RX_REASON_COUNT;
         reason_code++) {
      if (BCM_PKTIO_REASON_GET(
              bcmRxReasons.reasonsUnion.pktio_reasons.rx_reasons,
              reason_code)) {
        if (!result.empty()) {
          result.append(",");
        }
        result.append(kPktIORxReasonInfos[reason_code].name);
      }
    }
#endif
  }
  return result;
}

} // namespace facebook::fboss
