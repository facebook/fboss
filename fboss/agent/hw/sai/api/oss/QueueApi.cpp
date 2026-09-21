// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/QueueApi.h"

namespace facebook::fboss {

const std::vector<sai_stat_id_t>& SaiQueueTraits::egressGvoqWatermarkBytes() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

} // namespace facebook::fboss
