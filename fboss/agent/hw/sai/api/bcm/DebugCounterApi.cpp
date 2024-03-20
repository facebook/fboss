// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"

extern "C" {
#if defined(BRCM_SAI_SDK_GTE_11_0) && !defined(IS_OSS_BRCM_SAI)
#include <experimental/saidebugcounterextensions.h>
#else
#include <saidebugcounterextensions.h>
#endif
#include <sai.h>
}

namespace facebook::fboss::detail {

std::optional<sai_int32_t> trapDrops() {
#if defined BRCM_SAI_SDK_GTE_11_0
  return SAI_IN_DROP_REASON_ALL_TRAP_DROPS;
#endif
  return std::nullopt;
}
} // namespace facebook::fboss::detail
