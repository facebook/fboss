// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/BufferApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSharedFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSharedFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSramFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSramFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSramFadtXonOffset::operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
