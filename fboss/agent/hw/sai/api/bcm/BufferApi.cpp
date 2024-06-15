// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/BufferApi.h"

extern "C" {

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saibufferextensions.h>
#else
#include <saibufferextensions.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSharedFadtMaxTh::operator()() {
#if defined(SAI_VERSION_11_0_EA_DNX_ODP)
  return SAI_BUFFER_PROFILE_ATTR_SHARED_FADT_MAX_TH;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
