// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/MirrorApi.h"

// TODO: Add support for 12.0 as well
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
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
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_MIRROR_SESSION_ATTR_TC_BUFFER_LIMIT;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiEnhancedRemoteMirrorTraits::Attributes::
    AttributeTcBufferLimit::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_MIRROR_SESSION_ATTR_TC_BUFFER_LIMIT;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiLocalMirrorTraits::Attributes::AttributeTcBufferLimit::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_MIRROR_SESSION_ATTR_TC_BUFFER_LIMIT;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
