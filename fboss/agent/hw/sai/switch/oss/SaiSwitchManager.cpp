// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"

namespace facebook::fboss {

void fillHwSwitchDramStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDramStats& /*hwSwitchDramStats*/) {
  CHECK_EQ(counterId2Value.size(), 0);
}

} // namespace facebook::fboss
