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

void publishSwitchWatermarks(HwSwitchWatermarkStats& /*watermarkStats*/) {}

void publishSwitchPipelineStats(HwSwitchPipelineStats& /*pipelineStats*/) {}

void publishSwitchTemperatureStats(
    HwSwitchTemperatureStats& /*temperatureStats*/) {}

} // namespace facebook::fboss
