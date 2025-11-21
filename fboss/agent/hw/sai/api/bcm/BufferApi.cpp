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

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSharedFadtMaxTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SHARED_FADT_MAX_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSharedFadtMinTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SHARED_FADT_MIN_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramFadtMinTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_MIN_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramFadtMaxTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_MAX_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSramFadtXonOffset::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_XON_OFFSET;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramDynamicTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0) && !defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_DYNAMIC_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSharedFadtMaxTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SHARED_FADT_MAX_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSharedFadtMinTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SHARED_FADT_MIN_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtMinTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_MIN_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtMaxTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_MAX_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtXonOffset::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_FADT_XON_OFFSET;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramDynamicTh::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0) && !defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  return SAI_BUFFER_PROFILE_ATTR_SRAM_DYNAMIC_TH;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiIngressPriorityGroupTraits::Attributes::
    AttributeLosslessEnable::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && !defined(BRCM_SAI_SDK_DNX)
  return SAI_INGRESS_PRIORITY_GROUP_ATTR_LOSSLESS_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
