/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/tracer/SaiTracer.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
sai_firmware_api_t* wrappedFirmwareApi();

SET_ATTRIBUTE_FUNC_DECLARATION(Firmware);
#endif

} // namespace facebook::fboss

#endif
