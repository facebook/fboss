// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/QueueApi.h"

#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
extern "C" {
#include <brcm_sai_extensions.h>
#include <sai.h>
}
#endif

namespace facebook::fboss {

const std::vector<sai_stat_id_t>& SaiQueueTraits::egressGvoqWatermarkBytes() {
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_QUEUE_STAT_EGRESS_GVOQ_THRESHOLD_WM};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}
} // namespace facebook::fboss
