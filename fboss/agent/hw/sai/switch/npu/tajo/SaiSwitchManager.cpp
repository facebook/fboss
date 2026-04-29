// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"

#if !defined(TAJO_SDK_VERSION_1_42_8)
extern "C" {
#include <experimental/sai_attr_ext.h>
}
#endif

namespace facebook::fboss {

void fillHwSwitchDramStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDramStats& /*hwSwitchDramStats*/) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void fillHwSwitchWatermarkStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchWatermarkStats& hwSwitchWatermarkStats) {
#if !defined(TAJO_SDK_VERSION_1_42_8)
  auto it = counterId2Value.find(SAI_SWITCH_STAT_DEVICE_WATERMARK_BYTES);
  if (it != counterId2Value.end()) {
    hwSwitchWatermarkStats.deviceWatermarkBytes() = it->second;
  }
#endif
}

void fillHwSwitchCreditStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchCreditStats& /* hwSwitchCreditStats */) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void fillHwSwitchErrorStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDropStats& /*dropStats*/) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void fillHwSwitchPipelineStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    int /*idx*/,
    HwSwitchPipelineStats& /* hwSwitchPipelineStats */) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void fillHwSwitchSaiExtensionDropStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDropStats& /* dropStats */) {
  CHECK_EQ(counterId2Value.size(), 0);
}

} // namespace facebook::fboss
