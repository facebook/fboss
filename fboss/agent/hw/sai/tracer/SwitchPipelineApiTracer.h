// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)

extern "C" {
#include <sai.h>
#include <saiextensions.h>
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentalswitchpipeline.h>
#else
#include <saiexperimentalswitchpipeline.h>
#endif
}

namespace facebook::fboss {

sai_switch_pipeline_api_t* wrappedSwitchPipelineApi();

SET_ATTRIBUTE_FUNC_DECLARATION(SwitchPipeline);

} // namespace facebook::fboss

#endif
