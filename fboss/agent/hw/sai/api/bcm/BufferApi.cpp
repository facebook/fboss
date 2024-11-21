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
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SHARED_FADT_MAX_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSharedFadtMinTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SHARED_FADT_MIN_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSramFadtMinTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_MIN_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSramFadtMaxTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_MAX_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSramFadtXonOffset::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_XON_OFFSET;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
