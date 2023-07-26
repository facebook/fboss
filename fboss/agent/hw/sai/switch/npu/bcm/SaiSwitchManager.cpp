// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"

extern "C" {
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiswitchextensions.h>
#else
#include <saiswitchextensions.h>
#endif
}

namespace facebook::fboss {

void fillHwSwitchDramStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDramStats& hwSwitchDramStats) {
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
#if defined(SAI_VERSION_10_0_EA_DNX_ODP) || \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP)
      case SAI_SWITCH_STAT_DEVICE_DRAM_ENQUEUED_BYTES:
        hwSwitchDramStats.dramEnqueuedBytes() = value;
        break;
      case SAI_SWITCH_STAT_DEVICE_DRAM_DEQUEUED_BYTES:
        hwSwitchDramStats.dramDequeuedBytes() = value;
        break;
#endif
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}

} // namespace facebook::fboss
