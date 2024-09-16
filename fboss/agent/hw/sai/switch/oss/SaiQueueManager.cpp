// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"

namespace facebook::fboss {

bool fillQueueExtensionStats(
    const uint8_t /*queueId*/,
    const sai_stat_id_t& /*counterId*/,
    const uint64_t& /*counterValue*/,
    HwPortStats& /*hwPortStats*/) {
  // No extension stat ID for Queue
  return false;
}

} // namespace facebook::fboss
