// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/ArsApi.h"

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
std::optional<sai_attr_id_t>
SaiArsTraits::Attributes::AttributeNextHopGroupType::operator()() {
  return std::nullopt;
}
#endif

} // namespace facebook::fboss
