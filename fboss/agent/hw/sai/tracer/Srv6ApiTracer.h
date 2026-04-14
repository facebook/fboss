// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
sai_srv6_api_t* wrappedSrv6Api();

SET_ATTRIBUTE_FUNC_DECLARATION(Srv6SidList);
SET_ATTRIBUTE_FUNC_DECLARATION(MySidEntry);
#endif

} // namespace facebook::fboss
