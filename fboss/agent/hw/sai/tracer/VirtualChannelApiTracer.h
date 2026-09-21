// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"

#if defined(BRCM_SAI_SDK_XGS_GTE_16_0)

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

sai_virtual_channel_api_t* wrappedVirtualChannelApi();

SET_ATTRIBUTE_FUNC_DECLARATION(VirtualChannel);
SET_ATTRIBUTE_FUNC_DECLARATION(CbfcCreditProfile);

} // namespace facebook::fboss

#endif
