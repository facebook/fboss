// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/BufferApi.h"

extern "C" {
#include <experimental/sai_attr_ext.h>
}

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

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
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

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiIngressPriorityGroupTraits::Attributes::
    AttributeLosslessEnable::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiBufferPoolTraits::Attributes::AttributeReservedBytes::operator()() {
#if defined(TAJO_SDK_GTE_25_5)
  return SAI_BUFFER_POOL_ATTR_RESERVED_BUFFER_SIZE;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
