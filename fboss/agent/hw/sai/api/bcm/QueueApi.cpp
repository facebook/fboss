// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/QueueApi.h"

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

const std::vector<sai_stat_id_t>& SaiQueueTraits::egressGvoqWatermarkBytes() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_QUEUE_STAT_EGRESS_GVOQ_THRESHOLD_WM_BYTES};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}
} // namespace facebook::fboss
