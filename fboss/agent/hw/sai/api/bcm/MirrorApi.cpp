// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/MirrorApi.h"

// TODO: Add support for 12.0 as well
#if defined(SAI_VERSION_11_3_0_0_DNX_ODP)
extern "C" {

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saimirrorextensions.h>
#else
#include <saimirrorextensions.h>
#endif
}
#endif

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiSflowMirrorTraits::Attributes::AttributeTcBufferLimit::operator()() {
// TODO: Add support for 12.0 as well
#if defined(SAI_VERSION_11_3_0_0_DNX_ODP)
  return SAI_MIRROR_SESSION_ATTR_TC_BUFFER_LIMIT;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
