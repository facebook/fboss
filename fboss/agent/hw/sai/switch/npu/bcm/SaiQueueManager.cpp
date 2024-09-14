// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"

#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
extern "C" {

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiqueueextensions.h>
#else
#include <saiqueueextensions.h>
#endif
}
#endif

namespace facebook::fboss {

#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
bool fillQueueExtensionStats(
    const uint8_t queueId,
    const sai_stat_id_t& counterId,
    const uint64_t& counterValue,
    HwPortStats& hwPortStats) {
  bool counterHandled = true;
  switch (counterId) {
    case SAI_QUEUE_STAT_EGRESS_GVOQ_THRESHOLD_WM_BYTES:
      // There are a few limitations w.r.t. GVOQ watermarks
      // 1. Granularity of this counter is 8K bytes as of now
      // 2. Max VoQ watermark that can be reported per VoQ is 16M
      // 3. Max GVOQ watermark that can be reported per EGQ is 256M
      hwPortStats.egressGvoqWatermarkBytes_()[queueId] = counterValue;
      break;
    default:
      throw FbossError("Got unexpected queue counter id: ", counterId);
  }
  return counterHandled;
}
#else
bool fillQueueExtensionStats(
    const uint8_t /*queueId*/,
    const sai_stat_id_t& /*counterId*/,
    const uint64_t& /*counterValue*/,
    HwPortStats& /*hwPortStats*/) {
  // No extension stat ID for Queue
  return false;
}
#endif

} // namespace facebook::fboss
