// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"

namespace facebook::fboss {

void fillHwSwitchDramStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDramStats& /*hwSwitchDramStats*/) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void fillHwSwitchWatermarkStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchWatermarkStats& /*hwSwitchWatermarkStats*/) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void fillHwSwitchCreditStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchCreditStats& /*hwSwitchCreditStats*/) {
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
    HwSwitchPipelineStats& /*hwSwitchPipelineStats*/) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void fillHwSwitchSaiExtensionDropStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDropStats& /* dropStats */) {
  CHECK_EQ(counterId2Value.size(), 0);
}

void publishSwitchWatermarks(HwSwitchWatermarkStats& /*watermarkStats*/) {}

void publishSwitchPipelineStats(HwSwitchPipelineStats& /*pipelineStats*/) {}

void publishSwitchTemperatureStats(
    HwSwitchTemperatureStats& /*temperatureStats*/) {}

} // namespace facebook::fboss
