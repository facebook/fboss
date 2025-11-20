// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/BufferApi.h"

#if !defined(CHENAB_SAI_SDK_VERSION_116_133_2509_6)
#include "saibuffercustom.h"
#endif

namespace facebook::fboss {

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSharedFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSharedFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSramFadtXonOffset::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramDynamicTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSharedFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSharedFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtXonOffset::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramDynamicTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiIngressPriorityGroupTraits::Attributes::
    AttributeLosslessEnable::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
#if !defined(CHENAB_SAI_SDK_VERSION_116_133_2509_6)
  return SAI_BUFFER_PROFILE_ATTR_PG_PIPELINE_LATENCY_SIZE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
#if !defined(CHENAB_SAI_SDK_VERSION_116_133_2509_6)
  return SAI_BUFFER_PROFILE_ATTR_PG_PIPELINE_LATENCY_SIZE;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
