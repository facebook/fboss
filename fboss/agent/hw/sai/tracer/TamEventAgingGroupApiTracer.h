// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#if defined(SAI_VERSION_11_3_0_0_DNX_ODP)

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"

extern "C" {
#include <sai.h>
#include <saiextensions.h>
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentaltameventaginggroup.h>
#else
#include <saiexperimentaltameventaginggroup.h>
#endif
}

namespace facebook::fboss {

sai_tam_event_aging_group_api_t* wrappedTamEventAgingGroupApi();

SET_ATTRIBUTE_FUNC_DECLARATION(TamEventAgingGroup);

} // namespace facebook::fboss

#endif
